#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <netinet/in.h>

#define BUFFER_SIZE 1024

typedef struct ClientNode {
    int sock_fd;
    char client_id[32];
    struct ClientNode *next;
} ClientNode;

typedef struct {
    int server_fd;
    int port;
    int running;
    int client_count;
    ClientNode *client_list;
} ServerInfo;


int start_server(int port);
void close_server();
int send_message(int port, const char *hostname, const char *message);
int start_client(int port, const char *hostname);
int connect_to_server(const char *hostname, int port);
int check_server_activity();
void add_client(int client_fd);
void remove_client(int client_fd);
void broadcast_message(const char *message, int sender_fd);

#endif