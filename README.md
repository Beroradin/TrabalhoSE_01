# ✨ Trabalho 01 - Identificador de máximos e mínimos em um espaço bidimensional

<p align="center"> Repositório dedicado ao Trabalho 01 - SE do processo de capacitação do EmbarcaTech que envolve a implementação de um sistema embarcado coom diversas aplicações de revisão na placa Raspberry Pi Pico W por meio da Plataforma BitDogLab.</p>

## :clipboard: Apresentação da tarefa

Para revisar os tópicos abordados durante a primeira fase da capacitação em sistemas embarcados, esta atividade propõe a implementação de um projeto prático envolvendo a utilização do joystick (ligado ao canal 1 e 2 do ADC do RP2040) para o controle do LED RGB (PWM utilizado) como também de um quadrado no display OLED, caso o quadrado chegue nos pontos máximos/mínimos da tela, será emitida uma indicação audio-visual da matriz de LEDs e do Buzzer. O projeto também prevê a utilização dos botões por meio de interrupções (com seus devidos Debounce por software) para a alteração de alguns parâmetros do sistema. A depuração e manutenção serial se deve graças a interface UART, que por meio de um timer, periodicamente emite dados sobre o joystick e o quadrado 8x8.

## :dart: Objetivos

- Compreender o funcionamento e a aplicação de conversores analógico-digital em microcontroladores;

- Compreender o funcionamento e a aplicação de modulação por largura de pulso (PWM) em microcontroladores;

- Compreender o funcionamento e a aplicação de interrupções em microcontroladores;

- Implementar a técnica de debouncing via software para eliminar o efeito de bouncing em botões;

- Controlar o display OLED e o LED RGB por meio do valor ADC do Joystick;

- Manipular o display OLED por meio do botão do Joystick;

- Utilização do PIO para o controle da matriz de LEDs;

- Permitir habilitar/desabilitar o valor do LED RGB por meio do botão na GPIO 5;

- Permitir habilitar/desabilitar o buzzer por meio do botão na GPIO 6;

- Depuração de dados e parâmetros por meio da interface serial UART.

## :books: Descrição do Projeto

Utiizou-se a placa BitDogLab (que possui o microcontrolador RP2040) para a exibição no display OLED ssd1306 um quadrado 8x8 que é controlado por meio do conversor ADC ligado ao joystick. O LED RGB também é controlado pelo ADC, mas passa pelo periférico PWM para seu eventual acionamento e controle efetivo. Os botões também controlam aspectos (flags) do projeto. Também é utilizado a matriz de LEDs, buzzer e a interface serial UART.

## :walking: Integrantes do Projeto

- Matheus Pereira Alves

## :bookmark_tabs: Funcionamento do Projeto

- O display OLED possui um quadrado 8x8 que é controlado pelo joystick (conversor analógico-digital nos eixos X e Y);
- O LED RGB (cores vermelhas e azuis) são controladas pela iteração ADC-PWM, tendo seus máximos nos extremos (valor wrap 0 - 4095);
- O LED RGB verde é ligado por meio do botão do joystick (GPIO 22), que também aciona uma borda variável;
- O botão A (GPIO 5) controla o pwm, habilitando e desabilitando a sua mudança de valor de duty cicle;
- O PWM possui uma verificação de deadzone, que é uma área em qual o valor do pwm é setado para 0 e é próximo do valor adc de 2047;
- Houve a implementação de debounce e interrupções para os botões;
- A matriz de LEDs indica o máximo/mínimo que o quadrado 8x8 alcançou;
- A interface serial UART é utilizada para depuração de parâmetros do sistema embarcado;
- O buzzer é utilizado como um aspecto visual-auditivo auxiliar.

## :camera: GIF mostrando o funcionamento do programa na placa BitDogLab
<p align="center">
  <img src="images/trabalho01.gif" alt="GIF" width="526px" />
</p>

## :arrow_forward: Vídeo no youtube mostrando o funcionamento do programa na placa BitDogLab

<p align="center">
    <a href="https://youtu.be/h9pF9yb3Rns">Clique aqui para acessar o vídeo</a>
</p>
