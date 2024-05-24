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
    char *surnom;

    char buf[NBUF];
    int nbuf, nr;
    fd_set readfds;
    int nfds;

    /* prototype*/
    if (nargs != 4)
    {
        fprintf(stderr, "%s machine port surnom\n", args[0]);
        exit(1);
    }

    hostname = args[1];
    port = atoi(args[2]);
    surnom = args[3];

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

    write(sd, surnom, strlen(surnom));
    write(sd, "\n", 1); // on envoie le surnom et un retour à la ligne

    /*EXEMPLE SELECT FACILE*/
    FD_ZERO(&readfds);

    /*On écoute à la fois l'entrée standard et les messages distants, on pourrait
     * aussi utiliser fork ou ioctl par exemple surtout quand on écoute seulement
     * deux descripteurs simultanément*/
    FD_SET(STDIN_FILENO, &readfds);
    nfds = STDIN_FILENO + 1;
    FD_SET(sd, &readfds);
    if (sd >= nfds)
        nfds = sd + 1; // toujours penser au +1 !!!

    while (1)
    {
        fd_set readfds2 = readfds; /*on utilise un nouvel ensemble fd_set car select modifie
                        l'ensemble passé en paramètre !, en effet l'ensemble
                        readfds2 est restreint aux descripteurs surlesquels il s'est
                        passé un évenement */
        nr = select(nfds, &readfds2, 0, 0, 0);
        if (nr < 0)
        {
            perror("select");
            exit(1);
        }
        if (FD_ISSET(sd, &readfds2))
        {
            nbuf = read(sd, buf, NBUF);
            write(STDOUT_FILENO, buf, nbuf);
        }
        if (FD_ISSET(STDIN_FILENO, &readfds2))
        {
            nbuf = read(STDIN_FILENO, buf, NBUF);
            write(sd, buf, sizeof(buf));
            /*
            // Encodage de 8bits par 8 (un char)
            for (int i = 0; i < nbuf; i++) {
                uint16_t encoded = encode_G(buf[i] << 8); // On ajoute le padding de 8 bits
                int attempts = 0; // Compte le nombre de renvois
                int success = 0;
                
                while (attempts < NATTEMPTS && !success) {
                    write(sd, &encoded, sizeof(encoded));

                    // Lire la réponse du serveur
                    char ack_buf[4];
                    int ack_len = read(sd, ack_buf, sizeof(ack_buf) - 1);
                    if (ack_len > 0) {
                        ack_buf[ack_len] = '\0';
                        if (strcmp(ack_buf, "ACK") == 0) {
                            success = 1;
                            printf("Paquet %d reçu par serveur\n", i);
                        } else if (strcmp(ack_buf, "NACK") == 0) {
                            printf("Paquet %d non reçu par serveur, renvoi...\n", i);
                        }
                    }
                    attempts++;
                }

                // Si après 3 tentatives, toujours pas d'ACK, on abandonne
                if (!success) {
                    printf("Paquet %d n'est pas arrivé à destination après %d tentatives\n", i, NATTEMPTS);
                    break;
                }
            }*/

            
        }
    }

    close(sd);
    return 0;
}
