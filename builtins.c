#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <signal.h>


#include "builtins.h"
#include "io_helpers.h"
#include "variables.h"
#include "network.h"






// ====== Command execution =====


/* Return: index of builtin or -1 if cmd doesn't match a builtin
 */
bn_ptr check_builtin(const char *cmd) {
    ssize_t cmd_num = 0;
    while (cmd_num < BUILTINS_COUNT &&
           strncmp(BUILTINS[cmd_num], cmd, MAX_STR_LEN) != 0) {
        cmd_num += 1;
    }
    return BUILTINS_FN[cmd_num];
}




// ===== Builtins =====


/* Prereq: tokens is a NULL terminated sequence of strings.
 * Return 0 on success and -1 on error ... but there are no errors on echo.
 */
ssize_t bn_echo(char **tokens) {
    ssize_t index;
    ssize_t total_output_len = 0;
for (index = 1; tokens[index] != NULL; index++) {
        if (index > 1) {//if its not the first word
            if (total_output_len + 1 >= MAX_STR_LEN)
                break;
            display_message(" "); //add space if not at first word
            total_output_len++;
        }
       
        char *token = tokens[index];//current token
        for (char *p = token; *p != '\0' && total_output_len < MAX_STR_LEN; ) {//looops thorugh every character and stops when we reach null terminator or our output too long
            if (*p == '$') {//check if the current character is a $
                if (*(p + 1) == ' ' || *(p + 1) == '\0') {//if the $ is immediately followed by a space or end-of-string
                    if (total_output_len + 1 >= MAX_STR_LEN)
                        break;
                    display_message("$");  
                    total_output_len++;
                    p++;


                } else {//variable expansion
                    p++;//skip the $
                    char *var_start = p;


                    while (*p != '\0' && *p != ' ' && *p != '$') {
                        p++; //points to end of variable
                    }


                    size_t var_len = p - var_start;
                    char var_name[var_len + 1];
                    strncpy(var_name, var_start, var_len);//var_name now has the variable
                    var_name[var_len] = '\0';
                    char *var_value = get_var(var_name);
                    ssize_t var_value_len = strnlen(var_value, MAX_STR_LEN); //check len


                    if (total_output_len + var_value_len >= MAX_STR_LEN) {
                        ssize_t chars_leftover = MAX_STR_LEN - total_output_len;//we going over the len limit so we only display some
                        char truncated[chars_leftover + 1];
                        strncpy(truncated, var_value, chars_leftover);
                        truncated[chars_leftover] = '\0';
                        display_message(truncated);
                        total_output_len += chars_leftover;
                        break;
                    } else {
                        //else print the full variable
                        display_message(var_value);
                        total_output_len += var_value_len;
                    }
                }
            } else {//for normal text
                char *text_start = p;
                while (*p != '\0' && *p != '$') {
                    p++;//points to end of text
                }
                size_t text_len = p - text_start;
                if (total_output_len + text_len >= MAX_STR_LEN) {//if over limit
                    ssize_t chars_leftover = MAX_STR_LEN - total_output_len;
                    char truncated[chars_leftover + 1];
                    strncpy(truncated, text_start, chars_leftover);
                    truncated[chars_leftover] = '\0';
                    display_message(truncated);
                    total_output_len += chars_leftover;
                    break;
                } else {//print rest
                    char temp[text_len + 1];
                    strncpy(temp, text_start, text_len);
                    temp[text_len] = '\0';
                    display_message(temp);
                    total_output_len += text_len;
                }
            }
        }
    }


    // After processing all tokens, output a newline (if there is room).
    if (total_output_len < MAX_STR_LEN) {
        display_message("\n");
    }
    return 0;
}




ssize_t recursive_ls_traversal(int is_recursive, int deepest, int current_depth, char *substring, char *path) {
    struct dirent *entry;
    struct stat entry_stat;
    DIR *directory;
    int result = 0;


    if (path == NULL) {
        display_error("ERROR: Null path", "");
        return -1;
    }


    directory = opendir(path);
    if (directory == NULL) {
        display_error("ERROR: Invalid path", "");
        return -1;
    }


    struct dirent *entries[1000];
    int entry_count = 0;
   
    while ((entry = readdir(directory)) != NULL && entry_count < 1000) {
        entries[entry_count++] = entry;
    }
   
    if (deepest == -1 || current_depth <= deepest - 1) {
        for (int i = 0; i < entry_count; i++) {
            char *name = entries[i]->d_name;
           
            if (substring == NULL || strstr(name, substring) != NULL) {
                display_message(name);
                display_message("\n");
            }
        }
    }
   
    if (is_recursive && (deepest == -1 || current_depth < deepest)) {
        for (int i = 0; i < entry_count; i++) {
            char *name = entries[i]->d_name;
           
            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
                continue;
            }
           
            char full_path[PATH_MAX] = {0};
            int path_len = strlen(path);
           
            if (path_len + strlen(name) + 2 > PATH_MAX) {
                closedir(directory);
                display_error("ERROR: Path too long", "");
                return -1;
            }
           
            strcpy(full_path, path);
           
            if (path_len > 0 && path[path_len-1] != '/') {
                full_path[path_len] = '/';
                full_path[path_len + 1] = '\0';
            }
           
            strcat(full_path, name);
           
            if (stat(full_path, &entry_stat) == 0 && S_ISDIR(entry_stat.st_mode)) {
                result = recursive_ls_traversal(is_recursive, deepest, current_depth + 1, substring, full_path);
                if (result == -1) {
                    closedir(directory);
                    return -1;
                }
            }
        }
    }
   
    if (closedir(directory) != 0) {
        display_error("ERROR: Could not close directory", "");
        return -1;
    }
   
    return 0;
}










//make sure after --d there is a number
//does --rec --rec cancel the recusion out?
//what happens for two --f's does that cause an error?        from my implementation i say error
//what id the first argument has to be a path or else it's a bust
//for --d, go as far as you can dont error out if the num is too big
//if ls is callled on a file just print the file
//should i add /n to my display_messae/error's


ssize_t bn_ls(char **tokens) {
    char *path = NULL;
    char *substring = NULL;
    int is_recursive = 0;
    int depth = -1;


    for (int index = 1; tokens[index] != NULL; index++) {
        if (strcmp(tokens[index], "--rec") == 0) {
            is_recursive = 1;
        } else if (strcmp(tokens[index], "--d") == 0) {
            if (tokens[index + 1] == NULL) {
                display_error("ERROR: no value provided after --d flag", "");
                return -1;
            }


            char *end_ptr;
            long num = strtol(tokens[index + 1], &end_ptr, 10);
            if (( num < 0 )|| *end_ptr != '\0' || end_ptr == tokens[index + 1]) {
                display_error("ERROR: Invalid depth", "");
                return -1;
            }
           
            depth = (int) num;
            index++;
                     
        } else if (strcmp(tokens[index], "--f") == 0) {
            if (tokens[index + 1] == NULL) {
                display_error("ERROR: no value provided after --f flag", "");
                return -1;
            }
           
            if (tokens[index + 1][0] == '$') {
                char *var_name = tokens[index + 1] + 1;
                char *var_value = get_var(var_name);
                if (var_value && strlen(var_value) > 0) {
                    substring = var_value;
                } else {
                    substring = "";
                }
            } else {
                substring = tokens[index + 1];
            }
           
            index++;
        } else if (path == NULL) {
            if (tokens[index][0] == '$') {
                char *var_name = tokens[index] + 1;
                char *var_value = get_var(var_name);
                if (var_value && strlen(var_value) > 0) {
                    path = var_value;
                } else {
                    path = "";
                }
            } else {
                path = tokens[index];
            }
        } else {
            display_error("ERROR: Too many arguments: ls takes a single path", "");
            return -1;
        }
    }


    if (is_recursive == 0 && depth != -1) {
        display_error("ERROR: --d is provided and --rec is not", "");
        return -1;
    }


    if (path == NULL) {
        path = ".";
    }


    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        display_error("ERROR: Invalid path", "");
        return -1;
    }
   
    if ((S_ISDIR(path_stat.st_mode)) == 0) {
        display_error("ERROR: Not a directory", "");
        return -1;
    }


    int recursive_ls_traversal_result;
    if ((recursive_ls_traversal_result = recursive_ls_traversal(is_recursive, depth, 0, substring, path)) == -1) {
        display_error("ERROR: Builtin failed: ls", "");
        return -1;
    }
    return 0;
}


ssize_t bn_cd(char **tokens) {
    if (tokens[1] == NULL) {
        display_error("ERROR: No path provided", "");
        return -1;
    }
    char *path = tokens[1];
   
    if (tokens[2] != NULL) {
        display_error("ERROR: Too many arguments: cd takes a single path", "");
        return -1;
    }
   
    if (strcmp(path, "...") == 0) {
        path = "../..";
    } else if (strcmp(path, "....") == 0) {
        path = "../../..";
    }


    if (chdir(path) != 0) {
        display_error("ERROR: Invalid path", "");
        return -1;
    }
   
    return 0;
}


ssize_t bn_cat(char **tokens) {
    FILE *file = NULL;
    int read_from_stdin = 0;
   
    if (tokens[1] == NULL) {
        file = stdin;
        read_from_stdin = 1;
    } else {
        if ((file = fopen(tokens[1], "r")) == NULL) {
            display_error("ERROR: Cannot open file", "");
            return -1;
        }
       
        if (tokens[2] != NULL) {
            display_error("ERROR: Too many arguments: cat takes a single file", "");
            return -1;
        }
    }


    char buffer[MAX_STR_LEN + 1];
    while (fgets(buffer, MAX_STR_LEN + 1, file) != NULL) {
        display_message(buffer);
    }
   
    if (!read_from_stdin && fclose(file) != 0) {
        display_error("ERROR: Failed to close file", "");
        return -1;
    }
   
    return 0;
}


ssize_t bn_wc(char **tokens) {
    FILE *file = NULL;
    int read_from_stdin = 0;
   
    if (tokens[1] == NULL) {
        // No filename provided, read from stdin
        file = stdin;
        read_from_stdin = 1;
    } else {
        // Open the specified file
        if ((file = fopen(tokens[1], "r")) == NULL) {
            display_error("ERROR: Cannot open file", "");
            return -1;
        }
       
        if (tokens[2] != NULL) {
            display_error("ERROR: Too many arguments: wc takes a single file", "");
            return -1;
        }
    }


    int in_word = 0;
    int character;
    int word_count = 0;
    int character_count = 0;
    int newline_count = 0;


    for (character = fgetc(file); character != EOF; character = fgetc(file)) {
        character_count++;
       
        if (character == '\n') {
            newline_count++;
        }
       
        if (character == ' ' || character == '\t' || character == '\n') {
            in_word = 0;
        } else if (in_word == 0) {
            in_word = 1;
            word_count++;
        }
    }
   
    if (!read_from_stdin && fclose(file) != 0) {
        display_error("ERROR: Failed to close file", "");
        return -1;
    }
   
    char buffer[MAX_STR_LEN];


    strcpy(buffer, "word count ");
    char word_str[32];
    sprintf(word_str, "%d", word_count);
    strncat(buffer, word_str, MAX_STR_LEN - strlen(buffer) - 1);
    strncat(buffer, "\n", MAX_STR_LEN - strlen(buffer) - 1);
    display_message(buffer);


    strcpy(buffer, "character count ");
    char character_str[32];
    sprintf(character_str, "%d", character_count);
    strncat(buffer, character_str, MAX_STR_LEN - strlen(buffer) - 1);
    strncat(buffer, "\n", MAX_STR_LEN - strlen(buffer) - 1);
    display_message(buffer);


    strcpy(buffer, "newline count ");
    char newline_str[32];
    sprintf(newline_str, "%d", newline_count);
    strncat(buffer, newline_str, MAX_STR_LEN - strlen(buffer) - 1);
    strncat(buffer, "\n", MAX_STR_LEN - strlen(buffer) - 1);
    display_message(buffer);
   
    return 0;
}

ssize_t bn_kill(char **tokens) {
    if (tokens[1] == NULL) {
        display_error("ERROR: The process does not exist", "");
        return -1;
    }
    
    pid_t target_pid;
    if (tokens[1][0] == '$') {
        char *var_name = tokens[1] + 1;
        char *var_value = get_var(var_name);
        if (var_value && strlen(var_value) > 0) {
            char *endptr;
            target_pid = strtol(var_value, &endptr, 10);
            if (*endptr != '\0') {
                display_error("ERROR: The process does not exist", "");
                return -1;
            }
        } else {
            display_error("ERROR: The process does not exist", "");
            return -1;
        }
    } else {
        char *endptr;
        target_pid = strtol(tokens[1], &endptr, 10);
        if (*endptr != '\0') {
            display_error("ERROR: The process does not exist", "");
            return -1;
        }
    }
    
    int target_signal = SIGTERM;
    
    if (tokens[2] != NULL) {
        if (tokens[2][0] == '$') {
            char *var_name = tokens[2] + 1;
            char *var_value = get_var(var_name);
            if (var_value && strlen(var_value) > 0) {
                char *endptr;
                target_signal = strtol(var_value, &endptr, 10);
                if (*endptr != '\0') {
                    display_error("ERROR: Invalid signal specified", "");
                    return -1;
                }
            } else {
                display_error("ERROR: Invalid signal specified", "");
                return -1;
            }
        } else {
            char *endptr;
            target_signal = strtol(tokens[2], &endptr, 10);
            if (*endptr != '\0') {
                display_error("ERROR: Invalid signal specified", "");
                return -1;
            }
        }
        
        if (target_signal < 1 || target_signal > 31) {
            display_error("ERROR: Invalid signal specified", "");
            return -1;
        }
    }
    
    if (tokens[3] != NULL) {
        display_error("ERROR: Too many arguments: kill takes a pid and optional signal", "");
        return -1;
    }
    
    if (kill(target_pid, 0) == -1) {
        display_error("ERROR: The process does not exist", "");
        return -1;
    }
    
    if (kill(target_pid, target_signal) == -1) {
        display_error("ERROR: Failed to send signal to process", "");
        return -1;
    }
    
    return 0;
}

ssize_t bn_ps(char **tokens) {
    if (tokens[1] != NULL) {
        display_error("ERROR: ps takes no arguments", "");
        return -1;
    }
    display_background_processes();
    return 0;
}






ssize_t bn_start_server(char **tokens) {
    if (tokens[1] == NULL) {
        display_error("ERROR: No port provided", "");
        return -1;
    }
    
    long port;
    if (tokens[1][0] == '$') {
        char *var_name = tokens[1] + 1;
        char *var_value = get_var(var_name);
        
        if (var_value == NULL || strlen(var_value) == 0) {
            display_error("ERROR: Variable not defined", "");
            return -1;
        }
        
        char *endptr;
        port = strtol(var_value, &endptr, 10);
        
        if (*endptr != '\0' || port <= 0 || port > 65535) {
            display_error("ERROR: Invalid port number", "");
            return -1;
        }
    } else {
        char *endptr;
        port = strtol(tokens[1], &endptr, 10);
        
        if (*endptr != '\0' || port <= 0 || port > 65535) {
            display_error("ERROR: Invalid port number", "");
            return -1;
        }
    }
    
    if (tokens[2] != NULL) {
        display_error("ERROR: Too many arguments", "");
        return -1;
    }
    
    if (start_server((int)port) == -1) {
        display_error("ERROR: Builtin failed: start-server", "");
        return -1;
    }
    
    return 0;
}

ssize_t bn_close_server(char **tokens) {
    if (tokens[1] != NULL) {
        display_error("ERROR: close-server takes no arguments", "");
        return -1;
    }
    
    close_server();
    return 0;
}

ssize_t bn_send(char **tokens) {
    if (tokens[1] == NULL) {
        display_error("ERROR: No port provided", "");
        return -1;
    }
    
    long port;
    if (tokens[1][0] == '$') {
        char *var_name = tokens[1] + 1;
        char *var_value = get_var(var_name);
        
        if (var_value == NULL || strlen(var_value) == 0) {
            display_error("ERROR: Variable not defined", "");
            return -1;
        }
        
        char *endptr;
        port = strtol(var_value, &endptr, 10);
        
        if (*endptr != '\0' || port <= 0 || port > 65535) {
            display_error("ERROR: Invalid port number", "");
            return -1;
        }
    } else {
        char *endptr;
        port = strtol(tokens[1], &endptr, 10);
        
        if (*endptr != '\0' || port <= 0 || port > 65535) {
            display_error("ERROR: Invalid port number", "");
            return -1;
        }
    }
    
    if (tokens[2] == NULL) {
        display_error("ERROR: No hostname provided", "");
        return -1;
    }
    
    char *hostname;
    if (tokens[2][0] == '$') {
        char *var_name = tokens[2] + 1;
        hostname = get_var(var_name);
        
        if (hostname == NULL || strlen(hostname) == 0) {
            display_error("ERROR: Hostname variable not defined", "");
            return -1;
        }
    } else {
        hostname = tokens[2];
    }
    
    if (tokens[3] == NULL) {
        display_error("ERROR: No message provided", "");
        return -1;
    }
    
    char message[MAX_STR_LEN] = "";
    
    for (int i = 3; tokens[i] != NULL; i++) {
        if (i > 3) {
            strncat(message, " ", MAX_STR_LEN - strlen(message) - 1);
        }
        
        if (tokens[i][0] == '$') {
            char *var_name = tokens[i] + 1;
            char *var_value = get_var(var_name);
            
            if (var_value != NULL) {
                strncat(message, var_value, MAX_STR_LEN - strlen(message) - 1);
            }
        } else {
            strncat(message, tokens[i], MAX_STR_LEN - strlen(message) - 1);
        }
    }
    
    if (send_message((int)port, hostname, message) == -1) {
        display_error("ERROR: Builtin failed: send", "");
        return -1;
    }
    
    return 0;
}

ssize_t bn_start_client(char **tokens) {
    if (tokens[1] == NULL) {
        display_error("ERROR: No port provided", "");
        return -1;
    }
    
    long port;
    if (tokens[1][0] == '$') {
        char *var_name = tokens[1] + 1;
        char *var_value = get_var(var_name);
        
        if (var_value == NULL || strlen(var_value) == 0) {
            display_error("ERROR: Variable not defined", "");
            return -1;
        }
        
        char *endptr;
        port = strtol(var_value, &endptr, 10);
        
        if (*endptr != '\0' || port <= 0 || port > 65535) {
            display_error("ERROR: Invalid port number", "");
            return -1;
        }
    } else {
        char *endptr;
        port = strtol(tokens[1], &endptr, 10);
        
        if (*endptr != '\0' || port <= 0 || port > 65535) {
            display_error("ERROR: Invalid port number", "");
            return -1;
        }
    }
    
    if (tokens[2] == NULL) {
        display_error("ERROR: No hostname provided", "");
        return -1;
    }
    
    char *hostname;
    if (tokens[2][0] == '$') {
        char *var_name = tokens[2] + 1;
        hostname = get_var(var_name);
        
        if (hostname == NULL || strlen(hostname) == 0) {
            display_error("ERROR: Hostname variable not defined", "");
            return -1;
        }
    } else {
        hostname = tokens[2];
    }
    
    if (tokens[3] != NULL) {
        display_error("ERROR: Too many arguments", "");
        return -1;
    }
    
    if (start_client((int)port, hostname) == -1) {
        display_error("ERROR: Builtin failed: start-client", "");
        return -1;
    }
    
    return 0;
}