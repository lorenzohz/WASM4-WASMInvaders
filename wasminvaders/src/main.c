/**
 * main.c - Jogo Space Invaders reimaginado para o console de fantasia WASM-4.
 *
 * Este arquivo contém a lógica completa do jogo, incluindo:
 * - Movimento e tiro do jogador.
 * - Formação e movimento de alienígenas.
 * - Detecção de colisão.
 * - Sistema de pontuação.
 * - Fundo animado de estrelas com efeito de paralaxe.
 * - Paleta de cores customizada.
 * - Efeitos sonoros e música de vitória de onda.
 * - Explosões e efeitos visuais.
 * O código é autocontido e não depende de bibliotecas C padrão como stdio ou stdbool.
 */

#include "wasm4.h"

// --- Constantes para Lógica Booleana ---
#define TRUE 1
#define FALSE 0

// --- Constantes do Mouse ---
#define MOUSE_BUTTON_LEFT 0b0001
#define MOUSE_BUTTON_RIGHT 0b0010
#define MOUSE_BUTTON_MIDDLE 0b0100

// --- Constantes para Notas Musicais (Frequências em Hz) ---
#define NOTE_C5 523
#define NOTE_E5 659
#define NOTE_G5 784
#define NOTE_C6 1047
#define NOTE_REST 0 // Para pausas

// --- Constantes para Duração (em quadros, 60 quadros = 1 segundo) ---
#define DURATION_HALF 30    // Mínima
#define DURATION_QUARTER 15 // Semínima
#define DURATION_EIGHTH 7   // Colcheia

// --- Constantes do Jogo ---
#define PLAYER_SPEED 2                         // Velocidade de movimento do jogador
#define BULLET_SPEED 4                         // Velocidade do projétil
#define ALIEN_COLS 8                           // Número de colunas de alienígenas
#define ALIEN_ROWS 6                           // Número de linhas de alienígenas
#define TOTAL_ALIENS (ALIEN_COLS * ALIEN_ROWS) // Cálculo do total de alienígenas
#define STAR_COUNT 50                          // Número de estrelas no fundo

// --- Estados do Jogo ---
#define GAME_STATE_MENU 0
#define GAME_STATE_PLAYING 1

// --- Sprites e Paleta de Cores ---

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

// Estrutura para as estrelas do fundo
typedef struct
{
    int x, y;  // Posição na tela
    int speed; // Velocidade de movimento para o efeito de paralaxe
} Star;

// Estrutura para o jogador
typedef struct
{
    int x, y; // Posição na tela
} Player;

// Estrutura para um projétil
typedef struct
{
    int x, y;
    uint8_t active; // Estado do projétil (ativo ou inativo)
} Bullet;

// Estrutura para um alienígena
typedef struct
{
    int x, y;
    uint8_t alive; // Estado do alienígena (vivo ou morto)
} Alien;

// Estrutura para uma explosão
typedef struct {
    int x, y;
    int life; // Tempo de vida restante em frames
    uint8_t active;
} Explosion;

// --- Variáveis Globais ---
Player player;                  // Estado do jogador
Bullet player_bullet;           // Estado do projétil do jogador
Alien aliens[TOTAL_ALIENS];     // Array de alienígenas
Star stars[STAR_COUNT];         // Array de estrelas do fundo
int game_state;                 // Estado atual do jogo

int alien_direction = 1;        // Direção dos alienígenas (1=direita, -1=esquerda)
int alien_timer = 20;           // Timer para controlar a velocidade de movimento dos alienígenas
int current_alien_move_delay = 20; // Valor para resetar o timer
int score = 0;                  // Pontuação do jogador
uint32_t random_seed = 1;       // Semente para o gerador de números aleatórios
int aliens_left;                // Contador de alienígenas vivos
int current_wave = 1;           // Número da onda atual
int current_alien_rows;         // Número de linhas de alienígenas na onda atual
int current_alien_cols;         // Número de colunas de alienígenas na onda atual

#define EXPLOSION_DURATION 10 // Número de frames que a explosão permanece
Explosion explosions [TOTAL_ALIENS]; // Supondo que no máximo todos os alienígenas explodam ao mesmo tempo

// --- Variáveis Globais para música de vitória de onda ---
int wave_jingle_melody[] = {
    NOTE_C5, DURATION_EIGHTH,
    NOTE_E5, DURATION_EIGHTH,
    NOTE_G5, DURATION_QUARTER,
    NOTE_C6, DURATION_HALF,
    NOTE_REST, DURATION_EIGHTH};
int wave_jingle_length = sizeof(wave_jingle_melody) / sizeof(wave_jingle_melody[0]);
int current_jingle_note_index = 0;
int jingle_note_timer = 0;
int playing_wave_jingle = FALSE; // Flag para saber se o jingle está tocando

// --- Funções Utilitárias / Auxiliares ---

/**
 * Define a paleta de cores customizada do jogo.
 * As cores são definidas em formato hexadecimal RGB (0xRRGGBB).
 */

// Paleta de cores verde
void set_palette()
{
    PALETTE[0] = 0x191b1a; // Cor 1 (Fundo)
    PALETTE[1] = 0x294257; // Cor 2
    PALETTE[2] = 0x579c9a; // Cor 3
    PALETTE[3] = 0x99c9b3; // Cor 4 (Destaque)
}

// Paleta de cores roxa
/*
void set_palette()
{
    PALETTE[0] = 0x051f39; // Cor 1 (Fundo)
    PALETTE[1] = 0x051f39; // Cor 2
    PALETTE[2] = 0xc53a9d; // Cor 3
    PALETTE[3] = 0xff8e80; // Cor 4 (Destaque)
}
*/

/**
 * Gera um número inteiro pseudoaleatório dentro de um intervalo.
 * Usa um Gerador Linear Congruencial (LCG) simples.
 */
int random_int(int min, int max)
{
    random_seed = (random_seed * 1103515245 + 12345) & 0x7fffffff;
    return min + (int)(random_seed % (uint32_t)(max - min + 1));
}

/**
 * Converte um número inteiro para uma string (implementação de itoa).
 * Necessário porque a biblioteca C padrão não está disponível.
 */
void itoa(int n, char str[])
{
    int i = 0;
    if (n == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    int is_negative = n < 0;
    if (is_negative)
    {
        n = -n;
    }
    while (n != 0)
    {
        str[i++] = (n % 10) + '0';
        n = n / 10;
    }
    if (is_negative)
    {
        str[i++] = '-';
    }
    str[i] = '\0';
    int start = 0, end = i - 1;
    while (start < end)
    {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

// Retorna o menor valor entre dois inteiros
int minimum(int a, int b)
{
    return (a < b) ? a : b;
}

// --- Funções de Inicialização ---

/**
 * Posiciona as estrelas do fundo em locais aleatórios na tela.
 */
void init_stars()
{
    for (int i = 0; i < STAR_COUNT; ++i)
    {
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
void init_aliens()
{
    aliens_left = 0;

    // Limita o número de linhas e colunas
    if (current_alien_rows > ALIEN_ROWS)
    {
        current_alien_rows = ALIEN_ROWS;
    }
    if (current_alien_cols > ALIEN_COLS)
    {
        current_alien_cols = ALIEN_COLS;
    }

    // Inicializa alienígenas vivos
    for (int y = 0; y < current_alien_rows; ++y)
    {
        for (int x = 0; x < current_alien_cols; ++x)
        {
            int index = y * ALIEN_COLS + x;
            aliens[index] = (Alien){.x = 20 + x * 12, .y = 20 + y * 12, .alive = TRUE};
            aliens_left++;
        }
    }

    // Marca alienígenas restantes como mortos
    for (int i = aliens_left; i < TOTAL_ALIENS; ++i)
    {
        aliens[i].alive = FALSE;
    }
}

// Cria uma explosão na posição especificada

void create_explosion(int x, int y) {
    for (int i = 0; i < TOTAL_ALIENS; ++i) {
        if (!explosions [i].active) {
            explosions [i].x = x;
            explosions [i].y = y;
            explosions [i].life = EXPLOSION_DURATION;
            explosions [i].active = TRUE;
            break;
        }
    }
}

/**
 * Função principal de inicialização do jogo.
 * É chamada uma única vez quando o cartucho é carregado.
 */
void start()
{
    set_palette();
    init_stars();

    current_wave = 1;
    current_alien_rows = 1;
    current_alien_cols = ALIEN_COLS;

    init_aliens();

    player.x = 76;
    player.y = 140;
    player_bullet.active = FALSE;
    game_state = GAME_STATE_MENU;
    for (int i = 0; i < TOTAL_ALIENS; ++i) {
        explosions [i].active = FALSE;
    }

    current_alien_move_delay = 20;
    alien_timer = current_alien_move_delay;
}

// --- Funções de Lógica e Comportamento do Jogo ---

/**
 * Toca a melodia de "próxima onda" nota por nota.
 */
void play_wave_jingle()
{
    if (!playing_wave_jingle)
    {
        return;
    }

    if (jingle_note_timer > 0)
    {
        jingle_note_timer--;
        return;
    }

    if (current_jingle_note_index < wave_jingle_length)
    {
        int frequency = wave_jingle_melody[current_jingle_note_index];
        int duration_frames = wave_jingle_melody[current_jingle_note_index + 1];

        if (frequency != NOTE_REST)
        {
            tone((uint32_t)frequency, (uint32_t)duration_frames, 100, TONE_TRIANGLE);
        }

        jingle_note_timer = duration_frames;

        current_jingle_note_index += 2;
    }
    else
    {
        playing_wave_jingle = FALSE;
        tone(0, 0, 0, 0);
    }
}

/**
 * Processa a entrada do jogador, move a nave e gerencia o disparo.
 */
void update_player(uint8_t gamepad)
{
    // Movimento horizontal
    if (gamepad & BUTTON_LEFT)
        player.x -= PLAYER_SPEED;
    if (gamepad & BUTTON_RIGHT)
        player.x += PLAYER_SPEED;

    // Limita posição do jogador
    if (player.x < 0)
        player.x = 0;
    if (player.x > 160 - 8)
        player.x = 160 - 8;

    // Disparo do projétil
    if ((gamepad & BUTTON_1) && !player_bullet.active)
    {
        player_bullet.x = player.x + 3;
        player_bullet.y = player.y;
        player_bullet.active = TRUE;
        tone(1000, 10, 50, TONE_PULSE1); // Som de tiro
    }

    // Desenha o jogador
    *DRAW_COLORS = 3; // Cor da nave
    blit(player_sprite, player.x, player.y, 8, 8, BLIT_1BPP);
}

/**
 * Move e desenha o projétil do jogador se ele estiver ativo.
 */
void update_player_bullet()
{
    if (player_bullet.active)
    {
        player_bullet.y -= BULLET_SPEED;
        if (player_bullet.y < 0)
        {
            player_bullet.active = FALSE;
        }
        *DRAW_COLORS = 3; // Cor do projétil
        rect(player_bullet.x, player_bullet.y, 2, 4);
    }
}

/**
 * Move e desenha a formação de alienígenas.
 */
void update_aliens()
{
    uint8_t move_down = FALSE;
    alien_timer--;
    if (alien_timer <= 0)
    {
        alien_timer = current_alien_move_delay;
        // Verifica se algum alienígena chegou na borda
        for (int i = 0; i < TOTAL_ALIENS; ++i)
        {
            if (aliens[i].alive)
            {
                if ((aliens[i].x >= 160 - 8 && alien_direction > 0) || (aliens[i].x <= 0 && alien_direction < 0))
                {
                    move_down = TRUE;
                    break;
                }
            }
        }
        if (move_down)
        {
            alien_direction *= -1;
            for (int i = 0; i < TOTAL_ALIENS; ++i)
                aliens[i].y += 5;
        }
        else
        {
            for (int i = 0; i < TOTAL_ALIENS; ++i)
                aliens[i].x += alien_direction * 5;
        }
    }

    // Desenha alienígenas vivos
    *DRAW_COLORS = 4;
    for (int i = 0; i < TOTAL_ALIENS; ++i)
    {
        if (aliens[i].alive)
        {
            blit(alien_sprite, aliens[i].x, aliens[i].y, 8, 8, BLIT_1BPP);
        }
    }
}

/*
    * Atualiza o estado das explosões, diminuindo sua vida e desativando-as quando necessário.
*/
void update_explosions() {
    for (int i = 0; i < TOTAL_ALIENS; ++i) {
        if (explosions [i].active) {
            explosions [i].life--;
            if (explosions [i].life <= 0) {
                explosions [i].active = FALSE;
            }
        }
    }
}

/**
 * Verifica colisões entre o projétil do jogador e os alienígenas.
 */
void check_collisions()
{
    if (!player_bullet.active)
        return;
    for (int i = 0; i < TOTAL_ALIENS; ++i)
    {
        if (aliens[i].alive)
        {
            int a_x = aliens[i].x, a_y = aliens[i].y, a_w = 8, a_h = 8;
            int b_x = player_bullet.x, b_y = player_bullet.y, b_w = 2, b_h = 4;
            // Verifica colisão
            if (b_x < a_x + a_w && b_x + b_w > a_x && b_y < a_y + a_h && b_y + b_h > a_y)
            {
                aliens[i].alive = FALSE;
                create_explosion(aliens [i].x, aliens [i].y);
                player_bullet.active = FALSE;
                score += 10;
                aliens_left--;
                tone(200, 15, 80, TONE_NOISE);
                break;
            }
        }
    }
}

/**
 * Prepara a próxima onda de alienígenas, aumentando a dificuldade.
 */
void next_wave()
{
    current_wave++;

    // Aumenta velocidade dos alienígenas
    current_alien_move_delay = 20 - (current_wave * 3);
    if (current_alien_move_delay < 5)
    {
        current_alien_move_delay = 4;
    }
    alien_timer = current_alien_move_delay;

    current_alien_cols = ALIEN_COLS;
    current_alien_rows = (current_wave + 1) / 2;
    if (current_alien_rows > ALIEN_ROWS)
    {
        current_alien_rows = ALIEN_ROWS;
    }

    init_aliens();
    alien_direction = 1;

    // Inicia jingle de vitória
    current_jingle_note_index = 0;
    jingle_note_timer = 0;
    playing_wave_jingle = TRUE;
}

/**
 * Verifica colisões entre os alienígenas e o jogador.
 * Se houver colisão, define o estado do jogo para GAME_STATE_MENU.
 */
void check_player_collision()
{
    int p_x = player.x, p_y = player.y, p_w = 8, p_h = 8;

    for (int i = 0; i < TOTAL_ALIENS; ++i)
    {
        if (aliens[i].alive)
        {
            int a_x = aliens[i].x, a_y = aliens[i].y, a_w = 8, a_h = 8;

            // Verifica colisão
            if (p_x < a_x + a_w &&
                p_x + p_w > a_x &&
                p_y < a_y + a_h &&
                p_y + p_h > a_y)
            {
                game_state = GAME_STATE_MENU;

                tone(50, 60, 100, TONE_TRIANGLE);

                // Reinicia estado do jogo
                init_aliens();
                player.x = 76;
                player_bullet.active = FALSE;
                score = 0;
                alien_timer = 20;
                alien_direction = 1;
                current_wave = 1;
                current_alien_rows = 3;
                current_alien_cols = ALIEN_COLS;

                return;
            }
        }
    }
}

// --- Funções de Desenho e UI (Interface do Usuário) ---

/**
 * Desenha o fundo animado de estrelas.
 * As estrelas se movem para baixo, criando um efeito de paralaxe.
 */
void draw_background_stars()
{
    for (int i = 0; i < STAR_COUNT; ++i)
    {
        stars[i].y += minimum(stars[i].speed, 2);

        if (stars[i].y > 160)
        {
            stars[i].y = 0;
            stars[i].x = random_int(0, 159);
        }

        // Define a cor da estrela com base na sua velocidade para dar profundidade
        *DRAW_COLORS = (uint16_t)(stars[i].speed + 1);
        rect(stars[i].x, stars[i].y, 1, 1); // Desenha a estrela como um pixel
    }
}

/**
 * Desenha as explosões na tela.
 * As explosões são desenhadas como pequenos quadrados.
 */
void draw_explosions() {
    for (int i = 0; i < TOTAL_ALIENS; ++i) {
        if (explosions[i].active) {
            int base_x = explosions[i].x;
            int base_y = explosions[i].y;

            *DRAW_COLORS = 4;
            rect(base_x + random_int(-2, 2), base_y + random_int(-2, 2), 2, 2);

            *DRAW_COLORS = 4;
            rect(base_x + random_int(-3, 3), base_y + random_int(-3, 3), 3, 3);
            
            *DRAW_COLORS = 3;
            rect(base_x + random_int(-1, 1), base_y + random_int(-1, 1), 2, 2);
        }
    }
}

/**
 * Desenha a pontuação na tela.
 */
void draw_score()
{
    char score_text[20]; // Buffer para "SCORE:" + pontuação

    // Desenha a pontuação
    char score_value[10];
    itoa(score, score_value);
    char *score_ptr = score_text;
    const char *score_label = "SCORE:";
    while (*score_label)
        *score_ptr++ = *score_label++;
    char *value_ptr = score_value;
    while (*value_ptr)
        *score_ptr++ = *value_ptr++;
    *score_ptr = '\0';
    text(score_text, 5, 5); // Posição para a pontuação
}

/**
 * Desenha a onda atual na tela.
 */
void draw_wave()
{
    char wave_text[15]; // Buffer para "WAVE:" + onda

    // Desenha a onda atual
    char wave_value[5];
    itoa(current_wave, wave_value);
    char *wave_ptr = wave_text;
    const char *wave_label = "WAVE:";
    while (*wave_label)
        *wave_ptr++ = *wave_label++;
    char *value_ptr = wave_value;
    while (*value_ptr)
        *wave_ptr++ = *value_ptr++;
    *wave_ptr = '\0';
    text(wave_text, 100, 5); // Posição para a wave
}

// Atualiza tela de menu e verifica início do jogo
void update_menu()
{
    uint8_t gamepad = *GAMEPAD1;

    // Desenha o título e as instruções
    *DRAW_COLORS = 4;
    text("WASM INVADERS", 28, 50);

    *DRAW_COLORS = 3;
    text("Pressione espaco", 16, 80);
    text("ou clique para", 25, 90);
    text("comecar", 52, 100);

    // Lógica para iniciar o jogo:
    // Verifica se o Botão 1 do Gamepad foi pressionado OU
    // se o botão do mouse foi pressionado.
    if ((gamepad & BUTTON_1) || (*MOUSE_BUTTONS & MOUSE_BUTTON_LEFT))
    {
        game_state = GAME_STATE_PLAYING;

        // Reinicia o jogo, caso o usuário queira começar de novo
        init_aliens();
        score = 0;
        player.x = 76;
        player_bullet.active = FALSE;
    }
}

/**
 * Função principal de atualização do jogo.
 * É chamada 60 vezes por segundo pelo WASM-4.
 * Orquestra todas as chamadas de lógica e desenho.
 */
void update()
{
    draw_background_stars();

    switch (game_state)
    {
    case GAME_STATE_MENU:
    {
        update_menu();
        break;
    }

    case GAME_STATE_PLAYING:
    {
        uint8_t gamepad = *GAMEPAD1;

        update_player(gamepad);         // Atualiza jogador
        update_player_bullet();         // Atualiza projétil
        update_aliens();                // Atualiza alienígenas
        check_collisions();             // Verifica colisão projétil-alienígena
        check_player_collision();       // Verifica colisão jogador-alienígena
        draw_score();                   // Desenha pontuação
        draw_wave();                    // Desenha onda
        play_wave_jingle();             // Toca jingle de onda
        update_explosions();            // Atualiza explosões
        draw_explosions();              // Desenha explosões

        // Verifica se todos os alienígenas foram destruídos
        if (aliens_left <= 0)
        {
            next_wave();
        }
        break;
    }
    }
}