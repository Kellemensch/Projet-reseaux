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

/*Cette version est simplifié par rapport à celle qui est demandé dans l'énoncé
 * mais elle vous permettra de comprendre le comportment de la primitive select
 * notamment*/

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
    char encoded[17];
    // int nbuf;
    // int nr;
    // fd_set readfds;
    // int nfds;

    /* prototype*/
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
    /*printf("%x %s\n",ntohl(host),inet_ntoa(addr.sin_addr));*/

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

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = sd;
    fds[1].events = POLLIN;


    // Mot par mot stop and wait et prevenir l'utilisateur de ce qui se passe 
    while (1)
    {
        char *new_buf = NULL;
        // Lire l'entrée utilisateur
        if ((new_buf = fgets(buf, NBUF, stdin)) != NULL)
        {
            // Envoyer chaque caractère séparément en utilisant le stop-and-wait
            for (int i = 0; i < strlen(new_buf); i++)
            {
                char c = new_buf[i];
                uint16_t e = encode_G(c >> 8); // Ajouter le padding pour obtenir 16 bits
                memcpy(encoded, (void*)&e, sizeof(uint16_t));
                int attempts = 0;
                int resend = 1;
                encoded[16] = '\n'; // Pour bonne réception
                while (resend)
                {
                    printf("debut send\n");
                    // Envoyer le caractère encodé
                    if (send(sd, encoded, sizeof(encoded), 0) < 0)
                    {
                        perror("send");
                        exit(1);
                    }
                    printf("Envoi du message encodé --- %s -- ", encoded);

                    // Attendre l'ACK
                    if (poll(fds, 1, -1) < 0)
                    {
                        perror("poll");
                        exit(1);
                    }
                    if (fds[0].revents & POLLIN)
                    {
                        char ack_buf[4];
                        int n = recv(sd, ack_buf, sizeof(ack_buf), 0);
                        if (n > 0)
                        {
                            ack_buf[n] = '\0';
                            if (strcmp(ack_buf, "ACK") == 0)
                            {
                                printf("ACK reçu pour le caractère '%c'\n", c);
                                resend = 0;
                                break; // ACK reçu, passer au caractère suivant
                            }
                            else if (strcmp(ack_buf, "NACK") == 0)
                            {
                                printf("NACK reçu pour le caractère '%c', tentative #%d\n", c, attempts + 1);
                                attempts++;
                                if (attempts >= NATTEMPTS)
                                {
                                    fprintf(stderr, "Trop de tentatives, abandon.\n");
                                    exit(1);
                                }
                                // Attendre un peu avant de réessayer
                                sleep(1);
                            }
                        }
                        else if (n == 0)
                        {
                            printf("Connexion fermée par le serveur\n");
                            exit(1);
                        }
                        else
                        {
                            perror("recv");
                            exit(1);
                        }
                    }
                }
            }
        }
    }

    close(sd);
    return 0;
}
