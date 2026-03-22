#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>

#include "config.h"
#include "readwrite.h"
#include "msg.h"

void *receive_handler(void *arg) {
    int socket_fd = *(int *)arg;
    char msg[MAX_MSG_LEN + 128];

    while (1) {
        ssize_t bytes = recvMessage(socket_fd, msg, sizeof(msg) - 1);
        if (bytes > 0) {
            printf("\r\33[2K%s\n> ", msg);
            fflush(stdout);
        } else {
            printf("\nConnection lost\n");
            exit(0);
        }
        memset(msg, 0, sizeof(msg));
    }
    return NULL;
}

int main() {
    int sock_fd;
    struct sockaddr_in serv_addr;
    char buf[MAX_MSG_LEN + 1];

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    pthread_t tid;
    pthread_create(&tid, NULL, receive_handler, &sock_fd);
    pthread_detach(tid);

    printf("Chat started. Type messages below:\n> ");
    while (fgets(buf, MAX_MSG_LEN, stdin)) {
        buf[strcspn(buf, "\n")] = 0;
        if (strlen(buf) > 0) {
            sendMessage(sock_fd, buf);
        }
        printf("> ");
        fflush(stdout);
    }

    close(sock_fd);
    return 0;
}