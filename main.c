/*    Copyright (c) 2025 Sushant kr. Ray
 *
 *    Permission is hereby granted, free of charge, to any person obtaining a copy
 *    of this software and associated documentation files (the "Software"), to deal
 *    in the Software without restriction, including without limitation the rights
 *    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *    copies of the Software, and to permit persons to whom the Software is
 *    furnished to do so, subject to the following conditions:
 *
 *    The above copyright notice and this permission notice shall be included in all
 *    copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *    SOFTWARE.
 */

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#define BOARD_W 10
#define BOARD_H 20
#define TICK_MS_INITIAL 550
#define TICK_MS_MIN 80
#define LEVEL_UP_LINES 10

int board[BOARD_H][BOARD_W];

typedef struct { int x, y; int type; int rot; } Piece;

const unsigned short TETRO[7][4] = {
    {0x0F00, 0x2222, 0x00F0, 0x4444}, // |
    {0x0660, 0x0660, 0x0660, 0x0660}, // O
    {0x0E40, 0x4C40, 0x4E00, 0x4640}, // T
    {0x06C0, 0x4620, 0x06C0, 0x4620}, // S
    {0x0C60, 0x2640, 0x0C60, 0x2640}, // Z
    {0x08E0, 0x6440, 0x0E20, 0x44C0}, // J
    {0x02E0, 0x4460, 0x0E80, 0xC440}, // L
};

int colors_for_type[7] = { COLOR_CYAN, COLOR_YELLOW,
                           COLOR_MAGENTA, COLOR_GREEN,
                           COLOR_RED, COLOR_BLUE, COLOR_WHITE };

Piece cur;
bool game_over = false;
bool paused = false;
int score = 0;
int lines_cleared = 0;
int level = 1;
int tick_ms = TICK_MS_INITIAL;
int field_w, field_h;

int min(int a, int b){
    return a < b ? a : b;
}

int max(int a, int b){
    return a > b ? a : b;
}

void seed_rng(void){
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    srand((unsigned)(ts.tv_nsec ^ ts.tv_sec));
}

int rnd(int n){
    return rand() % n;
}

bool cell(int type, int rot, int r, int c){
    unsigned short m = TETRO[type][rot%4];
    return (m >> (15 - (r*4 + c))) & 1;
}

bool can_place(Piece p){
    for(int r = 0; r < 4; r++)
        for(int c = 0; c < 4; c++){
            if(!cell(p.type, p.rot, r, c))
                continue;

            int x = p.x + c;
            int y = p.y + r;
            if(x < 0 || x >= BOARD_W || y >= BOARD_H)
                return false;

            if(y >= 0 && board[y][x])
                return false;
        }

    return true;
}

void lock_piece(Piece p){
    for(int r = 0; r < 4; r++)
        for(int c = 0; c < 4; c++){
            if(!cell(p.type, p.rot, r, c))
                continue;

            int x = p.x + c;
            int y = p.y + r;
            if(y >= 0 && y < BOARD_H && x >= 0 && x < BOARD_W)
                board[y][x] = p.type + 1;
        }
}

int clear_lines(void){
    int cleared = 0;
    for(int y = BOARD_H - 1; y >= 0; y--){
        bool full = true;
        for(int x = 0; x < BOARD_W; x++)
            if(!board[y][x]){
                full = false;
                break;
            }

        if(full){
            cleared++;
            for(int yy = y; yy > 0; yy--)
                memcpy(board[yy], board[yy-1], sizeof(board[0]));

            memset(board[0], 0, sizeof(board[0]));
            y++;
        }
    }

    return cleared;
}

void new_piece(void){
    cur.type = rnd(7);
    cur.rot = 0;
    cur.x = BOARD_W / 2 - 2;
    cur.y = -2;
    if(!can_place(cur)){
        game_over = true;
    }
}

void draw_cell(int sy, int sx, int val, bool ghost){
    int pair = val ? val : 8;
    attron(COLOR_PAIR(pair));
    if(ghost)
        attron(A_DIM);

    mvaddch(sy, sx, ' ' | A_REVERSE);
    mvaddch(sy, sx+1, ' ' | A_REVERSE);

    if(ghost)
        attroff(A_DIM);

    attroff(COLOR_PAIR(pair));
}

void draw_board(int origin_y, int origin_x){
    int maxh, maxw;
    getmaxyx(stdscr, maxh, maxw);

    field_w = maxw - origin_x;
    field_h = maxh - origin_y;

    if (field_w < 38 || field_h < 22) {
        paused = true;

        mvaddstr(maxh / 2 - 1, maxw / 2 - 9,
                 "Terminal too small"
        );

        mvaddstr(maxh / 2, maxw / 2 - 13,
                 "Enlarge the window to play"
        );

        refresh();
        return;
    }

    attron(A_BOLD);
    move(origin_y - 1, origin_x);
    addch(ACS_ULCORNER);
    for(int i = 0; i < BOARD_W * 2; i++)
        addch(ACS_HLINE);
    addch(ACS_URCORNER);

    for(int y = 0; y < BOARD_H; y++){
        mvaddch(origin_y + y, origin_x, ACS_VLINE);
        mvaddch(origin_y + y, origin_x + BOARD_W * 2 + 1, ACS_VLINE);
    }

    move(origin_y + BOARD_H, origin_x);
    addch(ACS_LLCORNER);
    for(int i = 0; i < BOARD_W * 2; i++)
        addch(ACS_HLINE);
    addch(ACS_LRCORNER);
    attroff(A_BOLD);

    for(int y = 0; y < BOARD_H; y++){
        for(int x = 0; x < BOARD_W; x++){
            int sy = origin_y + y;
            int sx = origin_x + x * 2 + 1;
            draw_cell(sy, sx, board[y][x], false);
        }
    }

    for(int r = 0; r < 4; r++)
        for(int c = 0; c < 4; c++)
            if(cell(cur.type, cur.rot, r, c)){
                int x = cur.x + c;
                int y = cur.y + r;

                if(y >= 0){
                    int sy = origin_y + y;
                    int sx = origin_x + x * 2 + 1;
                    draw_cell(sy, sx, cur.type + 1, false);
                }
            }

    Piece g = cur;
    while(can_place((Piece){g.x, g.y + 1, g.type, g.rot}))
        g.y++;

    for(int r = 0; r < 4; r++)
        for(int c = 0; c < 4; c++)
            if(cell(g.type, g.rot, r, c)){
                int x = g.x + c;
                int y = g.y + r;
                if(y < 0)
                    continue;

                int sy = origin_y + y;
                int sx = origin_x + x * 2 + 1;
                draw_cell(sy, sx, cur.type + 1, true);
            }
}

void draw_ui(int origin_y, int origin_x){
    int info_x = origin_x + BOARD_W * 2 + 8;

    mvprintw(origin_y,     info_x, "Score: %d", score);
    mvprintw(origin_y + 1, info_x, "Lines: %d", lines_cleared);
    mvprintw(origin_y + 2, info_x, "Level: %d", level);
    if(paused)
        mvprintw(origin_y + 3, info_x, "PAUSED");

    if(game_over)
        mvprintw(origin_y + 4, info_x, "GAME OVER");
}

void init_colors(void){
    if(!has_colors())
        return;

    start_color();
    use_default_colors();
    init_pair(1, colors_for_type[0], -1);
    init_pair(2, colors_for_type[1], -1);
    init_pair(3, colors_for_type[2], -1);
    init_pair(4, colors_for_type[3], -1);
    init_pair(5, colors_for_type[4], -1);
    init_pair(6, colors_for_type[5], -1);
    init_pair(7, colors_for_type[6], -1);
    init_pair(8, COLOR_BLACK, -1);
}

void soft_drop(void){
    Piece nxt = cur;
    nxt.y++;

    if(can_place(nxt))
        cur = nxt;
    else {
        lock_piece(cur);
        int c = clear_lines();
        if(c){
            lines_cleared += c;
            const int table[5] = {0, 100, 300, 500, 800};
            score += table[c] * level;
            if(lines_cleared / LEVEL_UP_LINES + 1 > level){
                level = lines_cleared / LEVEL_UP_LINES + 1;
                tick_ms = max(TICK_MS_MIN, TICK_MS_INITIAL - (level - 1) * 45);
            }
        }

        new_piece();
    }
}

void hard_drop(void){
    while(can_place((Piece){cur.x, cur.y + 1, cur.type, cur.rot}))
        cur.y++;

    soft_drop();
}

int main(void){
    seed_rng();
    initscr();
    atexit((void(*)(void))endwin);
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

#ifdef NCURSES_VERSION
    set_escdelay(25);
#endif

    curs_set(0);
    init_colors();

    memset(board, 0, sizeof(board));
    new_piece();

    struct timespec last, now;
    clock_gettime(CLOCK_MONOTONIC, &last);
    int fall_accum = 0;

    while(1){
        clock_gettime(CLOCK_MONOTONIC, &now);
        int dt = (int)((now.tv_sec - last.tv_sec) * 1000 +
                       (now.tv_nsec - last.tv_nsec) / 1000000);

        if(dt < 0)
            dt = 0;

        last = now;
        if(!paused && !game_over){
            fall_accum += dt;
            while(fall_accum >= tick_ms){
                fall_accum -= tick_ms;
                soft_drop();
                if(game_over)
                    break;
            }
        }

        int ch = getch();
        if(ch != ERR){
            if(ch == 'q' || ch == 'Q')
                break;

            if(!game_over){
                if(ch == 'p' || ch == 'P')
                    paused = !paused;

                if(!paused){
                    if(ch == KEY_LEFT){
                        Piece n = cur;
                        n.x--;
                        if(can_place(n))
                            cur=n;
                    }

                    else if(ch == KEY_RIGHT){
                        Piece n = cur;
                        n.x++;
                        if(can_place(n))
                            cur = n;
                    }

                    else if(ch == KEY_DOWN){
                        soft_drop();
                    }

                    else if(ch == KEY_UP){
                        Piece n = cur;
                        n.rot = (n.rot + 1) % 4;
                        if(can_place(n))
                            cur=n;
                    }

                    else if(ch == ' '){
                        hard_drop();
                    }
                }
            }
        }

        erase();
        int oy = 2, ox = 4;
        mvprintw(0, 4, "Ncurses Tetris");
        draw_board(oy, ox);
        draw_ui(oy, ox);
        refresh();

        struct timespec ts = {0, 8000000};
        nanosleep(&ts, NULL);
    }

    erase();
    mvprintw(LINES / 2 - 1, (COLS - 20) / 2,
             "Final score: %d  Lines: %d", score, lines_cleared);
    mvprintw(LINES / 2, (COLS - 20) / 2,
             "Press any key to exit.");

    nodelay(stdscr, FALSE);
    getch();
    endwin();
    return 0;
}
