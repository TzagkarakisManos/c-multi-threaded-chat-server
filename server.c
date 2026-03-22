#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>

#include "config.h"
#include "readwrite.h"
#include "msg.h"
#include "clientsList.h"
#include "printMsg.h"

struct clientsList cl;

void *receive_thread(void *arg) {
    int socket_fd = *(int *)arg;
    free(arg);
    
    char msg[MAX_MSG_LEN + 1];
    char client_id[32]; 
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    if (getpeername(socket_fd, (struct sockaddr *)&addr, &addr_len) == 0) {
        char ip_only[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ip_only, sizeof(ip_only));
        int port = ntohs(addr.sin_port);
        snprintf(client_id, sizeof(client_id), "%s:%d", ip_only, port);
    } else {
        strncpy(client_id, "Unknown", sizeof(client_id));
    }

    while (1) {
        ssize_t read_bytes = recvMessage(socket_fd, msg, MAX_MSG_LEN);

        if (read_bytes > 0) {
            printf("[%s]: %s\n", client_id, msg);
            broadcastClientsList(&cl, msg, client_id, socket_fd);
        } else {
            broadcastClientsList(&cl, "left the conversation", client_id, socket_fd);
            printf("-- User %s disconnected --\n", client_id);
            removeClient(&cl, socket_fd);
            close(socket_fd);
            break; 
        }
        memset(msg, 0, sizeof(msg));
    }
    return NULL;
}

int main() {
    struct sockaddr_in addr;
    int socket_fd, client_fd, socket_option;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char ip_buf[INET_ADDRSTRLEN];

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) { perror("socket"); exit(1); }

    socket_option = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &socket_option, sizeof(socket_option));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind"); exit(1);
    }

    listen(socket_fd, SOMAXCONN);
    initClientsList(&cl);
    printf("Server listening on port %d\n", PORT);

    while (1) {
        client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == -1) continue; 

        inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, sizeof(ip_buf));
        addClient(&cl, client_fd);
        broadcastClientsList(&cl, "joined the room", ip_buf, -1);

        int *arg = malloc(sizeof(int));
        *arg = client_fd;

        pthread_t tid;
        if (pthread_create(&tid, NULL, receive_thread, arg) != 0) {
            removeClient(&cl, client_fd);
            free(arg); 
            close(client_fd);
            continue;
        }
        pthread_detach(tid);
    }
    return 0;
}