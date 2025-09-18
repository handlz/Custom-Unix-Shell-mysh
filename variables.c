#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "variables.h"

static Environment_var *environment_variables = NULL; //this is gonna be the front of the linked list

char *get_var(char *name) {
    Environment_var *curr = environment_variables; //start at the front of the LL

    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) { //check to see if name is in LL
            return curr->value;
        }
        curr = curr->next;
    }
    return "";
}

void set_var(char *value, char *name) {
    Environment_var *curr = environment_variables;

    while (curr!= NULL) {//checks to see if we already defined the name, if so we erase the previous value and replace it
        if (strcmp(curr->name, name) == 0) {
            free(curr->value);
            curr->value = strdup(value); //duplicates the string {value} and returns a pointer to it
            return;
        }
        curr = curr->next;
    }
    //we make a new variable cuz we dont have this one
    Environment_var *new_var;
    new_var = malloc(sizeof(Environment_var)); //set space for a new sruct
    new_var->value = strdup(value);
    new_var->name = strdup(name);
    new_var->next = environment_variables; //since we dont care about order we just make this the new front of the LL
    environment_variables = new_var;
}

void free_all_vars() {
    Environment_var *curr = environment_variables;
    while (curr != NULL) {
        Environment_var *next = curr->next; //store the next one cuz we freeing this one
        free(curr->value);
        free(curr->name);
        free(curr);
        curr = next;
    }
    environment_variables = NULL; //reset the head
}

