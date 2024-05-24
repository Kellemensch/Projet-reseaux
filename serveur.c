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
#include <poll.h>

#define BUFSIZE 1024
#define MAXCONN 1
#define MAXNOM 100

/*Cette strucure contient les informations sur un client*/
struct connexion
{
    int libre; /*booléen qui nous permet de traiter que les connexions existantes*/
    int sd;    /* descripteur */
    char surnom[MAXNOM];
    char buf[BUFSIZE];
    char *bufcourant;
};

// Variables globales
char buf[BUFSIZE];
fd_set readfds;
int sd, nfds;
struct connexion connexions[MAXCONN];

int lit(struct connexion *conn)
{
    int nbuf;
    int maxbuf;
    /* le -1 sert parce qu'on rajoute un '\0' */
    maxbuf = BUFSIZE - (conn->bufcourant - conn->buf) - 1;
    nbuf = read(conn->sd, conn->bufcourant, maxbuf);
    if (nbuf < 0)
    {
        perror("read");
        exit(1);
    }
    conn->bufcourant += nbuf;
    /* pour que ca soit une chaine terminee par un zero */
    *(conn->bufcourant) = '\0';
    return nbuf;
}

/* extrait le premier mot separe par '\n'
 le met dans la variable globale buf !!*/
int extrait(struct connexion *conn)
{
    char *fin;
    fin = index(conn->buf, '\n');
    if (fin == NULL)
    {
        return 0;
    }
    *fin = '\0';
    strcpy(buf, conn->buf);
    /* les zones memoire se chevauchent !!*/
    /* je mets strlen(fin+1)+1 car il faut copier le '\0' final ! */
    memmove(conn->buf, fin + 1, conn->bufcourant - conn->buf + 1);
    conn->bufcourant -= fin + 1 - conn->buf;
    return 1;
}

/* affiche tout ce qu'on a recupere de cette connexion */
void communique(struct connexion *conn)
{
    while (extrait(conn))
    {
        char buf2[BUFSIZE + MAXNOM + 4];
        int i;
        sprintf(buf2, "<%s> %s\n", conn->surnom, buf);
        write(STDOUT_FILENO, buf2, strlen(buf2));
        for (i = 0; i < MAXCONN; i++)
            if (!connexions[i].libre)
            {
                // write(connexions[i].sd, buf2, strlen(buf2));
            }
        /* c'est ici qu'il faut traiter la chaine buf;;;;
            et peut-etre l'envoyer a tous les clients..
            A VOUS DE VOIR CE QUE VOUS VOULEZ EN FAIRE !!!
        */
    }
}

void nouvelleconnexion()
{
    int sd2;
    // char *buf2;
    struct connexion *conn = 0;
    int i;

    for (i = 0; i < MAXCONN; i++)
    {
        if (connexions[i].libre)
        {
            conn = &connexions[i];
            break;
        }
    }

    if (conn == 0)
    {
        fprintf(stderr, "toutes les connexions sont occupees");
        return;
    }

    conn->libre = 0;
    /* on se sert pas de l'adresse
        de toute facon... */

    sd2 = accept(sd, 0, 0);
    /* ici on bloque le systeme
        il vaudrait mieux aller de nouveau dans
        le accept */
    if (sd2 < 0)
    {
        perror("accept");
        exit(1);
    }

    conn->sd = sd2;
    conn->bufcourant = conn->buf;
    /* je me bloque tant que je n'ai pas le
        nom !!!*/
    do
        lit(conn);
    while (!extrait(conn));
    strcpy(conn->surnom, buf);

    if (sd2 >= nfds)
        nfds = sd2 + 1;

    printf("nouvelle connexion : %s\n", conn->surnom);
}

/*On ferme ici les descripteurs ouverts*/
void finconnexion(struct connexion *conn)
{
    printf("connexion %s terminee\n", conn->surnom);
    FD_CLR(conn->sd, &readfds);
    close(conn->sd);
    conn->libre = 1;
}

int main(int argc, char **argv)
{
    unsigned short port;
    struct sockaddr_in addr/*, addrfrom*/;
    // int addrfromlen;
    // int nbuf;
    int i;
    int nr/*, fd, ret*/;
    // int nnom;
    // char nom[MAXNOM];

    if (argc != 2)
    {
        fprintf(stderr, "%s port\n", argv[0]);
        exit(1);
    }

    port = atoi(argv[1]);
    printf("Port d'écoute : %d\n",port);

    /* attention host est en format reseau */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    for (i = 0; i < MAXCONN; i++)
    {
        connexions[i].libre = 1;
    }

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0)
    {
        perror("socket");
        exit(1);
    }

    if (bind(sd, (struct sockaddr *)(&addr), sizeof(addr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    if (listen(sd, MAXCONN) < 0)
    {
        perror("listen");
        exit(1);
    }

    nouvelleconnexion();
    
    while (1)
    {
        if (lit(&connexions[0]) != 0) {
            communique(&connexions[0]);
        }
        else 
            finconnexion(&connexions[0]);
     
        printf("fin boucle\n");
    }

    close(sd);

    return 0;
}