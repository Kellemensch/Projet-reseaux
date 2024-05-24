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
#include <time.h>
#include <arpa/inet.h>

#include "correcteur.h"

#define BUFSIZE 17
#define MAXCONN 1
#define MAXNOM 100
#define NERREURS 1

// Variables globales
char buf[BUFSIZE];
int sd, nfds, sd_serv;

void introduce_errors(uint16_t *m) {
    // Introduit i erreurs

    srand(time(NULL));
    for (int j = 0; j < NERREURS; j++) {
        int i = rand() % 8;
        *m = chg_nth_bit(i, *m);
    }
}


int main(int argc, char **argv)
{
    unsigned short port, port_serv;
    struct sockaddr_in addr, addr_serv;
    char buf[17];
    char ack_buf[4];

    if (argc != 4)
    {
        fprintf(stderr, "%s port port_serv ip_serv\n", argv[0]);
        exit(1);
    }

    port = atoi(argv[1]);
    port_serv = atoi(argv[2]);
    char* ip_serv = argv[3];

    // Création de la socket pour communiquer avec le serveur
    int sd_serv;
    if ((sd_serv = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket serv");
        exit(1);
    }

    // Configuration de l'adresse du serveur
    addr_serv.sin_family = AF_INET;
    addr_serv.sin_port = htons(port_serv);
    if (inet_pton(AF_INET, ip_serv, &addr_serv.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }

    // Connexion au serveur
    if (connect(sd_serv, (struct sockaddr*) &addr_serv, sizeof(addr_serv)) < 0) {
        perror("connect serv");
        exit(1);
    }

    printf("Connecté au serveur %s:%d\n", ip_serv, port_serv);

    // Création de la socket du proxy
    int sd;
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket proxy");
        exit(1);
    }

    // Configuration de l'adresse du proxy
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Liaison de la socket du proxy à l'adresse locale
    if (bind(sd, (struct sockaddr *)(&addr), sizeof(addr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    // Attente des connexions entrantes
    if (listen(sd, MAXCONN) < 0)
    {
        perror("listen");
        exit(1);
    }

    // Acceptation de la connexion entrante du client
    int sin_size = sizeof(struct sockaddr_in);
    int new_fd = accept(sd, (struct sockaddr*)&addr, (socklen_t*)&sin_size);

    printf("Connecté au client\n");



    // Boucle de communication
    while (1) {
        // Reception des données du client
        int nbuf = recv(new_fd, &buf, sizeof(uint16_t), 0);
        if (nbuf > 0) {
            printf("Reçu du client : %s  ... bruitage...\n", buf);
            uint16_t encoded = atoi(buf);
            introduce_errors(&encoded);

            // Envoi des données au serveur
            memcpy(buf, &encoded, 16*sizeof(char));
            buf[16] = '\n';
            send(sd_serv, &buf, nbuf, 0);
            printf("Envoi données au serveur : %s\n",buf);
        }

        // Réception de l'ACK du serveur
        int nbuf2 = recv(sd_serv, (void*)&ack_buf, sizeof("ACK"), 0);
        if (nbuf2 > 0) {
            printf("Reçu du serveur : %s\n",ack_buf);
            // Envoi de l'ACK au client
            send(new_fd, ack_buf, nbuf2, 0);
            printf("Envoi ACK au client\n");
        }
    }

    close(sd);
    close(sd_serv);

    return 0;
}