#include <stdio.h>
#include "pico/stdlib.h"
#include <stdlib.h>
#include <string.h>
#include "hardware/timer.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "lib/neopixel.h"
#include "lib/ssd1306.h"
#include "lib/buzzer.h"


// Definição dos pinos dos LEDs RGB
#define RED_PIN 13
#define GREEN_PIN 11
#define BLUE_PIN 12

// Definições de botões
#define BUTTON_A 5
#define BUTTON_B 6
#define BUTTON_JOYSTICK 22

//Definição do pino do Buzzer
#define BUZZER_PIN 21

//Definição das variaveis do joystick
#define VRX_PIN 27  
#define VRY_PIN 26
#define ADC_MAX 4095
#define CENTRO 2047
#define DEADZONE 250  // Zona morta de 250 ao redor do centro (2047)

static volatile uint16_t vrx_valor;
static volatile uint16_t vry_valor;
static volatile bool joystick_mudou = false;

void ler_joystick();
uint16_t aplicar_deadzone(uint16_t valor_adc);

// Variáveis para debounce dos botões (armazenam tempo do último acionamento)
static volatile uint32_t last_time_button_a = 0;
static volatile uint32_t last_time_button_b = 0;

// Protótipos das funções de tratamento de interrupção
static void gpio_button_a_handler(uint gpio, uint32_t events);
static void gpio_button_b_handler(uint gpio, uint32_t events);
static void gpio_button_joystick_handler(uint gpio, uint32_t events);
static void gpio_button_handler(uint gpio, uint32_t events);

static volatile bool botao_a_pressionado = false;
static volatile bool botao_b_pressionado = false;

// Estrutura e funções do display OLED

#define LARGURA_BORDA 40
#define ALTURA_BORDA 40
static ssd1306_t ssd; // Estrutura de controle do display
char maca_texto[20];  // Pode armazenar a string de atualiza_display
void display_init();  // Inicialização do hardware
void atualiza_display();
void desenha_quadrado(uint8_t center_x, uint8_t center_y, uint8_t tamanho);
void desenha_borda(uint8_t centro_x, uint8_t centro_y);


// Definições de direções
#define DIREITA 0
#define ESQUERDA 1
#define CIMA 2
#define BAIXO 3

// Estrutura para coordenadas
typedef struct {
    uint8_t x, y;
} Coordenada;

typedef enum {
    FACIL,
    MEDIO,
    DIFICIL,
    NUM_FASES
} Fase;

typedef struct {
    uint8_t num_obstaculos;
    uint32_t velocidade;
    Coordenada obstaculos[6];
    Coordenada inicio_snake[3];
} ConfiguracaoFase;

static const ConfiguracaoFase fases[NUM_FASES] = {
    [FACIL] = {
        .num_obstaculos = 0,
        .velocidade = 1000,
        .obstaculos = {},
        .inicio_snake = {{2,2}, {1,2}, {0,2}}
    },
    [MEDIO] = {
        .num_obstaculos = 3,
        .velocidade = 800,
        .obstaculos = {{2,1}, {2,2}, {2,3}},
        .inicio_snake = {{2,0}, {1,0}, {0,0}}
    },
    [DIFICIL] = {
        .num_obstaculos = 6,
        .velocidade = 600,
        .obstaculos = {{1,1}, {1,2}, {1,3}, {3,1}, {3,2}, {3,3}},
        .inicio_snake = {{2,4}, {1,4}, {0,4}}
    }
};

// Definições gerais
#define TAM_MAX 10
#define TAM_MINIMO 3
#define LIMITE_MACAS 5

#define ESTADO_MENU 0
#define ESTADO_JOGO 1
#define ESTADO_FIM_JOGO 2
#define ESTADO_RESULTADO 3

// Variáveis de estado do jogo
static volatile uint8_t ESTADO = ESTADO_MENU;
static volatile uint32_t TAM_ATUAL = TAM_MINIMO;
static volatile uint8_t DIRECAO_ATUAL = DIREITA;
static volatile uint8_t DIRECAO_ANT = DIREITA;
static Coordenada Posicoes[TAM_MAX];
static Coordenada MACA;
static volatile uint8_t MACAS_COLETADAS = 0;
static volatile bool ERROR = false;
static Fase fase_atual = DIFICIL;


// Protótipos de funções
void setup();
void muda_direcao();
void inicializa_snake();
void atualiza_snake();
void desenha_snake();
void gerar_maca();
bool verifica_colisao();
void set_cor_matriz(uint8_t r, uint8_t g, uint8_t b);
Fase show_menu();
void drawNumberWithBackground(uint32_t numero, 
                             uint8_t num_r, uint8_t num_g, uint8_t num_b,
                             uint8_t bg_r, uint8_t bg_g, uint8_t bg_b);



int main() {
    
    setup();

    while(true) {

        ESTADO = ESTADO_MENU;
        atualiza_display();

        //Seleciona a fase
        fase_atual = show_menu();

        // Inicialização do jogo para a fase atual
        DIRECAO_ATUAL = DIREITA;
        TAM_ATUAL = TAM_MINIMO;
        MACAS_COLETADAS = 0;
        ERROR = false;
        
        ESTADO = ESTADO_JOGO;
        inicializa_snake();
        atualiza_display();

        uint32_t ultima_atualizacao = 0;
        const uint32_t intervalo = fases[fase_atual].velocidade;
        
        sleep_ms(intervalo);


        while(true) {
            buzzer_update();
            uint32_t agora = to_ms_since_boot(get_absolute_time());
            
            ler_joystick();
            if(joystick_mudou) atualiza_display();
            muda_direcao();
            
            if(agora - ultima_atualizacao >= intervalo) {
                DIRECAO_ANT = DIRECAO_ATUAL;
                atualiza_snake();

                if(ERROR || MACAS_COLETADAS >= LIMITE_MACAS || botao_a_pressionado) break;

                desenha_snake();

                ultima_atualizacao = agora;
            }
            
            sleep_ms(50);
        }
        
        if(!botao_a_pressionado){
        
            ESTADO = ESTADO_FIM_JOGO;
            atualiza_display(); // Atualiza o display de acordo com o resultado

            // Tela de resultado
            if(ERROR){

                printf("\nVocê Perdeu!!!\n");

                // Move cabeça para a posição correta novamente
                switch(DIRECAO_ATUAL) {
                    case DIREITA: Posicoes[0].x--; break;
                    case ESQUERDA: Posicoes[0].x++; break;
                    case CIMA:    Posicoes[0].y--; break;
                    case BAIXO:   Posicoes[0].y++; break;
                }

                desenha_snake();

                //Pisca o LED RBG Vermelho e a Cabeçe da Snake, além do feedback sonoro
                npSetLED(getIndex(Posicoes[0].x, Posicoes[0].y), 0, 0, 0);
                npWrite();
                gpio_put(RED_PIN, true);
                buzzer_start(1000, 100);
                sleep_ms(300);
                
                npSetLED(getIndex(Posicoes[0].x, Posicoes[0].y), 0, 10, 0);
                npWrite();
                gpio_put(RED_PIN, false);               
                buzzer_stop();
                sleep_ms(300);

                npSetLED(getIndex(Posicoes[0].x, Posicoes[0].y), 0, 0, 0);
                npWrite();
                gpio_put(RED_PIN, true);
                buzzer_start(1000, 100);
                sleep_ms(300);

                npSetLED(getIndex(Posicoes[0].x, Posicoes[0].y), 0, 10, 0);
                npWrite();
                gpio_put(RED_PIN, false);
                buzzer_stop();
                sleep_ms(300);
            }
            //Caso tenha coletado as maçãs
            else{

                printf("\nVocê Ganhou!!!\n");

                //Pisca o LED RBG Verde e a Maca, além do feedback sonoro
                npSetLED(getIndex(MACA.x, MACA.y), 0, 0, 0);
                npWrite();
                gpio_put(GREEN_PIN, true);
                buzzer_start(400, 100);
                sleep_ms(300);

                npSetLED(getIndex(MACA.x, MACA.y), 10, 0, 0);
                npWrite();
                gpio_put(GREEN_PIN, false);
                buzzer_stop();
                sleep_ms(300);

                npSetLED(getIndex(MACA.x, MACA.y), 0, 0, 0);
                npWrite();
                gpio_put(GREEN_PIN, true);
                buzzer_start(400, 100);
                sleep_ms(300);

                npSetLED(getIndex(MACA.x, MACA.y), 10, 0, 0);
                npWrite();
                gpio_put(GREEN_PIN, false);
                buzzer_stop();
                sleep_ms(300);
            }
            
            sleep_ms(3000);
        }
        
        else {
            buzzer_start(1000, 100);
            botao_a_pressionado = false;
        }

        printf("\nVoltando para o menu!\n");
    }
}

void setup() {
    stdio_init_all();

    adc_init();
    adc_gpio_init(VRY_PIN);  // Eixo Y
    adc_gpio_init(VRX_PIN);  // Eixo X

    npInit(LED_PIN);

    display_init();

    buzzer_init(BUZZER_PIN);

    // Configuração dos LEDs
    gpio_init(RED_PIN);
    gpio_set_dir(RED_PIN, GPIO_OUT);
    gpio_init(GREEN_PIN);
    gpio_set_dir(GREEN_PIN, GPIO_OUT);
    gpio_init(BLUE_PIN);
    gpio_set_dir(BLUE_PIN, GPIO_OUT);

    //Configuração dos Botões
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    gpio_init(BUTTON_JOYSTICK);
    gpio_set_dir(BUTTON_JOYSTICK, GPIO_IN);
    gpio_pull_up(BUTTON_JOYSTICK);

     // Configura interrupções para borda de descida (botão pressionado)
     gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_button_handler);
     gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_button_handler);
     gpio_set_irq_enabled_with_callback(BUTTON_JOYSTICK, GPIO_IRQ_EDGE_FALL, true, &gpio_button_handler);
}

void display_init() {
    // Configuração I2C a 400kHz
    i2c_init(I2C_PORT, 400 * 1000);
    
    // Configura pinos I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA); // Ativa pull-ups internos
    gpio_pull_up(I2C_SCL);

    // Inicialização do controlador SSD1306
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    // Limpa a tela inicialmente
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

void atualiza_display(){
    ssd1306_fill(&ssd, false);  // Limpa a tela
    
    switch(ESTADO)
    {
        case ESTADO_MENU:
            ssd1306_draw_string(&ssd, " APERTE B", 0, 12);
            ssd1306_draw_string(&ssd, "   PARA", 0, 25);
            ssd1306_draw_string(&ssd, "SELECIONAR", 0, 38);

            desenha_quadrado(108, 32, 8);
            desenha_borda(108,32);
            break;

        case ESTADO_JOGO:
            sprintf(maca_texto, "MACA:%d de %d", MACAS_COLETADAS, LIMITE_MACAS);

            ssd1306_draw_string(&ssd, "APERTE A", 0, 12);
            ssd1306_draw_string(&ssd, "PARA SAIR", 0, 25);
            ssd1306_draw_string(&ssd, maca_texto, 0, 40);

            desenha_quadrado(108, 32, 8);
            desenha_borda(108,32);
            break;
        case ESTADO_FIM_JOGO:
            if(ERROR) ssd1306_draw_string(&ssd, "VOCE PERDEU!!!", 8, 25);
            else ssd1306_draw_string(&ssd, "VOCE GANHOU!!!", 8, 25);
            ssd1306_draw_string(&ssd, "AGUARDE...", 24, 38);
            
    }
    
    ssd1306_send_data(&ssd); // Envia buffer para o display
}

uint16_t aplicar_deadzone(uint16_t valor_adc) {
    if (valor_adc >= (CENTRO - DEADZONE) && valor_adc <= (CENTRO + DEADZONE)) {
        return CENTRO;  // Define o valor como centro se estiver dentro da zona morta
    }
    return valor_adc;  // Retorna o valor original se estiver fora da zona morta
}

void ler_joystick() {
    //Guarda os valores antes da leitura para verificarmos se houve movimentação do joystick
    uint16_t vrx_ant = vrx_valor;
    uint16_t vry_ant = vry_valor;

    adc_select_input(1); // Seleciona o canal para o eixo X
    vrx_valor = adc_read();
    vrx_valor = aplicar_deadzone(vrx_valor);  // Aplica deadzone no eixo X

    adc_select_input(0); // Seleciona o canal para o eixo Y
    vry_valor = adc_read();
    vry_valor = aplicar_deadzone(vry_valor);  // Aplica deadzone no eixo Y

    if(vrx_ant != vrx_valor || vry_ant != vrx_valor){
        joystick_mudou = true;
        printf("\n===Leitura do Joystick===\n");
        printf("VRX: %d  VRY: %d\n\n", vrx_valor, vry_valor);
    }
    else joystick_mudou = false;
}

void muda_direcao(){

    //Para controlar melhor as mudanças de direção
    uint16_t limiar = 250;

    uint8_t NOVA_DIRECAO = DIRECAO_ATUAL;

    //Se a direção é cima ou baixo, ela só pode mudar para direita ou esquerda
    if (DIRECAO_ANT == CIMA || DIRECAO_ANT == BAIXO){
        if (vrx_valor > CENTRO + limiar && DIRECAO_ANT!= ESQUERDA) NOVA_DIRECAO = DIREITA;
        else if (vrx_valor < CENTRO - limiar && DIRECAO_ANT != DIREITA) NOVA_DIRECAO = ESQUERDA;
    }
    //Se não, a direção muda para cima ou para baixo
    else{
        if (vry_valor > CENTRO + limiar && DIRECAO_ANT != BAIXO) NOVA_DIRECAO = CIMA;
        else if (vry_valor < CENTRO - limiar  && DIRECAO_ANT != CIMA) NOVA_DIRECAO = BAIXO;
    }

    if(NOVA_DIRECAO != DIRECAO_ATUAL){
        switch(NOVA_DIRECAO){
            case CIMA:
                printf("NOVA DIRECAO: CIMA\n");
                break;
            case BAIXO:
                printf("NOVA DIRECAO: BAIXO\n");
                break;
            case DIREITA:
                printf("NOVA DIRECAO: DIREITA\n");
                break;
            case ESQUERDA:
                printf("NOVA DIRECAO: ESQUERDA\n");
                break;

        }
    }
    DIRECAO_ATUAL = NOVA_DIRECAO;
}

void gerar_maca() {
    do {
        //Gera uma Coordenada aleatória para maçã
        MACA.x = rand() % 5;
        MACA.y = rand() % 5;
        bool valido = true;

        // Verifica colisão com cobra
        for(int i = 0; i < TAM_ATUAL; i++) {
            if(Posicoes[i].x == MACA.x && Posicoes[i].y == MACA.y) {
                valido = false;
                break;
            }
        }

        // Verifica colisão com obstáculos
        for(int i = 0; i < fases[fase_atual].num_obstaculos; i++) {
            if(fases[fase_atual].obstaculos[i].x == MACA.x && 
               fases[fase_atual].obstaculos[i].y == MACA.y) {
                valido = false;
                break;
            }
        }

        if(valido) break;
    } while(true);
}

void inicializa_snake() {
    //Inicializa na fase atual
    for(int i = 0; i < 3; i++){
        Posicoes[i] = fases[fase_atual].inicio_snake[i];
    }
    gerar_maca();
    npClear();
    desenha_snake();
}

void atualiza_snake() {
    // Atualiza posições do corpo
    for(int i = TAM_ATUAL; i > 0; i--) {
        Posicoes[i] = Posicoes[i - 1];
    }

    // Move cabeça
    switch(DIRECAO_ATUAL) {
        case DIREITA: Posicoes[0].x++; break;
        case ESQUERDA: Posicoes[0].x--; break;
        case CIMA:    Posicoes[0].y++; break;
        case BAIXO:   Posicoes[0].y--; break;
    }

    // Verifica colisões
    if(Posicoes[0].x > 4 || Posicoes[0].x < 0 || 
       Posicoes[0].y > 4 || Posicoes[0].y < 0 || 
       verifica_colisao()) {
        
        ERROR = true;
        return;
    }

    // Verifica maçã
    if(Posicoes[0].x == MACA.x && Posicoes[0].y == MACA.y) {
        if(TAM_ATUAL < TAM_MAX) TAM_ATUAL++; // Aumenta o tamanho da snake
        
        //Atualiza o numero de maça coletadas e o display        
        MACAS_COLETADAS++;
        atualiza_display(); 

        if(MACAS_COLETADAS < LIMITE_MACAS) gerar_maca(); // Gera uma nova maça

        buzzer_start(400, 200); // Faz um barulhinho

        printf("Maça Coletada\n");
        printf("Maca: %d de %d\n", MACAS_COLETADAS, LIMITE_MACAS);
    }
}

void desenha_snake() {

    if(ERROR || MACAS_COLETADAS >= LIMITE_MACAS) return;

    npClear();
    
    // Desenha obstáculos
    for(int i = 0; i < fases[fase_atual].num_obstaculos; i++) {
        npSetLED(getIndex(fases[fase_atual].obstaculos[i].x, 
                        fases[fase_atual].obstaculos[i].y), 1, 1, 1);
    }
    
    // Desenha cobra
    npSetLED(getIndex(Posicoes[0].x, Posicoes[0].y), 0, 10, 0);
    for(int i = 1; i < TAM_ATUAL; i++) {
        npSetLED(getIndex(Posicoes[i].x, Posicoes[i].y), 2, 2, 0);
    }
    
    // Desenha maçã
    npSetLED(getIndex(MACA.x, MACA.y), 10, 0, 0);
    
    npWrite();
}

bool verifica_colisao() {
    // Colisão com corpo
    for(int i = 1; i < TAM_ATUAL; i++) {
        if(Posicoes[0].x == Posicoes[i].x && Posicoes[0].y == Posicoes[i].y)
            return true;
    }
    
    // Colisão com obstáculos
    for(int i = 0; i < fases[fase_atual].num_obstaculos; i++) {
        if(Posicoes[0].x == fases[fase_atual].obstaculos[i].x && 
           Posicoes[0].y == fases[fase_atual].obstaculos[i].y)
            return true;
    }
    
    return false;
}

void set_cor_matriz(uint8_t r, uint8_t g, uint8_t b) {
    npClear();
    for(int i = 0; i < 25; i++) {
        npSetLED(i, r, g, b);
    }
    npWrite();
}

// Função do menu
Fase show_menu() {
    Fase fase_menu = FACIL;
    bool selecionado = false;
    bool atualiza_matriz_led = true;
    uint32_t current_time;
    uint32_t last_time_joystick = 0;
    while(!selecionado) {

        buzzer_update();
        
        current_time = to_us_since_boot(get_absolute_time());

        // Define cores baseadas na dificuldade
        if(atualiza_matriz_led){
            printf("***FASE Atual: %d***\n", fase_menu+1);
            switch(fase_menu) {
                case FACIL:
                    drawNumberWithBackground(1,  // Número
                                            1, 1, 1,  // Branco (número)
                                            0, 0, 0);     // Verde (fundo)
                    break;
                
                case MEDIO:
                    drawNumberWithBackground(2, 
                                            255, 165, 0,  // Branco
                                            0, 0, 0);   // Laranja
                    break;
                
                case DIFICIL:
                    drawNumberWithBackground(3,
                                            1, 0, 0,  // Branco
                                            0, 0, 0);     // Vermelho
                    break;
            }
            atualiza_matriz_led = false;
        }

        // Controle do menu

        ler_joystick();

        //Sempre que o joystick alterar o valor, atualiza o display
        //Mas cada mudança precisa ter um intervalo de 200 ms para mudar o número da fase
        if(joystick_mudou){

            atualiza_display();

            if(vrx_valor > CENTRO && current_time - last_time_joystick> 200000) {
                fase_menu = (fase_menu + 1) % NUM_FASES;
                atualiza_matriz_led = true;
                last_time_joystick = current_time;
                buzzer_start(1000, 100);
            }
            else if(vrx_valor < CENTRO && current_time - last_time_joystick> 200000){
                fase_menu = ((fase_menu - 1) + NUM_FASES) % NUM_FASES;
                atualiza_matriz_led = true;
                last_time_joystick = current_time;
                buzzer_start(1000, 100);
            }
        }

        //Se botão b foi pressionado, seleciona a fase
        if(botao_b_pressionado) {
            botao_b_pressionado = false;
            selecionado = true;
            buzzer_start(1200, 100);
            sleep_ms(100);
            buzzer_stop();
        }

        sleep_ms(50);
    }
    return fase_menu;
    printf("\nEntrando na Fase %d\n", fase_menu+1);
}

void drawNumberWithBackground(uint32_t numero, 
                             uint8_t num_r, uint8_t num_g, uint8_t num_b,
                             uint8_t bg_r, uint8_t bg_g, uint8_t bg_b) {
    // Preenche todo o fundo primeiro
    for(int i = 0; i < 25; i++) {
        npSetLED(i, bg_r, bg_g, bg_b);
    }

    switch(numero){
        case 0:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(1,3), num_r,num_g,num_b);
            npSetLED(getIndex(3,3), num_r,num_g,num_b);
            npSetLED(getIndex(1,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(1,1), num_r,num_g,num_b);
            npSetLED(getIndex(3,1), num_r,num_g,num_b);
            npSetLED(getIndex(1,0), num_r,num_g,num_b);
            npSetLED(getIndex(2,0), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 1:
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(1,3), num_r,num_g,num_b);
            npSetLED(getIndex(2,3), num_r,num_g,num_b);
            npSetLED(getIndex(2,2), num_r,num_g,num_b);
            npSetLED(getIndex(2,1), num_r,num_g,num_b);
            npSetLED(getIndex(1,0), num_r,num_g,num_b);
            npSetLED(getIndex(2,0), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 2:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,3), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(2,2), num_r,num_g,num_b);
            npSetLED(getIndex(1,2), num_r,num_g,num_b);
            npSetLED(getIndex(1,1), num_r,num_g,num_b);
            npSetLED(getIndex(1,0), num_r,num_g,num_b);
            npSetLED(getIndex(2,0), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 3:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,3), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(2,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,1), num_r,num_g,num_b);
            npSetLED(getIndex(1,0), num_r,num_g,num_b);
            npSetLED(getIndex(2,0), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 4:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(1,3), num_r,num_g,num_b);
            npSetLED(getIndex(3,3), num_r,num_g,num_b);
            npSetLED(getIndex(1,2), num_r,num_g,num_b);
            npSetLED(getIndex(2,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,1), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 5:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(1,3), num_r,num_g,num_b);
            npSetLED(getIndex(1,2), num_r,num_g,num_b);
            npSetLED(getIndex(2,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,1), num_r,num_g,num_b);
            npSetLED(getIndex(1,0), num_r,num_g,num_b);
            npSetLED(getIndex(2,0), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 6:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(1,3), num_r,num_g,num_b);
            npSetLED(getIndex(1,2), num_r,num_g,num_b);
            npSetLED(getIndex(2,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(1,1), num_r,num_g,num_b);
            npSetLED(getIndex(3,1), num_r,num_g,num_b);
            npSetLED(getIndex(1,0), num_r,num_g,num_b);
            npSetLED(getIndex(2,0), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 7:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,3), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,1), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 8:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(1,3), num_r,num_g,num_b);
            npSetLED(getIndex(3,3), num_r,num_g,num_b);
            npSetLED(getIndex(1,2), num_r,num_g,num_b);
            npSetLED(getIndex(2,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(1,1), num_r,num_g,num_b);
            npSetLED(getIndex(3,1), num_r,num_g,num_b);
            npSetLED(getIndex(1,0), num_r,num_g,num_b);
            npSetLED(getIndex(2,0), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        case 9:
            npSetLED(getIndex(1,4), num_r,num_g,num_b);
            npSetLED(getIndex(2,4), num_r,num_g,num_b);
            npSetLED(getIndex(3,4), num_r,num_g,num_b);
            npSetLED(getIndex(1,3), num_r,num_g,num_b);
            npSetLED(getIndex(3,3), num_r,num_g,num_b);
            npSetLED(getIndex(1,2), num_r,num_g,num_b);
            npSetLED(getIndex(2,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,2), num_r,num_g,num_b);
            npSetLED(getIndex(3,1), num_r,num_g,num_b);
            npSetLED(getIndex(1,0), num_r,num_g,num_b);
            npSetLED(getIndex(2,0), num_r,num_g,num_b);
            npSetLED(getIndex(3,0), num_r,num_g,num_b);
            break;

        default: break;
    }
    npWrite();
}

void desenha_quadrado(uint8_t center_x, uint8_t center_y, uint8_t tamanho) {
    
    uint8_t borda_top = center_y - ALTURA_BORDA/2;
    uint8_t borda_left = center_x - LARGURA_BORDA/2;

    // Área útil da borda
    uint8_t area_util_largura = LARGURA_BORDA - tamanho; // tirando o tamanho do quadrado
    uint8_t area_util_altura = ALTURA_BORDA - tamanho;

    // Posição dentro da área útil
    uint pos_x = (vrx_valor/4095.0) * area_util_largura;
    uint pos_y = (vry_valor/4095.0) * area_util_altura;

    // Ajuste para o canto superior-esquerdo da borda
    pos_x += borda_left;
    pos_y = borda_top + (area_util_altura - pos_y); // Y invertido
    printf("\n====Posição Quadrado====\n");
    printf("Pos X: %d\n", pos_x);
    printf("Pos Y: %d\n\n", pos_y);

    // Desenha o quadrado 4x4 preenchido
    ssd1306_rect(&ssd, pos_y, pos_x, tamanho , tamanho, true, true);
}

void desenha_borda(uint8_t centro_x, uint8_t centro_y) {
    
    // Calcula onde começar (topo e esquerda) para centralizar a borda
    uint8_t left = centro_x - (LARGURA_BORDA / 2);
    uint8_t top = centro_y - (ALTURA_BORDA / 2);

    // Desenha a borda
    ssd1306_rect(&ssd, top, left, LARGURA_BORDA, LARGURA_BORDA, true, false);
}

// Tratador central de interrupções de botões
static void gpio_button_handler(uint gpio, uint32_t events) {
    switch(gpio) {
        case BUTTON_A:
            gpio_button_a_handler(gpio, events);
            break;
        case BUTTON_B:
            gpio_button_b_handler(gpio, events);
            break;
        case BUTTON_JOYSTICK:
            gpio_button_joystick_handler(gpio, events);
            break;
    }
}

// Tratador do botão A
static void gpio_button_a_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    
    // Debounce: verifica se passaram pelo menos 200ms desde o último acionamento
    if (current_time - last_time_button_a > 200000) {
        last_time_button_a = current_time;
        
        printf("Botão A pressionado\n");

        if(ESTADO == ESTADO_JOGO) botao_a_pressionado = true; // Só tem funcionalidade quando estamos no jogo
    }
}

// Tratador do botão B
static void gpio_button_b_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    
    // Debounce: verifica se passaram pelo menos 200ms desde o último acionamento
    if (current_time - last_time_button_b > 200000) {
        last_time_button_b = current_time;

        printf("Botão A pressionado\n");


        if(ESTADO == ESTADO_MENU) botao_b_pressionado = true; // Só tem funcionalidade no menu

    }
}
// Tratador do botão do joystick
static void gpio_button_joystick_handler(uint gpio, uint32_t events) {

    printf("\nHABILITANDO O MODO GRAVAÇÃO\n");

    // Adicionar feedback no display OLED
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "  HABILITANDO", 5, 25);
    ssd1306_draw_string(&ssd, " MODO GRAVACAO", 5, 38);
    ssd1306_send_data(&ssd);

    reset_usb_boot(0,0); // Reinicia no modo DFU
}