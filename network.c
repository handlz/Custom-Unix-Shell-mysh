#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>

#include "network.h"
#include "io_helpers.h"

int server_fd = -1;
int server_port = -1;
int server_running = 0;
int client_count = 0;
ClientNode *client_list = NULL;

int start_server(int port) {
    if (server_running) {
        display_error("ERROR: Server is already running on port ", "");
        return -1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        display_error("ERROR: Failed to create socket", "");
        return -1;
    }

    int on = 1;
    int status = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on));
    if (status < 0) {
        display_error("ERROR: Failed to set socket options", "");
        close(server_fd);
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        display_error("ERROR: Port already in use", "");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 5) < 0) {
        display_error("ERROR: Failed to listen on socket", "");
        close(server_fd);
        return -1;
    }

    server_port = port;
    server_running = 1;
    client_count = 0;
    client_list = NULL;

    return 0;
}




void close_server() {
    if (!server_running) {
        return;
    }
    ClientNode *curr = client_list;
    ClientNode *next;
    
    while (curr != NULL) {
        next = curr->next;
        close(curr->sock_fd);
        free(curr);
        curr = next;
    }
    
    client_list = NULL;

    close(server_fd);
    
    server_fd = -1;
    server_port = -1;
    server_running = 0;
    client_count = 0;
}

int send_message(int port, const char *hostname, const char *message) {
    int sock_fd = connect_to_server(hostname, port);
    if (sock_fd < 0) {
        return -1;
    }

    int len = strlen(message);
    if (write(sock_fd, message, len) != len) {
        display_error("ERROR: Failed to write message to socket", "");
        close(sock_fd);
        return -1;
    }
    
    if (len < 2 || message[len-2] != '\r' || message[len-1] != '\n') {
        if (write(sock_fd, "\r\n", 2) != 2) {
            display_error("ERROR: Failed to write newline to socket", "");
            close(sock_fd);
            return -1;
        }
    }
    
    close(sock_fd);
    
    for (int i = 0; i < 10; i++) {
        check_server_activity();
        usleep(100000);
    }
    
    return 0;
}


int start_client(int port, const char *hostname) {
    int sock_fd = connect_to_server(hostname, port);
    if (sock_fd < 0) {
        return -1;
    }

    fd_set read_fds, master_set;
    FD_ZERO(&master_set);
    FD_SET(STDIN_FILENO, &master_set);
    FD_SET(sock_fd, &master_set);
    int max_fd = sock_fd > STDIN_FILENO ? sock_fd : STDIN_FILENO;

    char buffer[BUFFER_SIZE];
    
    while (1) {
        read_fds = master_set;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            display_error("ERROR: Failed to select", "");
            break;
        }

        if (FD_ISSET(sock_fd, &read_fds)) {
            int bytes_read = read(sock_fd, buffer, BUFFER_SIZE - 1);
            if (bytes_read <= 0) {
                if (bytes_read == 0) {
                    display_message("Server disconnected\n");
                } else {
                    display_error("ERROR: Failed to read from socket", "");
                }
                break;
            }
            
            buffer[bytes_read] = '\0';
            display_message(buffer);
            
            if (bytes_read < 2 || buffer[bytes_read-2] != '\r' || buffer[bytes_read-1] != '\n') {
                display_message("\n");
            }
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            int bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1);
            if (bytes_read <= 0) {
                break;
            }
            
            buffer[bytes_read] = '\0';
            
            if (write(sock_fd, buffer, bytes_read) != bytes_read) {
                display_error("ERROR: Failed to write to socket", "");
                break;
            }
            
            if (bytes_read < 2 || buffer[bytes_read-2] != '\r' || buffer[bytes_read-1] != '\n') {
                if (write(sock_fd, "\r\n", 2) != 2) {
                    display_error("ERROR: Failed to write newline to socket", "");
                    break;
                }
            }
        }
    }

    close(sock_fd);
    return 0;
}

int connect_to_server(const char *hostname, int port) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        display_error("ERROR: Failed to create socket", "");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    struct hostent *he = gethostbyname(hostname);
    if (he == NULL) {
        display_error("ERROR: Failed to resolve hostname", "");
        close(sock_fd);
        return -1;
    }
    
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        display_error("ERROR: Failed to connect to server", "");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

int check_server_activity() {
    if (!server_running) {
        return 0;
    }

    fd_set read_fds;
    struct timeval tv;
    int activity, max_fd;

    FD_ZERO(&read_fds);
    FD_SET(server_fd, &read_fds);
    max_fd = server_fd;
    
    ClientNode *curr = client_list;
    while (curr != NULL) {
        FD_SET(curr->sock_fd, &read_fds);
        if (curr->sock_fd > max_fd) {
            max_fd = curr->sock_fd;
        }
        curr = curr->next;
    }
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    activity = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
    if (activity < 0) {
        display_error("ERROR: Failed to select", "");
        return -1;
    }
    
    if (activity == 0) {
        return 0;
    }

    if (FD_ISSET(server_fd, &read_fds)) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            display_error("ERROR: Failed to accept connection", "");
        } else {
            add_client(client_fd);
        }
    }

    curr = client_list;
    while (curr != NULL) {
        ClientNode *next = curr->next;
        if (FD_ISSET(curr->sock_fd, &read_fds)) {
            char buffer[BUFFER_SIZE];
            int bytes_read = read(curr->sock_fd, buffer, BUFFER_SIZE - 1);
            
            if (bytes_read <= 0) {
                remove_client(curr->sock_fd);
            } else {
                buffer[bytes_read] = '\0';                
                if (strcmp(buffer, "\\connected\r\n") == 0 || strcmp(buffer, "\\connected") == 0) {
                    int count = 0;
                    ClientNode *temp = client_list;
                    while (temp != NULL) {
                        count++;
                        temp = temp->next;
                    }                    
                    char response[20];
                    sprintf(response, "%d\r\n", count);
                    write(curr->sock_fd, response, strlen(response));

                    char display_count[50];
                    sprintf(display_count, "Connected clients: %d\n", count);
                    display_message(display_count);
                } else {
                    char formatted_message[BUFFER_SIZE + 128];
                    char clean_buffer[BUFFER_SIZE];
                    strcpy(clean_buffer, buffer);
                    int buffer_len = strlen(clean_buffer);
                    if (buffer_len >= 2 && clean_buffer[buffer_len-2] == '\r' && clean_buffer[buffer_len-1] == '\n') {
                        clean_buffer[buffer_len-2] = '\0';
                    }
                    snprintf(formatted_message, sizeof(formatted_message), "%s %s\n", curr->client_id, clean_buffer);
                    display_message(formatted_message);
                    char full_message[BUFFER_SIZE + 128];
                    snprintf(full_message, sizeof(full_message), "%s %s", curr->client_id, buffer);
                    broadcast_message(full_message, curr->sock_fd);
                }
            }
        }
        
        curr = next;
    }
    
    return 1;
}

void add_client(int client_fd) {
    ClientNode *new_client = malloc(sizeof(ClientNode));
    if (new_client == NULL) {
        display_error("ERROR: Failed to allocate memory for client", "");
        return;
    }

    new_client->sock_fd = client_fd;
    sprintf(new_client->client_id, "client%d:", ++client_count);
    new_client->next = NULL;

    if (client_list == NULL) {
        client_list = new_client;
    } else {
        new_client->next = client_list;
        client_list = new_client;
    }
}

void remove_client(int client_fd) {
    ClientNode *curr = client_list;
    ClientNode *prev = NULL;

    while (curr != NULL) {
        if (curr->sock_fd == client_fd) {
            close(curr->sock_fd);

            if (prev == NULL) {
                client_list = curr->next;
            } else {
                prev->next = curr->next;
            }

            free(curr);
            return;
        }

        prev = curr;
        curr = curr->next;
    }
}

void broadcast_message(const char *message, int sender_fd) {
    ClientNode *curr = client_list;
    
    while (curr != NULL) {
        if (curr->sock_fd != sender_fd) {
            int len = strlen(message);
            if (write(curr->sock_fd, message, len) != len) {

            }
        }
        curr = curr->next;
    }
}