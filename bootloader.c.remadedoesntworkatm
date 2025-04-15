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
    char name[50];
    char command[MAX_LINE];
} MenuOption;

// config struct
typedef struct {
    char title[50];
    int wait_time;
    MenuOption *options_list;
    int num_items;
} BootConfig;

// logger
void write_error(char *error_msg) {
    char log_file[256];
    sprintf(log_file, "%s/tmp/boot.log", getenv("HOME"));
    FILE *f = fopen(log_file, "a");
    if (f != NULL) {
        time_t current_time = time(NULL);
        fprintf(f, "[%s] Error: %s\n", ctime(&current_time), error_msg);
        fclose(f);
    }
}

// center text func
void put_text_in_middle(int row, int width, char *text) {
    int col = (width - strlen(text)) / 2;
    mvprintw(row, col, "%s", text);
}

   /*a diagram so i could visualise
     *      Boot Menu
     *    
     *      option 1 // default and highlighted first
     *      option 2
     *      option 3
     *
     *      timer: $time
   */

void show_menu(int height, int width, int selected, BootConfig *cfg, int seconds_left) {
    clear();
    int first_row = (height - cfg->num_items - 4) / 2;
    put_text_in_middle(first_row - 2, width, cfg->title);

    for (int i = 0; i < cfg->num_items; i++) {
        char temp[50];
        sprintf(temp, " %s ", cfg->options_list[i].name);
        if (i == selected) {
            attron(COLOR_PAIR(1));
            put_text_in_middle(first_row + i, width, temp);
            attroff(COLOR_PAIR(1));
        } else {
            put_text_in_middle(first_row + i, width, temp);
        }
    }

    char time_text[20];
    sprintf(time_text, "Timeout: %ds", seconds_left);
    put_text_in_middle(first_row + cfg->num_items + 1, width, time_text);
    refresh();
}

void run_option(int choice, BootConfig *cfg) {
    endwin();
    char log_file[256];
    sprintf(log_file, "%s/tmp/boot.log", getenv("HOME"));
    FILE *f = fopen(log_file, "a");
    if (f) {
        fprintf(f, "Running: %s\n", cfg->options_list[choice].command);
        fclose(f);
    } else {
        write_error("Couldn’t write to log file");
    }
    int result = system(cfg->options_list[choice].command);
    if (result != 0) {
        char err_msg[256];
        sprintf(err_msg, "Command %s failed, code %d", cfg->options_list[choice].command, result);
        write_error(err_msg);
    }
    exit(0);
}

// yaml parser basically
/* btw i cant be arsed to do it with switches again
 */
int read_yaml(char *file_path, BootConfig *cfg) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        write_error("Can’t open YAML file"); // just error checking
        return -1;
    }

    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        write_error("YAML parser init failed"); // another error check (its a good practice)
        fclose(file);
        return -1;
    }
    yaml_parser_set_input_file(&parser, file);

    yaml_event_t event;
    int finished = 0;
    int in_menu_section = 0;
    int in_options_list = 0;
    int current_item = -1;
    char option_names[MAX_OPTIONS][50];
    int option_count = 0;

    cfg->options_list = malloc(MAX_OPTIONS * sizeof(MenuOption));
    if (!cfg->options_list) {
        write_error("No memory for options");
        fclose(file);
        yaml_parser_delete(&parser);
        return -1;
    }
    cfg->num_items = 0;

    while (!finished) {
        if (!yaml_parser_parse(&parser, &event)) {
            char err[256];
            sprintf(err, "YAML error at line %zu: %s", parser.problem_mark.line + 1, parser.problem);
            write_error(err);
            break;
        }

        if (event.type == YAML_STREAM_END_EVENT) {
            finished = 1;
        } else if (event.type == YAML_MAPPING_START_EVENT) {
            // do nothing
        } else if (event.type == YAML_MAPPING_END_EVENT) {
            if (in_menu_section && !in_options_list) in_menu_section = 0;
            else if (current_item >= 0) current_item = -1;
        } else if (event.type == YAML_SEQUENCE_START_EVENT) {
            if (in_menu_section) in_options_list = 1;
        } else if (event.type == YAML_SEQUENCE_END_EVENT) {
            if (in_options_list) in_options_list = 0;
        } else if (event.type == YAML_SCALAR_EVENT) {
            char *val = (char *)event.data.scalar.value;

            if (!in_menu_section && current_item == -1 && !in_options_list) {
                if (strcmp(val, "prompt") == 0) {
                    in_menu_section = 1;
                } else {
                    for (int i = 0; i < option_count; i++) {
                        if (strcmp(val, option_names[i]) == 0) {
                            current_item = i;
                            break;
                        }
                    }
                }
            } else if (in_menu_section && !in_options_list) {
                if (strcmp(val, "title") == 0) {
                    yaml_parser_parse(&parser, &event);
                    strcpy(cfg->title, (char *)event.data.scalar.value);
                    yaml_event_delete(&event);
                } else if (strcmp(val, "timeout") == 0) {
                    yaml_parser_parse(&parser, &event);
                    cfg->wait_time = atoi((char *)event.data.scalar.value);
                    yaml_event_delete(&event);
                }
            } else if (in_options_list && option_count < MAX_OPTIONS) {
                strcpy(option_names[option_count++], val);
            } else if (current_item >= 0) {
                if (strcmp(val, "type") == 0) {
                    yaml_parser_parse(&parser, &event);
                    yaml_event_delete(&event);
                } else if (strcmp(val, "options") == 0) {
                    // skip
                } else if (strcmp(val, "label") == 0) {
                    yaml_parser_parse(&parser, &event);
                    strcpy(cfg->options_list[current_item].name, (char *)event.data.scalar.value);
                    cfg->num_items = current_item + 1 > cfg->num_items ? current_item + 1 : cfg->num_items;
                    yaml_event_delete(&event);
                } else if (strcmp(val, "cmd") == 0) {
                    yaml_parser_parse(&parser, &event);
                    strcpy(cfg->options_list[current_item].command, (char *)event.data.scalar.value);
                    yaml_event_delete(&event);
                }
            }
        }
        yaml_event_delete(&event);
    }

    yaml_parser_delete(&parser);
    fclose(file);

    if (cfg->num_items == 0) {
        write_error("No options in YAML");
        free(cfg->options_list);
        return -1;
    }
    return 0;
}

// shits bout to go down 
int main() {
    BootConfig config;
    strcpy(config.title, "Boot Menu"); // default
    config.wait_time = 10;
    config.options_list = NULL;
    config.num_items = 0;

    char config_file[256];
    sprintf(config_file, "%s/.config/bootloader/bootloader.yaml", getenv("HOME"));

    if (read_yaml(config_file, &config) != 0) {
        write_error("Using default options"); // error checker (its a good practice ik)
        config.num_items = 3;
        config.options_list = malloc(3 * sizeof(MenuOption));
        // fallback stuff if for some reason its fucked
        strcpy(config.options_list[0].name, "Proot-Distro Os");
        strcpy(config.options_list[0].command, "bash $HOME/.config/bootloader/boot");
        strcpy(config.options_list[1].name, "Proot Shell");
        strcpy(config.options_list[1].command, "pd list");
        strcpy(config.options_list[2].name, "Shutdown");
        strcpy(config.options_list[2].command, "pkill -f termux");
    }

    // Initialize NCurses 
    initscr();
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);

    /*Fix: Set a 100ms timeout for getch()
     * i found this discussed i stackoverflow but its kinda loose and they didnt give out an answer
     * so this sht i made myself ayways :skull:
  */
    timeout(100);

    int screen_height = LINES;
    int screen_width = COLS;
    int current_choice = 0;
    time_t end = time(NULL) + config.wait_time;

    while (time(NULL) < end) {
        int secs = end - time(NULL);
        show_menu(screen_height, screen_width, current_choice, &config, secs);
        int key = getch();
        if (key != ERR) {
            if (key == KEY_UP && current_choice > 0) {
                current_choice--;
                end = time(NULL) + config.wait_time; // Reset timer
            }
            if (key == KEY_DOWN && current_choice < config.num_items - 1) {
                current_choice++;
                end = time(NULL) + config.wait_time; // Reset timer
            }
            if (key == '\n') {
                run_option(current_choice, &config);
            }
        }
    }

    // Timeout reached, run default option
    run_option(0, &config);
    free(config.options_list); // Note: Unreachable due to exit(0) in run_option
    return 0;
}
