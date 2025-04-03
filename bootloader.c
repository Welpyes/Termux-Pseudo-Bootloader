#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>

#define TIMEOUT 10
#define NUM_OPTIONS 3
#define MAX_LINE 256

char OPTIONS[NUM_OPTIONS][50] = {
    "Fedora 41 (aarch64)",
    "Fedora root shell(fallback)",
    "Turn Off Computer"
};

char COMMANDS[NUM_OPTIONS][MAX_LINE] = {
    "bash $HOME/.config/startx",
    "bash -c 'pd sh fedora'",
    "/data/data/com.termux/files/usr/bin/pkill -f termux"
};

void strip_quotes(char *str) {
    int len = strlen(str);
    if (len >= 2 && str[0] == '"' && str[len - 1] == '"') {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}

int ini_parse(const char *filename, int (*handler)(void*, const char*, const char*, const char*), void *user) {
    FILE *file = fopen(filename, "r");
    if (!file) return -1;

    char line[MAX_LINE], section[MAX_LINE] = "", name[MAX_LINE], value[MAX_LINE];
    int lineno = 0;

    while (fgets(line, sizeof(line), file)) {
        lineno++;
        char *start = line;
        while (*start == ' ' || *start == '\t') start++;
        if (*start == '#' || *start == ';' || *start == '\n') continue;

        if (*start == '[') {
            char *end = strchr(start, ']');
            if (!end) continue;
            strncpy(section, start + 1, end - start - 1);
            section[end - start - 1] = '\0';
            continue;
        }

        char *eq = strchr(start, '=');
        if (!eq) continue;
        strncpy(name, start, eq - start);
        name[eq - start] = '\0';
        while (name[strlen(name) - 1] == ' ') name[strlen(name) - 1] = '\0';

        char *val_ptr = eq + 1;
        while (*val_ptr == ' ' || *val_ptr == '\t') val_ptr++;
        strncpy(value, val_ptr, sizeof(value) - 1);
        value[sizeof(value) - 1] = '\0';
        if (value[strlen(value) - 1] == '\n') value[strlen(value) - 1] = '\0';
        strip_quotes(value);

        if (handler(user, section, name, value) < 0) {
            fclose(file);
            return -1;
        }
    }

    fclose(file);
    return 0;
}

int ini_handler(void *user, const char *section, const char *name, const char *value) {
    if (strcmp(section, "selection_title") == 0) {
        if (strcmp(name, "distro") == 0) strncpy(OPTIONS[0], value, sizeof(OPTIONS[0]) - 1);
        else if (strcmp(name, "root") == 0) strncpy(OPTIONS[1], value, sizeof(OPTIONS[1]) - 1);
        else if (strcmp(name, "exit") == 0) strncpy(OPTIONS[2], value, sizeof(OPTIONS[2]) - 1);
    } else if (strcmp(section, "selection_cmd") == 0) {
        if (strcmp(name, "distro") == 0) strncpy(COMMANDS[0], value, sizeof(COMMANDS[0]) - 1);
        else if (strcmp(name, "root") == 0) strncpy(COMMANDS[1], value, sizeof(COMMANDS[1]) - 1);
        else if (strcmp(name, "exit") == 0) strncpy(COMMANDS[2], value, sizeof(COMMANDS[2]) - 1);
    }
    return 1;
}

void center_text(int row, int cols, const char *text) {
    int col = (cols - strlen(text)) / 2;
    mvprintw(row, col, "%s", text);
}

void draw_menu(int rows, int cols, int cursor) {
    clear();
    int start_row = (rows - NUM_OPTIONS - 4) / 2;
    center_text(start_row - 2, cols, "Boot Menu");

    for (int i = 0; i < NUM_OPTIONS; i++) {
        char option[50];
        snprintf(option, sizeof(option), " %s ", OPTIONS[i]);
        if (i == cursor) {
            attron(COLOR_PAIR(1));
            center_text(start_row + i, cols, option);
            attroff(COLOR_PAIR(1));
        } else {
            center_text(start_row + i, cols, option);
        }
    }

    char timeout_str[20];
    snprintf(timeout_str, sizeof(timeout_str), "Timeout %ds", TIMEOUT);
    center_text(start_row + NUM_OPTIONS + 1, cols, timeout_str);
    refresh();
}

void execute_command(int cursor) {
    endwin();
    FILE *log = fopen("$HOME/tmp/bootloader.log", "a");
    if (log) {
        fprintf(log, "Executing: %s\n", COMMANDS[cursor]);
        fclose(log);
    }
    int result = system(COMMANDS[cursor]);
    if (result != 0) {
        FILE *log = fopen("$HOME/tmp/bootloader.log", "a");
        if (log) {
            fprintf(log, "Command failed with exit code %d\n", result);
            fclose(log);
        }
    }
    exit(0);
}

int main() {
    char config_path[256];
    snprintf(config_path, sizeof(config_path), "%s/.config/bootloader/bootloader.ini", getenv("HOME"));
    ini_parse(config_path, ini_handler, NULL);

    initscr();
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);

    int rows = LINES;
    int cols = COLS;
    int cursor = 0;

    draw_menu(rows, cols, cursor);

    time_t end_time = time(NULL) + TIMEOUT;
    timeout(1000);

    while (time(NULL) < end_time) {
        int ch = getch();
        if (ch != ERR) {
            switch (ch) {
                case KEY_UP:
                    if (cursor > 0) cursor--;
                    end_time = time(NULL) + TIMEOUT;
                    break;
                case KEY_DOWN:
                    if (cursor < NUM_OPTIONS - 1) cursor++;
                    end_time = time(NULL) + TIMEOUT;
                    break;
                case '\n':
                    execute_command(cursor);
                    break;
            }
            draw_menu(rows, cols, cursor);
        }
    }

    execute_command(0);
    return 0;
}
