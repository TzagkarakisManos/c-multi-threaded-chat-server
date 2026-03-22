#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "readwrite.h"
#include "msg.h"
#include "clientsList.h"
#include "printMsg.h"

void *receive_thread(void *arg){
	if (arg == NULL) {
        perror("Socket initialization problem");
		return NULL;
    }
	int socket_fd = *(int *)arg;
	char msg[MAX_MSG_LEN + 1];
	char print_msg[MAX_PRINT_MSG_LEN + 1];

	while(1){
		ssize_t read_bytes = recvMessage(socket_fd, msg, MAX_MSG_LEN);
		if (read_bytes > 0){
				snprintf(print_msg, sizeof(print_msg), "\033[35mServer\033[0m> %s\n> ", msg);
				printMsg(stdout, print_msg);
			} else if (read_bytes == 0) {
				printf("Server disconnected.\n");
                break;
			} else {
				perror("recvMessage failure");
				break;
			}
			strcpy(msg, "");
	}

	shutdown(socket_fd, SHUT_RDWR);
	return NULL;	
}

void *send_thread(void *arg) {
    int fd = *(int *)arg;
    char msg[MAX_MSG_LEN + 1];

    while(1) {
        printf("> ");
        fflush(stdout);

        if(fgets(msg, MAX_MSG_LEN, stdin) != NULL) {
            msg[strcspn(msg, "\n")] = 0;
            if (sendMessage(fd, msg) == -1) {
                perror("sendMessage failure");
                break;
            }
        } else {	
            break; 
        }
    }
    shutdown(fd, SHUT_RDWR);
    return NULL;
}

int main()
{
	struct sockaddr_in addr;
	int socket_fd;

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1) {
		perror("socket failure");
		exit(EXIT_FAILURE);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);

	if (inet_pton(AF_INET, SERVER_IP_ADDRESS, &addr.sin_addr) <= 0) {
	    close(socket_fd);
	    perror("inet_pton failure");
	    exit(EXIT_FAILURE);
	}

	if (connect(socket_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		close(socket_fd);
	    perror("connect failure");
	    exit(EXIT_FAILURE);
	}

	int *thread_args = malloc(sizeof(int));
    *thread_args = socket_fd;

	pthread_t receive_thread_id;
	if (pthread_create(&receive_thread_id, NULL, receive_thread, thread_args) != 0){
		perror("Failed to initialize threads");
	}

	send_thread(thread_args);
	shutdown(socket_fd, SHUT_RDWR);
    pthread_join(receive_thread_id, NULL);
	close(socket_fd);

	return 0;
}