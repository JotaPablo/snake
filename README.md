# BitSnake - O Jogo da Cobrinha na BitDogLab

Uma versão do clássico jogo da cobrinha (Snake) utilizando a plataforma BitDogLab, com múltiplos periféricos integrados para movimentação, feedback visual e sonoro, e interface de usuário.

## Vídeo Demonstrativo
[![Assista ao vídeo no YouTube](https://img.youtube.com/vi/JN6O4QZe8Xc/hqdefault.jpg)](https://youtu.be/JN6O4QZe8Xc)

## Funcionalidades Principais

### Modo Menu
- Exibição do menu de seleção de dificuldade (Fase 1, 2 ou 3) na matriz de LEDs.
- Navegação pelo menu via joystick.
- Botão **B** utilizado para confirmar a seleção.
- Display OLED mostra:
  - Instruções de uso do botão **B**.
  - Movimentação de um quadrado 8x8 conforme o joystick (requisito obrigatório).
  Buzzer: Sons ao mudar de fase e apertar o botão **B**.

### Modo Jogo
- Jogo da cobrinha exibido na matriz de LEDs.
- Controle da cobra pelo joystick.
- Maçãs geradas aleatoriamente.
- Display OLED mostra:
  - Progresso das maçãs coletadas em relação ao total.
  - Movimentação contínua do quadrado 8x8 baseado no joystick.
  - Instrução sobre o botão A
  - Ao ganhar ou perder, aparece uma mensagem.
- Feedbacks:
  - **LED RGB:** Verde para vitória, vermelho para derrota.
  - **Buzzer:** Sons diferentes para coleta de maçã, mudança de fase, vitória e derrota.
- Botão **A** permite retornar ao menu principal.

**Botão **Joystick**: Entra em Modo BOOTSEL**


## Componentes Utilizados

| Componente            | GPIO/Pinos                     | Função                                              |
|------------------------|---------------------------------|-----------------------------------------------------|
| Display OLED SSD1306   | 14(SDA) e 15(SCL)                           | Exibição de instruções, progresso e movimento       |
| Matriz de LEDs         | 7              | Exibição do menu de fases e do jogo da cobrinha     |
| LED RGB                | 11(Verde), 12(Azul) e 13(Vermelho)               | Feedback visual de vitória ou derrota               |
| Buzzer                 | 21       | Feedback sonoro                                     |
| Joystick               | Eixos X(27) e Y(26)  | Controle da movimentação e navegação                |
| Botão A                | 5                | Voltar ao menu                                      |
| Botão B                | 6                 | Selecionar fase no menu                             |
| Botão Joystick         | 22                 | Entrar em Modo BOOTSEL |

- **Periféricos Adicionais:**
  - PWM para controle do Buzzer
  - ADC para leitura analógica do joystick
  - Interrupções e debounce para captação confiável dos botões


## ⚙️ Instalação e Uso

1. **Pré-requisitos**
   - Clonar o repositório:
     ```bash
     git clone https://github.com/JotaPablo/snake.git
     cd snake
     ```
   - Instalar o **Visual Studio Code** com as seguintes extensões:
     - **Raspberry Pi Pico SDK**
     - **Compilador ARM GCC**

2. **Compilação**
   - Compile o projeto no terminal:
     ```bash
     mkdir build
     cd build
     cmake ..
     make
     ```
   - Ou utilize a extensão da Raspberry Pi Pico no VS Code.

3. **Execução**
   - **Na placa física:** 
     - Conecte a placa ao computador em modo **BOOTSEL**.
     - Copie o arquivo `.uf2` gerado na pasta `build` para o dispositivo identificado como `RPI-RP2`, ou envie através da extensão da Raspberry Pi Pico no VS Code.
