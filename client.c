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
#include <string.h>
#include <sys/select.h>
#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <poll.h>

#include "correcteur.h"

#define NBUF 1000
#define NATTEMPTS 10


int main(int nargs, char **args)
{
    struct hostent *he;
    unsigned short port;
    char *hostname;
    int sd, host;
    struct sockaddr_in addr;

    char buf[NBUF];
    
    if (nargs != 3)
    {
        fprintf(stderr, "%s machine port\n", args[0]);
        exit(1);
    }

    hostname = args[1];
    port = atoi(args[2]);

    he = gethostbyname(hostname);
    if (he == 0)
    {
        herror("gethostbyname");
        exit(1);
    }
    if (he->h_length != sizeof(host))
    {
        fprintf(stderr, "la longueur des adresses AF_INET a changé ?\n");
        exit(1);
    }
    memcpy(&host, he->h_addr_list[0], sizeof(host));

    /* attention host est en format reseau */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = host;

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0)
    {
        perror("socket");
        exit(1);
    }

    if (connect(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        exit(1);
    }

    printf("Connecté\n");

    struct pollfd fds[2];

    fds[0].fd = sd;
    fds[0].events = POLLIN | POLLOUT;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    char input;
    int str_len;
    while (1) {
        int poll_result = poll(fds, 2, 3); // Timeout à 3 pour essayer mais aucun changement

        if (poll_result == -1) {
            perror("poll() error");
            exit(1);
        }

        if (fds[1].revents & POLLIN) { // Entrée du terminal
            if (read(STDIN_FILENO, &input, 1) > 0) {
                // Attente pour envoyer
                while (1) {
                    poll_result = poll(fds, 1, 3);
                    if (poll_result == -1) {
                        perror("poll() error");
                        exit(1);
                    }
                    if (fds[0].revents & POLLOUT) { // Envoi
                        uint16_t encoded = encode_G(input);
                        printf("encoded : %d\n", encoded);
                        if (send(sd, &encoded, 1, 0) == -1) {
                            perror("send() error");
                            exit(1);
                        }
                        printf("Envoyé: %c\n", input);
                        break;
                    }
                }

                // Attente pour envoyer ACK
                while (1) {
                    poll_result = poll(fds, 1, 3);
                    if (poll_result == -1) {
                        perror("poll() error");
                        exit(1);
                    }
                    if (fds[0].revents & POLLIN) { // ACK ou NACK dispo à lire
                        str_len = recv(sd, buf, NBUF, 0);
                        if (str_len == -1) {
                            perror("recv() error");
                            exit(1);
                        }
                        buf[str_len] = 0;
                        if (strcmp(buf, "ACK") == 0) {
                            printf("Reçu ACK pour: %c\n", input);
                            break;
                        }
                        else if (strcmp(buf, "NACK") == 0) {
                            printf("Reçu NACK pour: %c\n", input);
                            if (send(sd, &input, sizeof(input), 0) == -1) {
                                perror("send");
                                exit(1);
                            }
                            printf("Réenvoi\n");
                        }
                    }
                }
            }
        }
    }

    close(sd);
    return 0;
}
