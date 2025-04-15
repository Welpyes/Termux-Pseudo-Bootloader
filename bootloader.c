#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <yaml.h>

#define MAX_LINE 256
#define MAX_OPTIONS 10 // Arbitrary max for dynamic options; adjust as needed

// Struct to hold menu options
typedef struct {
    char label[50];
    char cmd[MAX_LINE];
} Option;

// Struct to hold prompt config
typedef struct {
    char title[50];
    int timeout;
    Option *options;
    int num_options;
} Config;

void log_error(const char *msg) {
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/tmp/bootloader.log", getenv("HOME"));
    FILE *log = fopen(log_path, "a");
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
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/tmp/bootloader.log", getenv("HOME"));
    FILE *log = fopen(log_path, "a");
    if (log) {
        fprintf(log, "Executing: %s\n", config->options[cursor].cmd);
        fclose(log);
    } else {
        log_error("Failed to open log file for writing");
    }
    int result = system(config->options[cursor].cmd);
    if (result != 0) {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Command '%s' failed with exit code %d", 
                 config->options[cursor].cmd, result);
        log_error(err_msg);
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
    int done = 0, in_prompt = 0, in_options_seq = 0, in_option_section = -1;
    char option_sections[MAX_OPTIONS][50];
    int option_count = 0;
    char log_msg[256];

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
            snprintf(log_msg, sizeof(log_msg), "YAML parsing error at line %zu: %s", 
                     parser.problem_mark.line + 1, parser.problem);
            log_error(log_msg);
            break;
        }

        switch (event.type) {
            case YAML_STREAM_END_EVENT:
                done = 1;
                snprintf(log_msg, sizeof(log_msg), "End of YAML stream");
                log_error(log_msg);
                break;
            case YAML_MAPPING_START_EVENT:
                snprintf(log_msg, sizeof(log_msg), "Start mapping");
                log_error(log_msg);
                break;
            case YAML_MAPPING_END_EVENT:
                snprintf(log_msg, sizeof(log_msg), "End mapping");
                log_error(log_msg);
                if (in_prompt && !in_options_seq) in_prompt = 0;
                else if (in_option_section >= 0) in_option_section = -1;
                break;
            case YAML_SEQUENCE_START_EVENT:
                if (in_prompt) in_options_seq = 1;
                snprintf(log_msg, sizeof(log_msg), "Start sequence");
                log_error(log_msg);
                break;
            case YAML_SEQUENCE_END_EVENT:
                if (in_options_seq) in_options_seq = 0;
                snprintf(log_msg, sizeof(log_msg), "End sequence");
                log_error(log_msg);
                break;
            case YAML_SCALAR_EVENT:
                snprintf(log_msg, sizeof(log_msg), "Scalar: %s", event.data.scalar.value);
                log_error(log_msg);

                if (!in_prompt && in_option_section == -1 && !in_options_seq) {
                    if (strcmp((char *)event.data.scalar.value, "prompt") == 0) {
                        in_prompt = 1;
                        snprintf(log_msg, sizeof(log_msg), "Entering prompt section");
                        log_error(log_msg);
                    } else {
                        for (int i = 0; i < option_count; i++) {
                            if (strcmp((char *)event.data.scalar.value, option_sections[i]) == 0) {
                                in_option_section = i;
                                snprintf(log_msg, sizeof(log_msg), "Entering option section %d: %s", 
                                         i, option_sections[i]);
                                log_error(log_msg);
                                break;
                            }
                        }
                    }
                } else if (in_prompt && !in_options_seq) {
                    if (strcmp((char *)event.data.scalar.value, "title") == 0) {
                        yaml_parser_parse(&parser, &event);
                        strncpy(config->title, (char *)event.data.scalar.value, sizeof(config->title) - 1);
                        snprintf(log_msg, sizeof(log_msg), "Set title: %s", config->title);
                        log_error(log_msg);
                        yaml_event_delete(&event);
                    } else if (strcmp((char *)event.data.scalar.value, "timeout") == 0) {
                        yaml_parser_parse(&parser, &event);
                        config->timeout = atoi((char *)event.data.scalar.value);
                        snprintf(log_msg, sizeof(log_msg), "Set timeout: %d", config->timeout);
                        log_error(log_msg);
                        yaml_event_delete(&event);
                    } else if (strcmp((char *)event.data.scalar.value, "options") == 0) {
                        // Next event will be sequence start
                    }
                } else if (in_options_seq && option_count < MAX_OPTIONS) {
                    strncpy(option_sections[option_count], (char *)event.data.scalar.value, 
                            sizeof(option_sections[option_count]) - 1);
                    snprintf(log_msg, sizeof(log_msg), "Added option section %d: %s", 
                             option_count, option_sections[option_count]);
                    log_error(log_msg);
                    option_count++;
                } else if (in_option_section >= 0) {
                    if (strcmp((char *)event.data.scalar.value, "type") == 0) {
                        yaml_parser_parse(&parser, &event);
                        yaml_event_delete(&event); // Skip type value
                    } else if (strcmp((char *)event.data.scalar.value, "options") == 0) {
                        // Next event will be mapping start
                    } else if (strcmp((char *)event.data.scalar.value, "label") == 0) {
                        yaml_parser_parse(&parser, &event);
                        strncpy(config->options[in_option_section].label, (char *)event.data.scalar.value, 
                                sizeof(config->options[in_option_section].label) - 1);
                        snprintf(log_msg, sizeof(log_msg), "Set label for %s: %s", 
                                 option_sections[in_option_section], config->options[in_option_section].label);
                        log_error(log_msg);
                        config->num_options = in_option_section + 1 > config->num_options ? 
                                              in_option_section + 1 : config->num_options;
                        yaml_event_delete(&event);
                    } else if (strcmp((char *)event.data.scalar.value, "cmd") == 0) {
                        yaml_parser_parse(&parser, &event);
                        strncpy(config->options[in_option_section].cmd, (char *)event.data.scalar.value, 
                                sizeof(config->options[in_option_section].cmd) - 1);
                        snprintf(log_msg, sizeof(log_msg), "Set cmd for %s: %s", 
                                 option_sections[in_option_section], config->options[in_option_section].cmd);
                        log_error(log_msg);
                        yaml_event_delete(&event);
                    }
                }
                break;
            default:
                break;
        }
        yaml_event_delete(&event);
    }

    yaml_parser_delete(&parser);
    fclose(file);
    snprintf(log_msg, sizeof(log_msg), "Parsed %d options", config->num_options);
    log_error(log_msg);
    if (config->num_options == 0) {
        log_error("No valid options found in YAML");
        free(config->options);
        return -1;
    }
    return 0;
}

int main() {
    Config config = { .title = "Bootloader", .timeout = 10, .options = NULL, .num_options = 0 };
    char config_path[256];
    snprintf(config_path, sizeof(config_path), "%s/.config/bootloader/bootloader.yaml", getenv("HOME"));

    if (parse_yaml(config_path, &config) != 0) {
        log_error("Falling back to default config due to YAML parsing failure");
        config.num_options = 3;
        config.options = malloc(config.num_options * sizeof(Option));
        strcpy(config.options[0].label, "Fedora 41 (aarch64)"); strcpy(config.options[0].cmd, "bash $HOME/.config/startx");
        strcpy(config.options[1].label, "Fedora root shell(fallback)"); strcpy(config.options[1].cmd, "pd sh fedora");
        strcpy(config.options[2].label, "Turn Off Computer"); strcpy(config.options[2].cmd, "pkill -f termux");
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
            switch (ch) {
                case KEY_UP:
                    if (cursor > 0) cursor--;
                    end_time = time(NULL) + config.timeout;
                    break;
                case KEY_DOWN:
                    if (cursor < config.num_options - 1) cursor++;
                    end_time = time(NULL) + config.timeout;
                    break;
                case '\n':
                    execute_command(cursor, &config);
                    break;
            }
        }
    }

    execute_command(0, &config);
    free(config.options);
    return 0;
}
