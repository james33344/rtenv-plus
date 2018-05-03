#include "rtenv.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ioe.h"
#define DIR_OFFSET 34
#define MAX_COUNT 256
int MAX_D_COUNT = 3;
int DELAY = 10;
int delay_count = 0;
extern int tick_count;
extern void itoa(int n, char *dst, int base);
/*	-----
 *	|		|
 *	|		|
 *	|		|
 *	|		|
 *	-----
 *    x  ->
 *    y  down
 *    rx <-
 *    ry up
 */
enum {
    To_x,
    To_y,
    To_rx,
    To_ry,
};


static TP_STATE *TP_State;
int btn_clicked() {
    if (!((TP_State->Y <= 318) && (TP_State->Y >= 240))) {
        return -1;
    }
    if ((TP_State->X <= LCD_PIXEL_WIDTH / 4) && (TP_State->X >= 1)) {
        return To_rx;
    }
    else if ((TP_State->X <= LCD_PIXEL_WIDTH / 2) &&
             (TP_State->X > LCD_PIXEL_WIDTH / 4)) {
        return To_ry;
    }
    else if ((TP_State->X <= LCD_PIXEL_WIDTH / 4 * 3) &&
             (TP_State->X > LCD_PIXEL_WIDTH / 2)) {
        return To_y;
    }
    else if ((TP_State->X <= LCD_PIXEL_WIDTH) &&
             (TP_State->X > LCD_PIXEL_WIDTH / 4 * 3)) {
        return To_x;
    }

    return -1;
}

#define MAX_S_COUNT 4
int special_count = 0;
int snake_count = 15;
int snake_size = 8;
int snake_tail_pos = 50;
int snake_x_pos = 110;
int is_over = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct snake_t {
    int x;
    int y;
    int dir;
} snake[MAX_COUNT];

void *af(void *arg) {
    while (1) {
        if (is_over) {
            pthread_exit(NULL);
        }
        TP_State = IOE_TP_GetState();
        if (TP_State->TouchDetected) {
            switch (btn_clicked()) {
                case To_x: {
                    snake[0].dir = To_x;
                } break;

                case To_y: {
                    snake[0].dir = To_y;
                } break;

                case To_rx: {
                    snake[0].dir = To_rx;
                } break;

                case To_ry: {
                    snake[0].dir = To_ry;
                } break;

                default:
                    break;
            }
        }
        if (++delay_count > MAX_D_COUNT) {
            delay_count = 0;
            MAX_D_COUNT += 1 / (DELAY * DELAY) * 120;
            DELAY == 2 ? DELAY = 2 : --DELAY;
        }
        sleep(DELAY);
    }

    pthread_exit(NULL);
    return NULL;
}

void *bf(void *arg) {
    while (1) {
        if (is_over) {
            break;
        }
        int rc = pthread_mutex_lock(&mutex);
        if (rc != 0) {
            LCD_DisplayStringLine(LINE(1), (uint8_t *) "mutex error");
        }
        LCD_SetTextColor(LCD_COLOR_WHITE);
        LCD_DrawFullRect(snake[snake_count - 1].x, snake[snake_count - 1].y,
                         snake_size, snake_size);
        LCD_SetTextColor(LCD_COLOR_BLACK);

        for (int i = snake_count - 1; i >= 0; i--) {
            switch (snake[i].dir) {
                case To_x: {
                    snake[i].x += snake_size;
                    if (snake[i].x > LCD_PIXEL_WIDTH) {
                        is_over = 1;
                    }
                } break;

                case To_y: {
                    snake[i].y += snake_size;
                    if (snake[i].y > 240) {
                        is_over = 1;
                    }
                } break;

                case To_rx: {
                    snake[i].x -= snake_size;
                    if (snake[i].x < 0) {
                        is_over = 1;
                    }
                } break;

                case To_ry: {
                    snake[i].y -= snake_size;
                    if (snake[i].y < 0) {
                        is_over = 1;
                    }
                } break;
            }
            if (i > 0 && snake[i].dir != snake[i - 1].dir) {
                snake[i].dir = snake[i - 1].dir;
            }
        }
        for (int i = 1; i < snake_count; i++) {
            if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
                is_over = 1;
                break;
            }
        }

        //	LCD_DisplayStringLine(LINE(2), (uint8_t*)"mutex unlock2");
        rc = pthread_mutex_unlock(&mutex);
        if (rc != 0) {
            LCD_DisplayStringLine(LINE(1), (uint8_t *) "mutex error");
        }
        sleep(DELAY);
    }
    pthread_exit(NULL);
    return NULL;
}

void *cf(void *arg) {
    while (1) {
        int rc = pthread_mutex_lock(&mutex);
        if (rc != 0) {
            LCD_DisplayStringLine(LINE(1), (uint8_t *) "mutex error");
        }

        if (!is_over) {
            if (++special_count > MAX_S_COUNT) {
                special_count = 0;
                snake[snake_count].x = snake[snake_count - 1].x;
                snake[snake_count].y = snake[snake_count - 1].y;
                snake[snake_count].dir = snake[snake_count - 1].dir;
                switch (snake[snake_count - 1].dir) {
                    case To_x: {
                        snake[snake_count].x -= snake_size;
                    } break;
                    case To_y: {
                        snake[snake_count].y -= snake_size;
                    } break;
                    case To_rx: {
                        snake[snake_count].x += snake_size;
                    } break;
                    case To_ry: {
                        snake[snake_count].y += snake_size;
                    } break;
                }
                snake_count++;
            }
            for (int i = 0; i < snake_count; i++) {
                LCD_DrawFullRect(snake[i].x, snake[i].y, snake_size,
                                 snake_size);
            }

            //		LCD_DrawFullCircle(k1, k2, 4);
        }
        else {
            pthread_exit(NULL);
        }

        rc = pthread_mutex_unlock(&mutex);
        //	LCD_DisplayStringLine(LINE(2), (uint8_t*)"mutex unlock3");
        if (rc != 0) {
            LCD_DisplayStringLine(LINE(1), (uint8_t *) "mutex error");
        }
        sleep(DELAY);
    }
    pthread_exit(NULL);
    return NULL;
}

int main() {
    struct sched_param a = {.policy = 0, .sched_priority = PRIORITY_DEFAULT};
    struct sched_param b = {.policy = 0,
                            .sched_priority = PRIORITY_DEFAULT + 1};
    struct sched_param c = {.policy = 0,
                            .sched_priority = PRIORITY_DEFAULT + 2};
    pthread_attr_t a_t, b_t, c_t;
    pthread_attr_init(&a_t);
    pthread_attr_init(&b_t);
    pthread_attr_init(&c_t);
    pthread_attr_setschedparam(&a_t, &a);
    pthread_attr_setschedparam(&b_t, &b);
    pthread_attr_setschedparam(&c_t, &c);


    uint16_t linenum = 0;
    LCD_Init();
    LCD_LayerInit();
    LTDC_Cmd(ENABLE);
    LCD_SetLayer(LCD_FOREGROUND_LAYER);
    LCD_Clear(LCD_COLOR_WHITE);
    //	LCD_DrawRect(40,40,40,40);
    IOE_Config();
    LCD_SetFont(&Font8x8);

    LCD_DisplayStringLine(LINE(10), (uint8_t *) "    main init finished");
    LCD_DisplayChar(LINE(35), LCD_PIXEL_WIDTH / 4 - DIR_OFFSET, (uint8_t) 'L');
    LCD_DisplayChar(LINE(35), LCD_PIXEL_WIDTH / 2 - DIR_OFFSET, (uint8_t) 'U');
    LCD_DisplayChar(LINE(35), LCD_PIXEL_WIDTH / 4 * 3 - DIR_OFFSET,
                    (uint8_t) 'D');
    LCD_DisplayChar(LINE(35), LCD_PIXEL_WIDTH - DIR_OFFSET, (uint8_t) 'R');
    LCD_DrawLine(0, 240, LCD_PIXEL_WIDTH, LCD_DIR_HORIZONTAL);
    LCD_DrawLine(LCD_PIXEL_WIDTH / 4, 240, 79, LCD_DIR_VERTICAL);
    LCD_DrawLine(LCD_PIXEL_WIDTH / 2, 240, 79, LCD_DIR_VERTICAL);
    LCD_DrawLine(LCD_PIXEL_WIDTH / 4 * 3, 240, 79, LCD_DIR_VERTICAL);
    LCD_DisplayStringLine(LINE(11), (uint8_t *) "    touch screen to start");

    while (1) {
        TP_State = IOE_TP_GetState();
        if (!TP_State->TouchDetected) {
            continue;
        }
        LCD_SetFont(&Font8x8);
        for (linenum = 0; linenum < 27; linenum++) {
            LCD_ClearLine(LINE(linenum));
        }

        for (int i = 0; i < snake_count; i++) {
            LCD_DrawFullRect(snake_x_pos, snake_tail_pos + snake_size * i,
                             snake_size, snake_size);
            snake[i].x = snake_x_pos;
            snake[i].y = snake_tail_pos + snake_size * i;
            snake[i].dir = To_ry;
        }

        pthread_t ap, bp, cp;
        pthread_create(&ap, &a_t, af, NULL);
        pthread_create(&bp, &b_t, bf, NULL);
        pthread_create(&cp, &c_t, cf, NULL);

        pthread_join(ap, NULL);
        pthread_join(bp, NULL);
        pthread_join(cp, NULL);
        LCD_SetFont(&Font8x8);
        for (linenum = 0; linenum < 30; linenum++) {
            LCD_ClearLine(LINE(linenum));
        }
        LCD_DisplayStringLine(LINE(10), (uint8_t *) "    main init finished");
        LCD_DisplayStringLine(LINE(11),
                              (uint8_t *) "    touch screen to start");
        LCD_DisplayStringLine(LINE(13), (uint8_t *) "          GAME OVER");
        LCD_SetFont(&Font12x12);
        char s[22];
        itoa(tick_count, s, 10);
        LCD_DisplayStringLine(LINE(18), (uint8_t *) "Score:");
        LCD_DisplayStringLine(LINE(19), (uint8_t *) s);
        LCD_SetFont(&Font8x8);
        is_over = 0;
    }

    return 0;
}
