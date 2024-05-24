#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <poll.h>
#include <arpa/inet.h>

#include "correcteur.h"

#define BUFSIZE sizeof(uint16_t)
#define MAXCONN 1
#define MAXNOM 100
#define ACK "ACK"
#define NACK "NACK"


// Variables globales
char buf[BUFSIZE];
struct pollfd fds[MAXCONN + 1]; // Une pour chaque connexion plus une pour le socket d'écoute


int main(int argc, char** argv) {
    int sockfd, new_fd;
    char buf[17];

    socklen_t sin_size = sizeof(struct sockaddr_in); 

    struct sockaddr_in my_addr;
    struct sockaddr_in client;

    if(argc != 2)
    {
        printf("Usage: %s port_local\n", argv[0]);
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(atoi(argv[1]));
    my_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, 1) < 0) {
        perror("listen");
        exit(1);
    }

    if ((new_fd = accept(sockfd, (struct sockaddr*)&client, &sin_size)) < 0) {
        perror("accept");
        exit(1);
    }

    printf("Connecté au proxy\n");


    while (1) {
        // Réception du proxy
        int n = recv(new_fd, &buf, BUFSIZE, 0);
        if (n > 0) {
            printf("Reçu du proxy : %s -- ", buf);

            uint16_t encoded = (uint16_t)*buf;
            uint8_t verif = crcVerif(encoded);

            if (verif == 0) {
                // Envoi de ACK
                if (send(new_fd, ACK, sizeof(ACK), 0) < 0) {
                    perror("send");
                    exit(1);
                }
                printf("Envoi ACK\n");
            } else {
                int error_index = crc_error_amount(encoded);

                // Plus de 1 erreur on peut pas corriger
                if (error_index == -1) {
                    printf("Plus de 1 erreur on ne peut corriger -- envoi NACK\n");
                    if (send(new_fd, NACK, sizeof(NACK), 0) < 0) {
                        perror("send");
                        exit(1);
                    }
                    printf("Envoi NACK\n");
                } else {
                    // Seulement 1 erreur que l'on corrige
                    encoded = chg_nth_bit(error_index+8, encoded);
                    printf("Corrigé : %c\n", (uint8_t) encoded << 8);
                }
            }
        }
        else if (n == 0) {
            printf("Connexion fermée par le proxy\n");
            break;
        }
        else {
            perror("recv");
            exit(1);
        }
    }
    

    close(sockfd);
    close(new_fd);

    return 0;
}