#include <string.h>
#include <stdio.h>

#include "commands.h"
#include "io_helpers.h"
#include "variables.h"
//later on i might want to create a command checker

int var_assignment(char *statement) {
    char *ptr_to_equalsign = strchr(statement, '=');//where the equal sign it
    if (ptr_to_equalsign != NULL) {
        //printf("wowzers\n");
        if (statement[0] == '=') {//if the statement starts with an '='
            display_error("ERROR: Incorrect variable assignment ", statement);
            return -1;
        }

        *ptr_to_equalsign = '\0'; //set the equal sign to null terminator so it splits
        //now statement will only be the statement up until the equal sign not including the equal sign
        ptr_to_equalsign += 1; //now this is gonna be the value of what 

        char expanded_value[MAX_STR_LEN];
        expanded_value[0] = '\0';
        ssize_t expanded_len = 0;
        
        for (char *p = ptr_to_equalsign; *p != '\0' && expanded_len < MAX_STR_LEN;) {
            if (*p == '$') {
                if (*(p + 1) == ' ' || *(p + 1) == '\0') {
                    if (expanded_len + 1 >= MAX_STR_LEN)
                        break;

                    expanded_value[expanded_len++] = '$';
                    expanded_value[expanded_len] = '\0';
                    p++;

                } else {
                    p++; 
                    char *var_start = p;
                    while (*p != '\0' && *p != ' ' && *p != '$') {
                        p++;
                    }
                    
                    size_t var_len = p - var_start;
                    char var_name[var_len + 1];
                    strncpy(var_name, var_start, var_len);
                    var_name[var_len] = '\0';
                    
                    char *var_value = get_var(var_name);
                    ssize_t var_value_len = strnlen(var_value, MAX_STR_LEN);
                    
                    if (expanded_len + var_value_len >= MAX_STR_LEN) {
                        ssize_t chars_leftover = MAX_STR_LEN - expanded_len;
                        strncpy(expanded_value + expanded_len, var_value, chars_leftover);
                        expanded_len += chars_leftover;
                        break;
                    } else {
                        strcpy(expanded_value + expanded_len, var_value);
                        expanded_len += var_value_len;
                    }
                }
            } else {
                char *text_start = p;
                while (*p != '\0' && *p != '$') {
                    p++;
                }
                
                size_t text_len = p - text_start;
                if (expanded_len + text_len >= MAX_STR_LEN) {
                    ssize_t chars_leftover = MAX_STR_LEN - expanded_len;
                    strncpy(expanded_value + expanded_len, text_start, chars_leftover);
                    expanded_len += chars_leftover;
                    break;
                } else {
                    strncpy(expanded_value + expanded_len, text_start, text_len);
                    expanded_len += text_len;
                    expanded_value[expanded_len] = '\0';
                }
            }
        }
        
        if (expanded_len < MAX_STR_LEN) {
            expanded_value[expanded_len] = '\0';
        } else {
            expanded_value[MAX_STR_LEN - 1] = '\0';
        }
        set_var(expanded_value, statement);
        return 1;
    }

    return -1;
}