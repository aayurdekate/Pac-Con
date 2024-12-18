#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <ncurses.h>
#include <unistd.h>
#include <string.h>

#define WIDTH 40
#define HEIGHT 20
#define PACMAN '<'
#define WALL '#'
#define FOOD '.'
#define EMPTY ' '
#define DEMON 'X'
#define POWERUP 'O'
#define NUM_DEMONS 4
#define NUM_DOTS 100
#define POWERUP_DURATION 30
#define DEMON_RESPAWN_TIME 20
#define MAX_HIGH_SCORES 5
#define HIGH_SCORE_FILE "high_scores.txt"

typedef struct {
    int x, y;
} Position;

typedef struct {
    Position pos;
    int direction;
    bool on_food;
    bool active;
    int respawn_timer;
} Demon;

typedef struct {
    char name[20];
    int score;
} HighScore;

// Global variables
int score = 0;
int food_count = 0;
bool game_over = false;
bool win = false;
Position pacman;
char board[HEIGHT][WIDTH];
Demon demons[NUM_DEMONS];
int demon_count = 0;
bool powerup_active = false;
int powerup_timer = 0;
int lives = 3;

// Function declarations
void initialize_board();
void place_walls();
void place_food_and_powerups();
void place_demons();
void initialize_game();
void draw_board();
void reset_pacman_position();
void move_pacman(int dx, int dy);
void move_demon(Demon* demon);
void respawn_demon(Demon* demon);
void update_demons();
void update_powerup();
int play_game();
void display_home_page();
int display_main_menu();
void save_high_score(int score);
void view_high_scores();

// Function implementations
void initialize_board() {
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if (i == 0 || j == WIDTH - 1 || j == 0 || i == HEIGHT - 1) {
                board[i][j] = WALL;
            } else {
                board[i][j] = EMPTY;
            }
        }
    }
}

void place_walls() {
    int count = 50;
    while (count != 0) {
        int i = rand() % (HEIGHT - 2) + 1;
        int j = rand() % (WIDTH - 2) + 1;
        if (board[i][j] == EMPTY) {
            board[i][j] = WALL;
            count--;
        }
    }
}

void place_food_and_powerups() {
    food_count = 0;
    int powerups_placed = 0;
    int max_powerups = NUM_DOTS / 20;

    while (food_count < NUM_DOTS || powerups_placed < max_powerups) {
        int i = rand() % (HEIGHT - 2) + 1;
        int j = rand() % (WIDTH - 2) + 1;
        if (board[i][j] == EMPTY) {
            if (food_count < NUM_DOTS) {
                board[i][j] = FOOD;
                food_count++;
            } else if (powerups_placed < max_powerups) {
                board[i][j] = POWERUP;
                powerups_placed++;
            }
        }
    }
}

void place_demons() {
    demon_count = NUM_DEMONS;
    for (int i = 0; i < NUM_DEMONS; i++) {
        int x, y;
        do {
            x = rand() % (WIDTH - 2) + 1;
            y = rand() % (HEIGHT - 2) + 1;
        } while (board[y][x] != EMPTY && board[y][x] != FOOD);
        demons[i].pos.x = x;
        demons[i].pos.y = y;
        demons[i].direction = rand() % 4;
        demons[i].on_food = (board[y][x] == FOOD);
        demons[i].active = true;
        demons[i].respawn_timer = 0;
        board[y][x] = DEMON;
    }
}



void initialize_game() {
    srand(time(NULL));
    initialize_board();
    place_walls();
    place_food_and_powerups();
    place_demons();
    
    pacman.x = WIDTH / 2;
    pacman.y = HEIGHT / 2;
    board[pacman.y][pacman.x] = PACMAN;

    score = 0;
    lives = 3;
    game_over = false;
    win = false;
    powerup_active = false;
    powerup_timer = 0;
}


void draw_board() {
    clear();
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            switch (board[i][j]) {
                case PACMAN:
                    attron(COLOR_PAIR(powerup_active ? 6 : 1));
                    mvaddch(i, j, PACMAN);
                    attroff(COLOR_PAIR(powerup_active ? 6 : 1));
                    break;
                case WALL:
                    attron(COLOR_PAIR(2));
                    mvaddch(i, j, WALL);
                    attroff(COLOR_PAIR(2));
                    break;
                case FOOD:
                    attron(COLOR_PAIR(3));
                    mvaddch(i, j, FOOD);
                    attroff(COLOR_PAIR(3));
                    break;
                case DEMON:
                    attron(COLOR_PAIR(4));
                    mvaddch(i, j, DEMON);
                    attroff(COLOR_PAIR(4));
                    break;
                case POWERUP:
                    attron(COLOR_PAIR(5));
                    mvaddch(i, j, POWERUP);
                    attroff(COLOR_PAIR(5));
                    break;
                default:
                    mvaddch(i, j, board[i][j]);
            }
        }
    }
    mvprintw(HEIGHT + 1, 0, "Score: %d | Food remaining: %d | Powerup: %d | Lives: %d", 
             score, food_count, powerup_timer / 10, lives);
    refresh();
}


void reset_pacman_position() {
    board[pacman.y][pacman.x] = EMPTY;
    pacman.x = WIDTH / 2;
    pacman.y = HEIGHT / 2;
    board[pacman.y][pacman.x] = PACMAN;
}
void move_pacman(int dx, int dy) {
    int new_x = pacman.x + dx;
    int new_y = pacman.y + dy;

    if (board[new_y][new_x] != WALL) {
        if (board[new_y][new_x] == FOOD) {
            score += 10;
            food_count--;
            if (food_count == 0) {
                win = true;
                game_over = true;
                return;
            }
        } else if (board[new_y][new_x] == POWERUP) {
            powerup_active = true;
            powerup_timer = POWERUP_DURATION;
            score += 50;
        } else if (board[new_y][new_x] == DEMON) {
            if (powerup_active) {
                for (int i = 0; i < demon_count; i++) {
                    if (demons[i].pos.x == new_x && demons[i].pos.y == new_y && demons[i].active) {
                        demons[i].active = false;
                        demons[i].respawn_timer = DEMON_RESPAWN_TIME;
                        score += 200;
                        break;
                    }
                }
            } else {
                lives--;
                if (lives == 0) {
                    game_over = true;
                    return;
                }
                reset_pacman_position();
                return;
            }
        }

        board[pacman.y][pacman.x] = EMPTY;
        pacman.x = new_x;
        pacman.y = new_y;
        board[pacman.y][pacman.x] = PACMAN;
    }
}
void move_demon(Demon* demon) {
    if (!demon->active) return;

    int dx = 0, dy = 0;
    switch (demon->direction) {
        case 0: dy = -1; break;
        case 1: dx = 1; break;
        case 2: dy = 1; break;
        case 3: dx = -1; break;
    }

    int new_x = demon->pos.x + dx;
    int new_y = demon->pos.y + dy;

    if (board[new_y][new_x] != WALL && board[new_y][new_x] != DEMON) {
        if (demon->on_food) {
            board[demon->pos.y][demon->pos.x] = FOOD;
        } else {
            board[demon->pos.y][demon->pos.x] = EMPTY;
        }

        demon->pos.x = new_x;
        demon->pos.y = new_y;

        if (board[new_y][new_x] == FOOD) {
            demon->on_food = true;
        } else {
            demon->on_food = false;
        }

        if (board[new_y][new_x] == PACMAN) {
            if (!powerup_active) {
                lives--;
                if (lives == 0) {
                    game_over = true;
                } else {
                    reset_pacman_position();
                }
            }
        } else {
            board[new_y][new_x] = DEMON;
        }
    } else {
        demon->direction = rand() % 4;
    }
}

void respawn_demon(Demon* demon) {
    int x, y;
    do {
        x = rand() % (WIDTH - 2) + 1;
        y = rand() % (HEIGHT - 2) + 1;
    } while (board[y][x] != EMPTY && board[y][x] != FOOD);

    demon->pos.x = x;
    demon->pos.y = y;
    demon->direction = rand() % 4;
    demon->on_food = (board[y][x] == FOOD);
    demon->active = true;
    demon->respawn_timer = 0;
    board[y][x] = DEMON;
}

void update_demons() {
    for (int i = 0; i < demon_count; i++) {
        if (demons[i].active) {
            move_demon(&demons[i]);
        } else {
            demons[i].respawn_timer--;
            if (demons[i].respawn_timer <= 0) {
                respawn_demon(&demons[i]);
            }
        }
    }
}

void update_powerup() {
    if (powerup_active) {
        powerup_timer--;
        if (powerup_timer == 0) {
            powerup_active = false;
        }
    }
}

int play_game() {
    int ch;
    int frame_count = 0;
    int demon_move_interval = 3;

    while (!game_over && !win) {
        draw_board();
        ch = getch();
        if (ch != ERR) {
            switch (ch) {
                case 'w': case KEY_UP:    move_pacman(0, -1); break;
                case 's': case KEY_DOWN:  move_pacman(0, 1); break;
                case 'a': case KEY_LEFT:  move_pacman(-1, 0); break;
                case 'd': case KEY_RIGHT: move_pacman(1, 0); break;
                case 'q': game_over = true; break;
            }
        }
        
        if (frame_count % demon_move_interval == 0) {
            update_demons();
        }
        
        update_powerup();
        frame_count++;
        usleep(100000);
    }

    draw_board();
    if (win) {
        mvprintw(HEIGHT + 2, 0, "You have won the Game! Your final score: %d", score);
    } else if (game_over) {
        mvprintw(HEIGHT + 2, 0, "Game Over! Your final score: %d", score);
    }
    refresh();
    getch();
    
    return score;
}

void display_home_page() {
    clear();
    
    
    printw("\n\n");
    printw(" /$$      /$$           /$$                                                   /$$$$$$$$             \n");
    printw("| $$  /$ | $$          | $$                                                  |__  $$__/             \n");
    printw("| $$ /$$$| $$  /$$$$$$ | $$  /$$$$$$$  /$$$$$$  /$$$$$$/$$$$   /$$$$$$          | $$  /$$$$$$       \n");
    printw("| $$/$$ $$ $$ /$$__  $$| $$ /$$_____/ /$$__  $$| $$_  $$_  $$ /$$__  $$         | $$ /$$__  $$      \n");
    printw("| $$$$_  $$$$| $$$$$$$$| $$| $$      | $$  \\ $$| $$ \\ $$ \\ $$| $$$$$$$$         | $$| $$  \\ $$      \n");
    printw("| $$$/ \\  $$$| $$_____/| $$| $$      | $$  | $$| $$ | $$ | $$| $$_____/         | $$| $$  | $$      \n");
    printw("| $$/   \\  $$|  $$$$$$$| $$|  $$$$$$$|  $$$$$$/| $$ | $$ | $$|  $$$$$$$         | $$|  $$$$$$/      \n");
    printw("|__/     \\__/ \\_______/|__/ \\_______/ \\______/ |__/ |__/ |__/ \\_______/         |__/ \\______/       \n");
    printw("                                                                                                    \n");
    printw("                                                                                                    \n");
    printw("                                                                                                    \n");
    printw("                   /$$$$$$$                                                                         \n");
    printw("                  | $$__  $$                                                                        \n");
    printw("                  | $$  \\ $$ /$$$$$$   /$$$$$$$ /$$$$$$/$$$$   /$$$$$$  /$$$$$$$                    \n");
    printw("                  | $$$$$$$/|____  $$ /$$_____/| $$_  $$_  $$ |____  $$| $$__  $$                   \n");
    printw("                  | $$____/  /$$$$$$$| $$      | $$ \\ $$ \\ $$  /$$$$$$$| $$  \\ $$                   \n");
    printw("                  | $$      /$$__  $$| $$      | $$ | $$ | $$ /$$__  $$| $$  | $$                   \n");
    printw("                  | $$     |  $$$$$$$|  $$$$$$$| $$ | $$ | $$|  $$$$$$$| $$  | $$                   \n");
    printw("                  |__/      \\_______/ \\_______/|__/ |__/ |__/ \\_______/|__/  |__/                   \n");
    printw("                                                                                                    \n");
    printw("                                                                                                    \n");
    
    printw("\n\nPress any key to continue...");
    refresh();
    getch();
}
int display_main_menu() {
    clear();
    printw("Main Menu\n");
    printw("1. Play the Game\n");
    printw("2. View High Scores\n");
    printw("3. Exit\n");
    printw("\nEnter your choice: ");
    refresh();
    
    int choice;
    scanf("%d", &choice);
    return choice;
}
void save_high_score(int score) {
    HighScore high_scores[MAX_HIGH_SCORES];
    FILE *file = fopen(HIGH_SCORE_FILE, "r");
    int count = 0;
    
    if (file) {
        while (fscanf(file, "%s %d", high_scores[count].name, &high_scores[count].score) == 2 && count < MAX_HIGH_SCORES) {
            count++;
        }
        fclose(file);
    }
    
    if (count < MAX_HIGH_SCORES || score > high_scores[count-1].score) {
        char name[20];
        clear();
        printw("You got a high score! Enter your name: ");
        refresh();
        echo();
        getnstr(name, sizeof(name));
        noecho();
        
        int i;
        for (i = count - 1; i >= 0 && high_scores[i].score < score; i--) {
            if (i + 1 < MAX_HIGH_SCORES) {
                high_scores[i + 1] = high_scores[i];
            }
        }
        
        if (i + 1 < MAX_HIGH_SCORES) {
            strcpy(high_scores[i + 1].name, name);
            high_scores[i + 1].score = score;
            if (count < MAX_HIGH_SCORES) count++;
        }
        
        file = fopen(HIGH_SCORE_FILE, "w");
        for (i = 0; i < count; i++) {
            fprintf(file, "%s %d\n", high_scores[i].name, high_scores[i].score);
        }
        fclose(file);
    }
}
void view_high_scores() {
    clear();
    printw("High Scores\n\n");
    
    FILE *file = fopen(HIGH_SCORE_FILE, "r");
    if (file) {
        char name[20];
        int score;
        int rank = 1;
        while (fscanf(file, "%s %d", name, &score) == 2 && rank <= MAX_HIGH_SCORES) {
            printw("%d. %s: %d\n", rank, name, score);
            rank++;
        }
        fclose(file);
    } else {
        printw("No high scores yet.\n");
    }
    
    printw("\nPress any key to return to the main menu...");
    refresh();
    getch();
}

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    start_color();
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_WHITE, COLOR_BLACK);

    display_home_page();

    bool keep_playing = true;
    while (keep_playing) {
        int choice = display_main_menu();
        switch (choice) {
            case 1:
                // Play the game
                initialize_game();
                int final_score = play_game();
                save_high_score(final_score);
                break;
            case 2:
                view_high_scores();
                break;
            case 3:
                keep_playing = false;
                break;
            default:
                printw("Invalid choice. Press any key to continue...");
                refresh();
                getch();
        }
    }

    endwin();
    return 0;
}

