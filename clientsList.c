#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "clientsList.h"
#include "msg.h"
#include "config.h"

void initClientsList(struct clientsList *cl) {
    cl->clients_num = 0;
    pthread_mutex_init(&cl->clients_lock, NULL);
}

int addClient(struct clientsList *cl, int fd) {
    pthread_mutex_lock(&cl->clients_lock);
    if (cl->clients_num == MAX_CONNECTED_CLIENTS) {
        pthread_mutex_unlock(&cl->clients_lock);
        return -1;
    }
    cl->clients_fd[cl->clients_num++] = fd;
    pthread_mutex_unlock(&cl->clients_lock);
    return 0;
}

int removeClient(struct clientsList *cl, int fd) {
    int removed = 0;
    pthread_mutex_lock(&cl->clients_lock);
    for (int i = 0; i < cl->clients_num; i++) {
        if (cl->clients_fd[i] == fd) {
            for (int j = i + 1; j < cl->clients_num; j++) {
                cl->clients_fd[j - 1] = cl->clients_fd[j];
            }
            cl->clients_num--;
            removed = 1;
            break;
        }
    }
    pthread_mutex_unlock(&cl->clients_lock);
    return removed ? 0 : -1;
}

int broadcastClientsList(struct clientsList *cl, const char *msg, const char *sender, int sender_fd) {
    char full_msg[MAX_MSG_LEN + 128];
    
    pthread_mutex_lock(&cl->clients_lock);
    snprintf(full_msg, sizeof(full_msg), "\033[35m%s\033[0m> %s", sender, msg);

    for (int i = 0; i < cl->clients_num; i++) {
        if (cl->clients_fd[i] != sender_fd) {
            sendMessage(cl->clients_fd[i], full_msg);
        }
    }
    pthread_mutex_unlock(&cl->clients_lock);
    return 0;
}

void destroyClientsList(struct clientsList *cl) {
    pthread_mutex_destroy(&cl->clients_lock);
}