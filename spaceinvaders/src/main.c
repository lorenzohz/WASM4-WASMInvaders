/**
 * main.c - Jogo Space Invaders para o console de fantasia WASM-4.
 *
 * Este arquivo contém a lógica completa do jogo, incluindo:
 * - Movimento e tiro do jogador.
 * - Formação e movimento de alienígenas.
 * - Detecção de colisão.
 * - Sistema de pontuação.
 * - Fundo animado de estrelas com efeito de paralaxe.
 * - Paleta de cores customizada.
 * O código é autocontido e não depende de bibliotecas C padrão como stdio ou stdbool.
 */

#include "wasm4.h"

// --- Constantes para Lógica Booleana ---
// Usamos 1 e 0 para representar verdadeiro e falso, respectivamente.
#define TRUE 1
#define FALSE 0

// --- Sprites (Desenhos dos personagens) ---
// Cada linha representa 8 pixels. 1 é um pixel desenhado, 0 é transparente.

// Sprite do jogador (8x8 pixels)
const uint8_t player_sprite[] = {
    0b11100111, 
    0b11100111, 
    0b11100111, 
    0b11000011, 
    0b10000001, 
    0b10000001, 
    0b10011001, 
    0b10111101,
};

// Sprite do alienígena (8x8 pixels)
const uint8_t alien_sprite[] = {
    0b11000011, 
    0b10000001, 
    0b00100100, 
    0b00000000, 
    0b10000001, 
    0b11011011, 
    0b10111101, 
    0b11111111,
};


// --- Estruturas de Dados ---
// Agrupam variáveis relacionadas para cada elemento do jogo.

// Estrutura para as estrelas do fundo
typedef struct {
    int x, y;    // Posição na tela
    int speed;   // Velocidade de movimento para o efeito de paralaxe
} Star;

// Estrutura para o jogador
typedef struct {
    int x, y;    // Posição na tela
} Player;

// Estrutura para um projétil
typedef struct {
    int x, y;
    uint8_t active; // Estado do projétil (ativo ou inativo)
} Bullet;

// Estrutura para um alienígena
typedef struct {
    int x, y;
    uint8_t alive; // Estado do alienígena (vivo ou morto)
} Alien;


// --- Constantes do Jogo ---
#define PLAYER_SPEED 2         // Velocidade de movimento do jogador
#define BULLET_SPEED 4         // Velocidade do projétil
#define ALIEN_COLS 8           // Número de colunas de alienígenas
#define ALIEN_ROWS 3           // Número de linhas de alienígenas
#define TOTAL_ALIENS (ALIEN_COLS * ALIEN_ROWS) // Cálculo do total de alienígenas
#define STAR_COUNT 50          // Número de estrelas no fundo


// --- Variáveis Globais ---
// Variáveis que mantêm o estado do jogo durante a execução.
Player player;
Bullet player_bullet;
Alien aliens[TOTAL_ALIENS];
Star stars[STAR_COUNT];

int alien_direction = 1;     // Direção dos alienígenas (1=direita, -1=esquerda)
int alien_timer = 20;        // Timer para controlar a velocidade de movimento dos alienígenas
int score = 0;               // Pontuação do jogador
uint32_t random_seed = 1;    // Semente para o gerador de números aleatórios


// --- Funções Auxiliares ---

/**
 * Define a paleta de cores customizada do jogo.
 * As cores são definidas em formato hexadecimal RGB (0xRRGGBB).
 */
void set_palette() {
    PALETTE[0] = 0x191b1a; // Cor 1 (Fundo)
    PALETTE[1] = 0x294257; // Cor 2
    PALETTE[2] = 0x579c9a; // Cor 3
    PALETTE[3] = 0x99c9b3; // Cor 4 (Destaque)
}

/**
 * Gera um número inteiro pseudoaleatório dentro de um intervalo.
 * Usa um Gerador Linear Congruencial (LCG) simples.
 * @param min O valor mínimo do intervalo (inclusivo).
 * @param max O valor máximo do intervalo (inclusivo).
 * @return Um número inteiro aleatório.
 */
int random_int(int min, int max) {
    random_seed = (random_seed * 1103515245 + 12345) & 0x7fffffff;
    // CORREÇÃO: Casts explícitos para evitar erros de conversão de sinal.
    return min + (int)(random_seed % (uint32_t)(max - min + 1));
}

/**
 * Converte um número inteiro para uma string (implementação de itoa).
 * Necessário porque a biblioteca C padrão não está disponível.
 * @param n O número a ser convertido.
 * @param str O buffer de string onde o resultado será armazenado.
 */
void itoa(int n, char str[]) {
    int i = 0;
    if (n == 0) { str[i++] = '0'; str[i] = '\0'; return; }
    int is_negative = n < 0;
    if (is_negative) { n = -n; }
    while (n != 0) { str[i++] = (n % 10) + '0'; n = n / 10; }
    if (is_negative) { str[i++] = '-'; }
    str[i] = '\0';
    int start = 0, end = i - 1;
    while (start < end) { char temp = str[start]; str[start] = str[end]; str[end] = temp; start++; end--; }
}


// --- Funções de Inicialização (chamadas uma vez no início) ---

/**
 * Posiciona as estrelas do fundo em locais aleatórios na tela.
 */
void init_stars() {
    for (int i = 0; i < STAR_COUNT; ++i) {
        stars[i] = (Star){
            .x = random_int(0, 159),
            .y = random_int(0, 159),
            .speed = random_int(1, 3),
        };
    }
}

/**
 * Posiciona os alienígenas na formação inicial.
 */
void init_aliens() {
    for (int y = 0; y < ALIEN_ROWS; ++y) {
        for (int x = 0; x < ALIEN_COLS; ++x) {
            int index = y * ALIEN_COLS + x;
            aliens[index] = (Alien){ .x = 20 + x * 12, .y = 20 + y * 12, .alive = TRUE };
        }
    }
}

/**
 * Função principal de inicialização do jogo.
 * É chamada uma única vez quando o cartucho é carregado.
 */
void start() {
    set_palette();         // Define as cores
    init_aliens();         // Cria os alienígenas
    init_stars();          // Cria o fundo de estrelas

    // Posiciona o jogador
    player.x = 76;
    player.y = 140;

    // Garante que não há projétil na tela no início
    player_bullet.active = FALSE;
}


// --- Funções de Lógica e Desenho (chamadas a cada quadro) ---

/**
 * Atualiza e desenha o fundo de estrelas para criar a animação.
 */

int minimum(int a, int b) {
    return (a < b) ? a : b;
}

void update_background() {
    for (int i = 0; i < STAR_COUNT; ++i) {
        stars[i].y += minimum(stars[i].speed, 2); // Move a estrela para baixo

        if (stars[i].y > 160) { // Se a estrela saiu da tela
            stars[i].y = 0;             // Reposiciona no topo
            stars[i].x = random_int(0, 159); // Em uma nova posição X
        }

        // Define a cor da estrela com base na sua velocidade para dar profundidade
        // CORREÇÃO: Cast explícito para uint16_t para evitar erro de conversão.
        *DRAW_COLORS = (uint16_t)(stars[i].speed + 1);
        rect(stars[i].x, stars[i].y, 1, 1); // Desenha a estrela como um pixel
    }
}

/**
 * Desenha a pontuação na tela.
 */
void draw_score() {
    *DRAW_COLORS = 4; // Usa a cor 4 para o texto (a mais clara)
    char final_text[20] = "SCORE: ";
    char score_value[10];
    char* final_text_ptr = final_text + 7;
    
    itoa(score, score_value);
    
    // Concatena "SCORE: " com o valor da pontuação
    char* value_ptr = score_value;
    while (*value_ptr) { *final_text_ptr++ = *value_ptr++; }
    *final_text_ptr = '\0';
    
    text(final_text, 5, 5);
}

/**
 * Processa a entrada do jogador, move a nave e gerencia o disparo.
 * @param gamepad O estado do controle no quadro atual.
 */
void update_player(uint8_t gamepad) {
    if (gamepad & BUTTON_LEFT) player.x -= PLAYER_SPEED;
    if (gamepad & BUTTON_RIGHT) player.x += PLAYER_SPEED;

    if (player.x < 0) player.x = 0;
    if (player.x > 160 - 8) player.x = 160 - 8;

    if ((gamepad & BUTTON_1) && !player_bullet.active) {
        player_bullet.x = player.x + 3;
        player_bullet.y = player.y;
        player_bullet.active = TRUE;
        tone(1000, 10, 50, TONE_PULSE1); // Som de tiro
    }

    *DRAW_COLORS = 3; // Cor da nave
    blit(player_sprite, player.x, player.y, 8, 8, BLIT_1BPP);
}

/**
 * Move e desenha o projétil do jogador se ele estiver ativo.
 */
void update_player_bullet() {
    if (player_bullet.active) {
        player_bullet.y -= BULLET_SPEED;
        if (player_bullet.y < 0) {
            player_bullet.active = FALSE;
        }
        *DRAW_COLORS = 3; // Cor do projétil
        rect(player_bullet.x, player_bullet.y, 2, 4);
    }
}

/**
 * Move e desenha a formação de alienígenas.
 */
void update_aliens() {
    uint8_t move_down = FALSE;
    alien_timer--;
    if (alien_timer <= 0) {
        alien_timer = 20; // Reseta o temporizador
        for (int i = 0; i < TOTAL_ALIENS; ++i) {
            if (aliens[i].alive) {
                if ((aliens[i].x >= 160 - 8 && alien_direction > 0) || (aliens[i].x <= 0 && alien_direction < 0)) {
                    move_down = TRUE;
                    break;
                }
            }
        }
        if (move_down) {
            alien_direction *= -1; // Inverte a direção
            for (int i = 0; i < TOTAL_ALIENS; ++i) aliens[i].y += 5; // Move para baixo
        } else {
            for (int i = 0; i < TOTAL_ALIENS; ++i) aliens[i].x += alien_direction * 5; // Move para o lado
        }
    }
    
    *DRAW_COLORS = 4; // Cor dos alienígenas
    for (int i = 0; i < TOTAL_ALIENS; ++i) {
        if (aliens[i].alive) {
            blit(alien_sprite, aliens[i].x, aliens[i].y, 8, 8, BLIT_1BPP);
        }
    }
}

/**
 * Verifica colisões entre o projétil do jogador e os alienígenas.
 */
void check_collisions() {
    if (!player_bullet.active) return;
    for (int i = 0; i < TOTAL_ALIENS; ++i) {
        if (aliens[i].alive) {
            int a_x = aliens[i].x, a_y = aliens[i].y, a_w = 8, a_h = 8;
            int b_x = player_bullet.x, b_y = player_bullet.y, b_w = 2, b_h = 4;
            if (b_x < a_x + a_w && b_x + b_w > a_x && b_y < a_y + a_h && b_y + b_h > a_y) {
                aliens[i].alive = FALSE;
                player_bullet.active = FALSE;
                score += 10;
                tone(200, 15, 80, TONE_NOISE); // Som de explosão
                break;
            }
        }
    }
}

/**
 * Função principal de atualização do jogo.
 * É chamada 60 vezes por segundo pelo WASM-4.
 * Orquestra todas as chamadas de lógica e desenho.
 */
void update() {
    uint8_t gamepad = *GAMEPAD1; // Lê o estado do controle

    // A ordem de desenho é importante: do fundo para a frente.
    update_background();      // 1. Desenha o fundo
    update_player(gamepad);   // 2. Atualiza e desenha o jogador
    update_player_bullet();   // 3. Atualiza e desenha o projétil
    update_aliens();          // 4. Atualiza e desenha os alienígenas
    check_collisions();       // 5. Verifica a lógica de colisão
    draw_score();             // 6. Desenha a interface (pontuação) por cima de tudo
}