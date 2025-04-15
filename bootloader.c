#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <yaml.h>

#define MAX_LINE 256
#define MAX_OPTIONS 10

typedef struct {
    char label[50];
    char cmd[MAX_LINE];
} Option;

typedef struct {
    char title[50];
    int timeout;
    Option *options;
    int num_options;
} Config;

void log_error(const char *msg) {
    char path[256];
    snprintf(path, sizeof(path), "%s/tmp/bootloader.log", getenv("HOME"));
    FILE *log = fopen(path, "a");
    if (log) {
        time_t now = time(NULL);
        fprintf(log, "[%s] ERROR: %s\n", ctime(&now), msg);
        fclose(log);
    }
}

void center_text(int row, int cols, const char *text) {
    int col = (cols - strlen(text)) / 2;
    mvprintw(row, col, "%s", text);
}

void draw_menu(int rows, int cols, int cursor, Config *config, int remaining) {
    clear();
    int start_row = (rows - config->num_options - 4) / 2;
    center_text(start_row - 2, cols, config->title);

    for (int i = 0; i < config->num_options; i++) {
        char option[50];
        snprintf(option, sizeof(option), " %s ", config->options[i].label);
        if (i == cursor) {
            attron(COLOR_PAIR(1));
            center_text(start_row + i, cols, option);
            attroff(COLOR_PAIR(1));
        } else {
            center_text(start_row + i, cols, option);
        }
    }

    char timeout_str[20];
    snprintf(timeout_str, sizeof(timeout_str), "Timeout %ds", remaining);
    center_text(start_row + config->num_options + 1, cols, timeout_str);
    refresh();
}

void execute_command(int cursor, Config *config) {
    endwin();
    char path[256];
    snprintf(path, sizeof(path), "%s/tmp/bootloader.log", getenv("HOME"));
    FILE *log = fopen(path, "a");
    if (log) {
        fprintf(log, "Executing: %s\n", config->options[cursor].cmd);
        fclose(log);
    } else {
        log_error("Failed to open log file for writing");
    }
    int result = system(config->options[cursor].cmd);
    if (result != 0) {
        char err[256];
        snprintf(err, sizeof(err), "Command '%s' failed with exit code %d", 
                 config->options[cursor].cmd, result);
        log_error(err);
    }
    exit(0);
}

int parse_yaml(const char *filename, Config *config) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        log_error("Failed to open YAML config file");
        return -1;
    }

    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        log_error("Failed to initialize YAML parser");
        fclose(file);
        return -1;
    }
    yaml_parser_set_input_file(&parser, file);

    yaml_event_t event;
    int done = 0, in_prompt = 0, in_options_seq = 0, current_option = -1;
    char options_list[MAX_OPTIONS][50];
    int option_count = 0;

    config->options = malloc(MAX_OPTIONS * sizeof(Option));
    if (!config->options) {
        log_error("Memory allocation failed for options");
        fclose(file);
        yaml_parser_delete(&parser);
        return -1;
    }
    config->num_options = 0;

    while (!done) {
        if (!yaml_parser_parse(&parser, &event)) {
            char err[256];
            snprintf(err, sizeof(err), "YAML parsing error at line %zu: %s", 
                     parser.problem_mark.line + 1, parser.problem);
            log_error(err);
            break;
        }

        if (event.type == YAML_STREAM_END_EVENT) {
            done = 1;
        } else if (event.type == YAML_MAPPING_START_EVENT) {
            // Just keep going
        } else if (event.type == YAML_MAPPING_END_EVENT) {
            if (in_prompt && !in_options_seq) in_prompt = 0;
            else if (current_option >= 0) current_option = -1;
        } else if (event.type == YAML_SEQUENCE_START_EVENT) {
            if (in_prompt) in_options_seq = 1;
        } else if (event.type == YAML_SEQUENCE_END_EVENT) {
            if (in_options_seq) in_options_seq = 0;
        } else if (event.type == YAML_SCALAR_EVENT) {
            char *value = (char *)event.data.scalar.value;

            if (!in_prompt && current_option == -1 && !in_options_seq) {
                if (strcmp(value, "prompt") == 0) {
                    in_prompt = 1;
                } else {
                    for (int i = 0; i < option_count; i++) {
                        if (strcmp(value, options_list[i]) == 0) {
                            current_option = i;
                            break;
                        }
                    }
                }
            } else if (in_prompt && !in_options_seq) {
                if (strcmp(value, "title") == 0) {
                    yaml_parser_parse(&parser, &event);
                    strncpy(config->title, (char *)event.data.scalar.value, sizeof(config->title) - 1);
                    yaml_event_delete(&event);
                } else if (strcmp(value, "timeout") == 0) {
                    yaml_parser_parse(&parser, &event);
                    config->timeout = atoi((char *)event.data.scalar.value);
                    yaml_event_delete(&event);
                }
            } else if (in_options_seq && option_count < MAX_OPTIONS) {
                strncpy(options_list[option_count++], value, 50);
            } else if (current_option >= 0) {
                if (strcmp(value, "type") == 0) {
                    yaml_parser_parse(&parser, &event);
                    yaml_event_delete(&event);
                } else if (strcmp(value, "options") == 0) {
                    // Skip this one
                } else if (strcmp(value, "label") == 0) {
                    yaml_parser_parse(&parser, &event);
                    strncpy(config->options[current_option].label, (char *)event.data.scalar.value, 50);
                    config->num_options = current_option + 1 > config->num_options ? 
                                          current_option + 1 : config->num_options;
                    yaml_event_delete(&event);
                } else if (strcmp(value, "cmd") == 0) {
                    yaml_parser_parse(&parser, &event);
                    strncpy(config->options[current_option].cmd, (char *)event.data.scalar.value, MAX_LINE);
                    yaml_event_delete(&event);
                }
            }
        }
        yaml_event_delete(&event);
    }

    yaml_parser_delete(&parser);
    fclose(file);

    if (config->num_options == 0) {
        log_error("No valid options found in YAML");
        free(config->options);
        return -1;
    }
    return 0;
}

int main() {
    Config config = { "Bootloader", 10, NULL, 0 };
    char path[256];
    snprintf(path, sizeof(path), "%s/.config/bootloader/bootloader.yaml", getenv("HOME"));

    if (parse_yaml(path, &config) != 0) {
        log_error("Falling back to default config due to YAML parsing failure");
        config.num_options = 3;
        config.options = malloc(3 * sizeof(Option));
        strcpy(config.options[0].label, "Fedora 41 (aarch64)");
        strcpy(config.options[0].cmd, "bash $HOME/.config/startx");
        strcpy(config.options[1].label, "Fedora root shell(fallback)");
        strcpy(config.options[1].cmd, "pd sh fedora");
        strcpy(config.options[2].label, "Turn Off Computer");
        strcpy(config.options[2].cmd, "pkill -f termux");
    }

    initscr();
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);

    int rows = LINES, cols = COLS, cursor = 0;
    time_t end_time = time(NULL) + config.timeout;
    timeout(1000);

    while (time(NULL) < end_time) {
        int remaining = end_time - time(NULL);
        draw_menu(rows, cols, cursor, &config, remaining);
        int ch = getch();
        if (ch != ERR) {
            if (ch == KEY_UP && cursor > 0) {
                cursor--;
                end_time = time(NULL) + config.timeout;
            } else if (ch == KEY_DOWN && cursor < config.num_options - 1) {
                cursor++;
                end_time = time(NULL) + config.timeout;
            } else if (ch == '\n') {
                execute_command(cursor, &config);
            }
        }
    }

    execute_command(0, &config);
    free(config.options);
    return 0;
}
