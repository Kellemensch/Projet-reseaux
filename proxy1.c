#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include "correcteur.h"

#define SERVER_PORT 12345
#define CLIENT_PORT 54321
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        exit(EXIT_FAILURE);
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char ** argv) {
    
    if (argc < 3){
        printf("Usage : ./proxy <serveur_port> <client_port>\n");
        return -1;
    }
    int server_fd, client_fd, new_client_fd;
    struct sockaddr_in server_addr, client_addr, new_client_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 0;
    char buffer[BUFFER_SIZE];
    int i, j, n;

    // Création de la socket du serveur
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("server socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));


    if (connect(server_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect serv");
        exit(1);
    }

    printf("Connecté au serveur\n");

    // Création de la socket du client
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("client socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(atoi(argv[2]));

    if (bind(client_fd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("client bind");
        close(server_fd);
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(client_fd, 10) < 0) {
        perror("client listen");
        close(server_fd);
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    set_nonblocking(server_fd);
    set_nonblocking(client_fd);

    // Initialisation de pollfd 
    memset(fds, 0, sizeof(fds));
    fds[0].fd = client_fd;
    fds[0].events = POLLIN;
    nfds = 1;

    while (1) {
        int poll_count = poll(fds, nfds, -1);
        if (poll_count < 0) {
            perror("poll");
            break;
        }

        // Vérifie si un nouveau client cherche à se connecter
        if (fds[0].revents & POLLIN) {
            while ((new_client_fd = accept(client_fd, (struct sockaddr *)&new_client_addr, &addr_len)) > 0) {
                set_nonblocking(new_client_fd);
                if (nfds < MAX_CLIENTS + 1) {
                    fds[nfds].fd = new_client_fd;
                    fds[nfds].events = POLLIN;
                    nfds++;
                } else {
                    close(new_client_fd);
                    fprintf(stderr, "Nombre maximum de clients atteint\n");
                }
            }
            if (new_client_fd == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("accept");
            }
        }

        // Vérifie si les clients nous ont envoyé un message
        for (i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                memset(buffer, 0, BUFFER_SIZE);
                n = recv(fds[i].fd, buffer, BUFFER_SIZE, 0);
                if (n <= 0) {
                    if (n == 0) {
                        printf("Client déconnecté\n");
                    } else {
                        perror("Réception d'un client");
                    }
                    close(fds[i].fd);
                    fds[i].fd = -1;
                } else {
                    // Implémente une erreur dans le message reçu
                    // int bit = rand() % n;
                    // create_error((uint8_t)&buffer[bit]);

                    // Envoie le message au serveur
                    if (send(server_fd, buffer, n, 0) < 0) {
                        perror("Envoi du message au serveur");
                    }
                }
            }

            // Vérifie si le serveur a envoyé un message
            if (fds[i].revents & POLLOUT) {
                memset(buffer, 0, BUFFER_SIZE);
                n = recv(server_fd, buffer, BUFFER_SIZE, 0);
                if (n <= 0) {
                    if (n == 0) {
                        printf("Server déconnecté\n");
                    } else {
                        perror("Réception du serveur");
                    }
                    close(fds[i].fd);
                    fds[i].fd = -1;
                } else {
                    // Envoie le message au client
                    if (send(fds[i].fd, buffer, n, 0) < 0) {
                        perror("send to client");
                    }
                }
            }
        }

        // Vérifie si un client s'est déconnecté
        for (i = 1; i < nfds; i++) {
            if (fds[i].fd == -1) {
                for (j = i; j < nfds - 1; j++) {
                    fds[j] = fds[j + 1];
                }
                nfds--;
                i--;
            }
        }
    }

    close(server_fd);
    close(client_fd);
    return 0;
}
