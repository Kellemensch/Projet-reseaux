all: serveur proxy client

serveur: serveur.o correcteur.o
	gcc -Wall -Werror serveur.o correcteur.o -o serveur

proxy: proxy.o correcteur.o
	gcc -Wall -Werror proxy.o correcteur.o -o proxy

client: client.o correcteur.o
	gcc -Wall -Werror client.o correcteur.o -o client

serveur.o: serveur.c correcteur.h
	gcc -Wall -Werror -c serveur.c -o serveur.o

proxy.o: proxy.c correcteur.h
	gcc -Wall -Werror -c proxy.c -o proxy.o

client.o: client.c correcteur.h
	gcc -Wall -Werror -c client.c -o client.o

correcteur.o: correcteur.c correcteur.h
	gcc -Wall -Werror -c correcteur.c -o correcteur.o

clean:
	rm -f client proxy serveur client.o proxy.o serveur.o correcteur.o
