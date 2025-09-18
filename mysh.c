#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>


#include "builtins.h"
#include "io_helpers.h"
#include "commands.h"
#include "variables.h"
#include "network.h"


char *prompt = "mysh$ "; // TODO Step 1, Uncomment this.

typedef struct process_list {
    pid_t pid;
    char *cmd_name;
    struct process_list *next;
} ProcessList;

typedef struct bg_job {
    int job_id;
    pid_t process_id;
    char *command_text;
    struct bg_job *next_job;
    ProcessList *processes;
} BackgroundJob;


BackgroundJob *jobs_list = NULL;
int current_job_id = 1;

void add_process_to_job(int job_id, pid_t pid, char *cmd_name) {
    BackgroundJob *job = jobs_list;
    while (job != NULL && job->job_id != job_id) {
        job = job->next_job;
    }
    
    if (job == NULL) {
        return;
    }
    
    ProcessList *new_process = malloc(sizeof(ProcessList));
    if (new_process == NULL) {
        return;
    }
    
    new_process->pid = pid;
    new_process->cmd_name = strdup(cmd_name);
    new_process->next = job->processes;
    job->processes = new_process;
}

void register_background_job(pid_t pid, char *cmd) {
    BackgroundJob *new_job = malloc(sizeof(BackgroundJob));
    if (new_job == NULL) {
        display_error("ERROR: Failed to allocate memory for job tracking", "");
        return;
    }
   
    new_job->job_id = current_job_id++;
    new_job->process_id = pid;
    char cleaned_cmd[MAX_STR_LEN];
    strncpy(cleaned_cmd, cmd, MAX_STR_LEN - 1);
    cleaned_cmd[MAX_STR_LEN - 1] = '\0';
    
    // Remove trailing whitespace
    size_t len = strlen(cleaned_cmd);
    while (len > 0 && (cleaned_cmd[len-1] == ' ' || cleaned_cmd[len-1] == '\t')) {
        cleaned_cmd[--len] = '\0';
    }
    new_job->command_text = strdup(cleaned_cmd);
   
    if (new_job->command_text == NULL) {
        free(new_job);
        display_error("ERROR: Failed to store command text", "");
        return;
    }

    new_job->processes = NULL;
    char cmd_name_buf[MAX_STR_LEN];
    strncpy(cmd_name_buf, cleaned_cmd, MAX_STR_LEN - 1);
    cmd_name_buf[MAX_STR_LEN - 1] = '\0';
    
    char *cmd_name = strtok(cmd_name_buf, " \t|");
    if (cmd_name != NULL) {
        ProcessList *first_process = malloc(sizeof(ProcessList));
        if (first_process != NULL) {
            first_process->pid = pid;
            first_process->cmd_name = strdup(cmd_name);
            first_process->next = NULL;
            new_job->processes = first_process;
        }
    }
   
    new_job->next_job = jobs_list;
    jobs_list = new_job;
   
    char buffer[MAX_STR_LEN];
    strcpy(buffer, "[");
   
    char id_str[16];
    sprintf(id_str, "%d", new_job->job_id);
    strcat(buffer, id_str);
   
    strcat(buffer, "] ");
   
    char pid_str[16];
    sprintf(pid_str, "%d", pid);
    strcat(buffer, pid_str);
   
    strcat(buffer, "\n");
    display_message(buffer);
}


void process_completed_jobs() {
    BackgroundJob *current = jobs_list;
    BackgroundJob *previous = NULL;
    int status;
   
    while (current != NULL) {
        pid_t result = waitpid(current->process_id, &status, WNOHANG);
       
        if (result > 0) {
            char buffer[MAX_STR_LEN];
            strcpy(buffer, "[");
           
            char id_str[16];
            sprintf(id_str, "%d", current->job_id);
            strcat(buffer, id_str);
           
            strcat(buffer, "]+  Done ");
            strcat(buffer, current->command_text);
            strcat(buffer, "\n");            
            display_message(buffer);
           
            BackgroundJob *to_free = current;
           
            if (previous == NULL) {
                jobs_list = current->next_job;
                current = jobs_list;
            } else {
                previous->next_job = current->next_job;
                current = current->next_job;
            }

            ProcessList *process = to_free->processes;
            while (process != NULL) {
                ProcessList *next_process = process->next;
                free(process->cmd_name);
                free(process);
                process = next_process;
            }
           
            free(to_free->command_text);
            free(to_free);
           
            if (jobs_list == NULL) {
                current_job_id = 1;
            }
        } else {
            previous = current;
            current = current->next_job;
        }
    }
}


void display_background_processes() {
    BackgroundJob *current = jobs_list;
   
    while (current != NULL) {
        ProcessList *process = current->processes;
        
        if (process == NULL) {
            char process_info[MAX_STR_LEN];
            
            char cmd_copy[MAX_STR_LEN];
            strncpy(cmd_copy, current->command_text, MAX_STR_LEN - 1);
            cmd_copy[MAX_STR_LEN - 1] = '\0';
            
            char *cmd_name = strtok(cmd_copy, " \t");
            if (cmd_name != NULL) {
                strcpy(process_info, cmd_name);
            } else {
                strcpy(process_info, "unknown");
            }
            strcat(process_info, " ");
            char pid_str[16];
            sprintf(pid_str, "%d", current->process_id);
            strcat(process_info, pid_str);
            strcat(process_info, "\n");
            
            display_message(process_info);
        } else {
            while (process != NULL) {
                char process_info[MAX_STR_LEN];
                strcpy(process_info, process->cmd_name);
                strcat(process_info, " ");
                
                char pid_str[16];
                sprintf(pid_str, "%d", process->pid);
                strcat(process_info, pid_str);
                strcat(process_info, "\n");
                
                display_message(process_info);
                process = process->next;
            }
        }
        
        current = current->next_job;
    }
}

void cleanup_background_jobs() {//free all of the jobs when exittitng
    BackgroundJob *current = jobs_list;
   
    while (current != NULL) {
        BackgroundJob *next = current->next_job;
        ProcessList *process = current->processes;
        while (process != NULL) {
            ProcessList *next_process = process->next;
            free(process->cmd_name);
            free(process);
            process = next_process;
        }
        
        free(current->command_text);
        free(current);
        current = next;
    }
    jobs_list = NULL;
    current_job_id = 1;
}


void child_signal_handler(__attribute__((unused)) int sig) {//when a child process changes
    process_completed_jobs();
}


void signal_handler(__attribute__((unused)) int code) {
    display_message("\n");//for ctrl+c
    display_message(prompt);
}


int run_command_pipeline(char **all_tokens, size_t token_total, int background_mode, char *command_string) {
    int pipe_locations[MAX_STR_LEN];
    int total_pipes = 0;
   
    for (size_t i = 0; i < token_total; i++) {
        if (strcmp(all_tokens[i], "|") == 0) {
            pipe_locations[total_pipes++] = i;//see where every pipe is
        }
    }
   
    int command_segments = total_pipes + 1;
    char **command_parts[command_segments];//list of all command pointer
    int segment_start = 0;
    for (int i = 0; i < total_pipes; i++) {//split the cmds
        command_parts[i] = &all_tokens[segment_start];
        all_tokens[pipe_locations[i]] = NULL;
        segment_start = pipe_locations[i] + 1;
    }


    command_parts[total_pipes] = &all_tokens[segment_start];  
    int pipe_fds[total_pipes][2];
   
    for (int i = 0; i < total_pipes; i++) {//make all the pipes
        if (pipe(pipe_fds[i]) == -1) {
            display_error("ERROR: Failed to create pipe", "");            
            for (int j = 0; j < i; j++) {//close all the previous pipes if we get an error
                if (close(pipe_fds[j][0]) == -1) {
                    display_error("ERROR: Failed to close pipe read end", "");
                }
                if (close(pipe_fds[j][1]) == -1) {
                    display_error("ERROR: Failed to close pipe write end", "");
                }
            }
            return -1;
        }
    }
   
    pid_t child_pids[command_segments];
    for (int i = 0; i < command_segments; i++) {//mkae all the forks
        child_pids[i] = fork();
       
        if (child_pids[i] == -1) {//close all the forks if there is a error
            display_error("ERROR: Failed to fork", "");
            for (int j = 0; j < total_pipes; j++) {
                if (close(pipe_fds[j][0]) == -1) {
                    display_error("ERROR: Failed to close pipe read end", "");
                }
                if (close(pipe_fds[j][1]) == -1) {
                    display_error("ERROR: Failed to close pipe write end", "");
                }
            }            
            for (int j = 0; j < i; j++) {
                if (waitpid(child_pids[j], NULL, 0) == -1) {
                    display_error("ERROR: Failed to wait for command", "");
                }
            }
            return -1;
        }
       
        if (child_pids[i] == 0) {//child
            if (i > 0) {
                if (dup2(pipe_fds[i-1][0], STDIN_FILENO) == -1) {//this is new stdin if its not the first
                    display_error("ERROR: Failed to redirect stdin", "");
                    exit(1);
                }
            }
           
            if (i < command_segments - 1) {
                if (dup2(pipe_fds[i][1], STDOUT_FILENO) == -1) {//this is new stdout if not the last
                    display_error("ERROR: Failed to redirect stdout", "");
                    exit(1);
                }
            }
           
            for (int j = 0; j < total_pipes; j++) {//now we did dup2 close all the open fd's
                if (close(pipe_fds[j][0]) == -1) {
                    display_error("ERROR: Failed to close pipe read end", "");
                    exit(1);
                }
                if (close(pipe_fds[j][1]) == -1) {
                    display_error("ERROR: Failed to close pipe write end", "");
                    exit(1);
                }
            }
           
            bn_ptr builtin_func = check_builtin(command_parts[i][0]);
            if (builtin_func != NULL) {
                int result = builtin_func(command_parts[i]);
                exit(result);
            } else {
                execvp(command_parts[i][0], command_parts[i]);
                display_error("ERROR: Unknown command: ", command_parts[i][0]);
                exit(1);
            }
        }
    }
   
    for (int i = 0; i < total_pipes; i++) {//closing all parent pipe fd's
        if (close(pipe_fds[i][0]) == -1) {
            display_error("ERROR: Failed to close pipe read end", "");
        }
        if (close(pipe_fds[i][1]) == -1) {
            display_error("ERROR: Failed to close pipe write end", "");
        }
    }
   
    if (!background_mode) {//wait unless background mode
        for (int i = 0; i < command_segments; i++) {
            int status;
            if (waitpid(child_pids[i], &status, 0) == -1) {
                display_error("ERROR: Failed to wait for command", "");
            } else if (i == 0 && WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                display_error("ERROR: Builtin failed: ", command_parts[i][0]);
            }
        }
    } else {
        register_background_job(child_pids[0], command_string);
    
        for (int i = 1; i < command_segments; i++) {
            char *cmd_name = command_parts[i][0];          
            if (jobs_list != NULL) {
                add_process_to_job(jobs_list->job_id, child_pids[i], cmd_name);
            }
        }
    }
   
    return 0;
}




// You can remove __attribute__((unused)) once argc and argv are used.
int main(__attribute__((unused)) int argc,
         __attribute__((unused)) char* argv[]) {


    char input_buf[MAX_STR_LEN + 1];
    input_buf[MAX_STR_LEN] = '\0';
    char *token_arr[MAX_STR_LEN] = {NULL};


    struct sigaction singaler;//for ctrl+c
    memset(&singaler, 0, sizeof(singaler));
    singaler.sa_handler = signal_handler;
    sigaction(SIGINT, &singaler, NULL);


    struct sigaction child_action;//for when a chold process
    memset(&child_action, 0, sizeof(child_action));
    child_action.sa_handler = child_signal_handler;
    child_action.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &child_action, NULL);


    while (1) {
        process_completed_jobs();
        // Prompt and input tokenization
        check_server_activity();


        // TODO Step 2: DONE
        // Display the prompt via the display_message function.
        display_message(prompt);


       


        int ret = get_input(input_buf);


        //if ctr + D is pressed --> end of file shi
        if (ret == 0) {
            display_message("\n");
            cleanup_background_jobs();
            free_all_vars();
            break;
        }
       
        char original_input[MAX_STR_LEN + 1];
        strncpy(original_input, input_buf, MAX_STR_LEN);
        original_input[MAX_STR_LEN] = '\0';


        size_t token_count = tokenize_input(input_buf, token_arr);


        if (token_count == 0) {// if nothing was entered
            continue;
        }


        int run_in_background = 0;
        if (token_count > 0 && strcmp(token_arr[token_count - 1], "&") == 0) {
            run_in_background = 1;
            token_arr[token_count - 1] = NULL;
            token_count -= 1;
           
            if (token_count == 0) {
                continue;
            }
        }


        int has_pipe = 0;
        for (size_t i = 0; i < token_count; i++) {
            if (strcmp(token_arr[i], "|") == 0) {
                has_pipe = 1;
                break;
            }
        }
       
        if (has_pipe) {
            char cmd_copy[MAX_STR_LEN];
            if (run_in_background) {
                strncpy(cmd_copy, original_input, MAX_STR_LEN - 1);
                cmd_copy[MAX_STR_LEN - 1] = '\0';
               
                char *symbol = strrchr(cmd_copy, '&');
                if (symbol != NULL) {
                    *symbol = '\0';
                }
            }
            char *cmd_str = NULL;
            if (run_in_background) {
                cmd_str = cmd_copy;
            }
            run_command_pipeline(token_arr, token_count, run_in_background, cmd_str);
            continue;
        }
       
        // Clean exit
        // TODO: The next line has a subtle issue. DONE
        if (ret != -1 && (strncmp("exit", token_arr[0], 4) == 0 && (token_arr[0][4] == ' ' || token_arr[0][4] == '\0'))) {
            cleanup_background_jobs();
            free_all_vars();
            break;
        }
        //if 0 or more input bytes are read and if(just enter is pressed OR (exit was typed by itself or with a space and anything else) then break


        if (token_count == 1 && strchr(token_arr[0], '=') != NULL) {
            if (var_assignment(token_arr[0]) == 1) {
                continue;
            } else {
                continue;//basically the first word has equal put its not valid
            }
        }

        bn_ptr builtin_fn = check_builtin(token_arr[0]); //check if builtin exsists
        if (builtin_fn != NULL) {
            if (run_in_background) {//run cmd in background
                pid_t pid = fork();
                if (pid == -1) {
                    display_error("ERROR: Failed to fork", "");
                } else if (pid == 0) {//child
                    ssize_t err = builtin_fn(token_arr); //run builtin
                    if (err == -1) {
                        exit(1);
                    }
                    exit(0);
                } else { //Parent 
                    char command_copy[MAX_STR_LEN];
                    strncpy(command_copy, original_input, MAX_STR_LEN - 1);
                    command_copy[MAX_STR_LEN - 1] = '\0';
                    
                    char *symbol = strrchr(command_copy, '&');
                    if (symbol != NULL) {
                        *symbol = '\0';
                    }
                    
                    register_background_job(pid, command_copy);
                }
            } else {
                ssize_t err = builtin_fn(token_arr); //run builtin
                if (err == -1) {
                    display_error("ERROR: Builtin failed: ", token_arr[0]);
                }
            }
        } else {
            pid_t pid = fork();
            if (pid == -1) {
                display_error("ERROR: Failed to fork", "");
            } else if (pid == 0) {//child
                execvp(token_arr[0], token_arr);
                display_error("ERROR: Unknown command: ", token_arr[0]);//exec failed
                exit(1);
            } else if (pid > 0){
                if (run_in_background) {
                    char command_copy[MAX_STR_LEN];
                    strncpy(command_copy, original_input, MAX_STR_LEN - 1);
                    command_copy[MAX_STR_LEN - 1] = '\0';
                   
                    char *symbol = strrchr(command_copy, '&');
                    if (symbol != NULL) {
                        *symbol = '\0';
                    }
                   
                    register_background_job(pid, command_copy);
                } else {
                    int status;
                    if (waitpid(pid, &status, 0) == -1) {
                        display_error("ERROR: Failed to wait for command", "");
                    }
                }
            }
        }        
    }


    return 0;
}


