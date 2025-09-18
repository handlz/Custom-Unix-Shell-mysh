#ifndef __VARIABLES_H__
#define __VARIABLES_H__

typedef struct environment_var { //linked list
    char *value;
    char *name;
    struct environment_var *next; //this is gonna be our pointer to the next link
} Environment_var;

char *get_var(char *name); //getter

void set_var(char *value, char *name); //setter

void free_all_vars(); //when we exit the program

#endif
