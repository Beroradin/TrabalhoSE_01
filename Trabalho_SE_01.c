#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "ws2818b.pio.h"
#include "inc/ssd1306.h"
#include "inc/font.h"



// Definição dos pinos
#define EIXO_Y 26    // ADC0
#define EIXO_X 27    // ADC1
#define LED1_PIN 11  // LED controlado pelo botão do joystick
#define LED2_PIN 12  // LED controlado pelo eixo Y
#define LED3_PIN 13  // LED controlado pelo eixo X
#define I2C_SDA 14   // Pino SDA do i2c   
#define I2C_SCL 15   // Pino SCL do i2c
#define I2C_PORT i2c1    
#define BUTTONA_PIN 5  // Botão A
#define BUTTONB_PIN 6 // Botão B
#define BUZZER_PIN 21 // Buzzer
#define JOYSTICK_BUTTON 22 // Botão do joystick
#define PWM_WRAP    4095    // 12 bits de wrap (4096 valores)

// Constantes
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define SQUARE_SIZE 8
#define LED_COUNT 25
#define LED_PIN 7

// Variáveis globais
volatile bool pwm_enabled = true;
volatile bool buzzer_enable = true;
volatile bool borda_style = false;
volatile bool green_value = false;
absolute_time_t last_interrupt_time = 0;
volatile int digit = 10;  // Digito da matriz de led
bool cor = true;


// Variáveis para posição do quadrado
int pos_x = (DISPLAY_WIDTH - SQUARE_SIZE) / 2;  // Centraliza horizontalmente
int pos_y = (DISPLAY_HEIGHT - SQUARE_SIZE) / 2; // Centraliza verticalmente
uint16_t x_value = 0; // Valor do eixo X do joystick
uint16_t y_value = 0; // Valor do eixo Y do joystick
const int SPEED = 1;  // Velocidade do quadrado
const int MAX_X = DISPLAY_WIDTH - SQUARE_SIZE;  // Limite direito
const int MAX_Y = DISPLAY_HEIGHT - SQUARE_SIZE; // Limite inferior

// Definição da estrutura do pixel
struct pixel_t {
    uint8_t G, R, B;
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t;

npLED_t leds[LED_COUNT];
PIO np_pio;
uint sm;

// Protótipos de funções
void gpio_callback(uint gpio, uint32_t events);
void leitura_e_controle_joystick(uint slice1, uint slice2);
void npDisplayDigit(int digit);
bool repeating_timer_callback(struct repeating_timer *t);
void verificacao_extremidade(int pos_x, int pos_y);

// Matrizes para cada dígito 
const uint8_t digits[11][5][5][3] = {
    // Dígito 0
    {
        {{0, 0, 0}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 0}}, 
        {{0, 0, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 110}},    
        {{0, 0, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 110}},    
        {{0, 0, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 110}},    
        {{0, 0, 0}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 0}}  
    },
    // Dígito 1
    {
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 110}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 110}, {0, 0, 110}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 110}, {0, 0, 0}, {0, 0, 110}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 110}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}}
    },
    // Dígito 2
    {
        {{0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 110}},
        {{0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}},
        {{0, 0, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}} 
    },
    // Dígito 3
    {
        {{0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 110}},
        {{0, 0, 0}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}},   
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 110}},
        {{0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}} 
    },
    // Dígito 4
    {
        {{0, 0, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 110}},
        {{0, 0, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 110}},
        {{0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 110}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 110}}
    },
    // Dígito 5
    {
        {{0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}},
        {{0, 0, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 110}},
        {{0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}} 
    },
    // Dígito 6
    {
        {{0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 0}},   
        {{0, 0, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}},
        {{0, 0, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 110}},
        {{0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}} 
    },
    // Dígito 7
    {
        {{0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 110}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 110}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}
    },
    // Dígito 8
    {
        {{0, 0, 0}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 0}},
        {{0, 0, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 110}},
        {{0, 0, 0}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 0}},
        {{0, 0, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 110}},
        {{0, 0, 0}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 0}}
    },
    // Dígito 9
    {
        {{0, 0, 0}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 0}},      
        {{0, 0, 110}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 110}},
        {{0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 110}},
        {{0, 0, 0}, {0, 0, 110}, {0, 0, 110}, {0, 0, 110}, {0, 0, 0}}       
    },
    // Inicializar zerado
    {
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},      
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}       
    }

};

void npSetLED(uint index, uint8_t r, uint8_t g, uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

void npClear() {
   digit = 10;
   npDisplayDigit(digit);
}

// Inicialização da matriz de LEDs
void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, true);
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
    npClear();
}

void npWrite() {
    for (uint i = 0; i < LED_COUNT; i++) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100);
}

int getIndex(int x, int y) {
    if (y % 2 == 0) {
        return 24 - (y * 5 + x);
    } else {
        return 24 - (y * 5 + (4 - x));
    }
}

int main() {
    stdio_init_all();
    npInit(LED_PIN);
    // Inicialização do i2c
    i2c_init(I2C_PORT, 400 * 5000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    ssd1306_t ssd;
    ssd1306_init(&ssd, DISPLAY_WIDTH, DISPLAY_HEIGHT, false, 0x3c, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    // Limpa o display
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Inicializa o ADC
    adc_init();
    adc_gpio_init(EIXO_Y);
    adc_gpio_init(EIXO_X);

    // Configura os LEDs
    gpio_set_function(LED2_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED3_PIN, GPIO_FUNC_PWM);
    gpio_init(LED1_PIN);
    gpio_set_dir(LED1_PIN, GPIO_OUT);
    gpio_put(LED1_PIN, 0);

    //Configura buzzer
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);

    // Configura PWM
    uint slice1 = pwm_gpio_to_slice_num(LED2_PIN);
    uint slice2 = pwm_gpio_to_slice_num(LED3_PIN);
    uint slice3 = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_wrap(slice1, PWM_WRAP);
    pwm_set_wrap(slice2, PWM_WRAP);
    pwm_set_wrap(slice3, 999);
    pwm_set_clkdiv(slice1, 2.0f);
    pwm_set_clkdiv(slice2, 2.0f);
    pwm_set_clkdiv(slice3, 125.0f);
    pwm_set_enabled(slice1, true);
    pwm_set_enabled(slice2, true);
    pwm_set_enabled(slice3, true);
    
    // Configura botões
    gpio_init(JOYSTICK_BUTTON);
    gpio_set_dir(JOYSTICK_BUTTON, GPIO_IN);
    gpio_pull_up(JOYSTICK_BUTTON);
    gpio_init(BUTTONA_PIN);
    gpio_set_dir(BUTTONA_PIN, GPIO_IN);
    gpio_pull_up(BUTTONA_PIN);
    gpio_init(BUTTONB_PIN);
    gpio_set_dir(BUTTONB_PIN, GPIO_IN);
    gpio_pull_up(BUTTONB_PIN);
    gpio_set_irq_enabled(BUTTONA_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BUTTONB_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(JOYSTICK_BUTTON, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_callback(gpio_callback);
    irq_set_enabled(IO_IRQ_BANK0, true);

    // Repetição serial do estado do display
    struct repeating_timer timer;
    add_repeating_timer_ms(2000, repeating_timer_callback, NULL, &timer);

    while (1) {
        leitura_e_controle_joystick(slice1, slice2);
        cor = !cor;
        
        // Limpa e redesenha o display
        ssd1306_fill(&ssd, false);
        if (borda_style){
            ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor);  // borda variável
        }
        else{
            ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);  // borda fixa
        }
        ssd1306_rect(&ssd, pos_y, pos_x, SQUARE_SIZE, SQUARE_SIZE, true, true);  // Quadrado
        ssd1306_send_data(&ssd);
    
        // Chama as funções de verificação e exibição
        verificacao_extremidade(pos_x, pos_y);
        npDisplayDigit(digit);
        
        sleep_ms(30);
    }
}

// Funções de callback e controle permanecem as mesmas
void gpio_callback(uint gpio, uint32_t events) {
    absolute_time_t now = get_absolute_time();
    int64_t diff = absolute_time_diff_us(last_interrupt_time, now);

    if (diff < 250000) return;
    last_interrupt_time = now;

    if (gpio == BUTTONA_PIN) {
        pwm_enabled = !pwm_enabled;
    } 
    else if (gpio == BUTTONB_PIN){
        buzzer_enable = !buzzer_enable;
    }
    else if (gpio == JOYSTICK_BUTTON) {
        green_value = !green_value;
        gpio_put(LED1_PIN, green_value);
        borda_style = !borda_style;

    }
    
}

void leitura_e_controle_joystick(uint slice1, uint slice2) {
    const uint16_t CENTER = 2047;
    const uint16_t DEADZONE = 170;  // Valor empírico da deadzone do joystick

    adc_select_input(0);  // Verifica o canal 1
    y_value = adc_read();
    
    adc_select_input(1);  // Verifica o canal 2
    x_value = adc_read();

    int16_t y_diff = (int16_t)y_value - CENTER;
    int16_t x_diff = (int16_t)x_value - CENTER;

    // Aciona o buzzer em valores de alta variação
    //buzzer_control(x_value, y_value);

    // Corrigindo o movimento no eixo X (movimento horizontal)
    if (abs(x_diff) > DEADZONE) {
        pos_x += (x_diff > 0) ? SPEED : -SPEED;
        pos_x = (pos_x < 0) ? 0 : (pos_x > MAX_X) ? MAX_X : pos_x;
    }
    
    // Corrigindo o movimento no eixo Y (movimento vertical)
    if (abs(y_diff) > DEADZONE) {
        pos_y += (y_diff > 0) ? -SPEED : SPEED;  
        pos_y = (pos_y < 0) ? 0 : (pos_y > MAX_Y) ? MAX_Y : pos_y;
    }

    // Verificação do pwm em relação a deadzone
    uint16_t pwm_y = (abs(y_diff) <= DEADZONE) ? 0 : abs(y_diff) * 2;
    uint16_t pwm_x = (abs(x_diff) <= DEADZONE) ? 0 : abs(x_diff) * 2;

    if (pwm_enabled) {
        pwm_set_gpio_level(LED2_PIN, pwm_y);
        pwm_set_gpio_level(LED3_PIN, pwm_x);
    }
}

void npDisplayDigit(int digit) {
    for (int coluna = 0; coluna < 5; coluna++) {
        for (int linha = 0; linha < 5; linha++) {
            int posicao = getIndex(linha, coluna);
            npSetLED(
                posicao,
                digits[digit][coluna][linha][0],  // R
                digits[digit][coluna][linha][1],  // G
                digits[digit][coluna][linha][2]   // B+
            );
        }
    }
    npWrite();
}

bool repeating_timer_callback(struct repeating_timer *t)
{
  printf("###############################\n");
  printf("Valor da posição x: %d\n", pos_x);
  printf("Valor da posição y: %d\n", pos_y);
  printf("Valor da velocidade x: %d\n", x_value);
  printf("Valor da velocidade y: %d\n", y_value);

  return true;

}

void verificacao_extremidade(int pos_x, int pos_y){

    // Configurando o PWM do buzzer que será utilizado para indicar a posição do quadrado
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint channel = pwm_gpio_to_channel(BUZZER_PIN);

    //Flag de máximo e mínimo do display
    bool maximo = false;

    if (pos_x < 2 && pos_y < 2){
        digit = 1;
        maximo = true;
    }
    else if(pos_x > 118 && pos_y < 2)
    {
        digit = 2;
        maximo = true;
    }
    else if(pos_x < 2 && pos_y > 55)
    {
        digit = 3;
        maximo = true;
    }
    else if(pos_x > 118 && pos_y > 55)
    {
        digit = 4;
        maximo = true;
    }
    else{
        digit = 10;
        maximo = false;   
    }

    if (maximo && buzzer_enable) {
        pwm_set_chan_level(slice_num, channel, 500); 
    } else {
        pwm_set_chan_level(slice_num, channel, 0);
    }
}