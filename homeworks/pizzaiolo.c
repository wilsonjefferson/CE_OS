#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

int main(){
	int fd1, fd2, tagli[1], importo[1], valore_fifo;
	int prezzo[1];
	char buf0[100];
	
	sem_t *pizzaiolo, *cliente, *mutex;
	
	/*
	commentato codice seguente per verificare se creando una nuova fifo
	per ogni cliente risolvo il problema --> non risolto
	*/
	/*
	unlink("dialogo_cliente"); // creazione pipe 
	valore_fifo = mkfifo("dialogo_cliente", 0777);
	printf("valore fifo = %d\n", valore_fifo);
	*/
	
	sem_unlink("semaP"); // unlink dei semafori
	sem_unlink("semaC");	
	sem_unlink("semaM");
	
	pizzaiolo = sem_open("semaP", O_CREAT, S_IWUSR|S_IRUSR, 0); // inizializzazione semafori
	cliente = sem_open("semaC", O_CREAT, S_IWUSR|S_IRUSR, 0);
	mutex = sem_open("semaM", O_CREAT, S_IWUSR|S_IRUSR, 1);
	
	int val_pizzaiolo[1], val_cliente[1], val_mutex[1]; // controllo inizializzazione semafori
	sem_getvalue(pizzaiolo, val_pizzaiolo);
	sem_getvalue(cliente, val_cliente);
	sem_getvalue(mutex, val_mutex);
	printf("pizzaiolo = %d\n", *val_pizzaiolo);
	printf("cliente = %d\n", *val_cliente);
	printf("mutex = %d\n", *val_mutex);
	
	printf("entro nel loop\n");
	
	while(1){  // operativo
		
		unlink("dialogo_cliente"); // creazione pipe per ogni cliente
		valore_fifo = mkfifo("dialogo_cliente", 0777);
		printf("valore fifo = %d\n", valore_fifo);
		fd1 = open("dialogo_cliente", 0522);
		printf("fd1 = %d\n", fd1);
		
		tagli[0] = importo[0] = 0;
		
		printf("attendo i clienti.\n");  // attesa di un cliente
		sem_wait(cliente);               // abbassa il semaforo del cliente per "occuparlo"
		printf("è arrivato un cliente, mi alzo.\n");
		sem_getvalue(pizzaiolo, val_pizzaiolo);
		printf("pizzaiolo = %d\n", *val_pizzaiolo);
		sem_post(pizzaiolo);             // il pizzaiolo si alza
		sem_getvalue(mutex, val_mutex);
		printf("mutex = %d\n", *val_mutex);
		sleep(1); // ritardo temporale per dare il tempo al cliente di comunicare il numero di tagli
		sem_wait(mutex);   // attende di poter leggere la richiesta
		
		fd2 = open("dialogo_pizzaiolo", O_WRONLY);  // apertura connessione in scrittura della pipe
		printf("fd2 = %d\n", fd2);
		
		printf("leggo la richiesta del cliente.\n");  // lettura richiesta
		read(fd1, tagli, sizeof(tagli));
		
		//tagli[0] = atoi(buf0);
		printf("il cliente vuole %d tagli di pizza.\n", tagli[0]); // elaborazione e calcolo importo
		importo[0] = tagli[0]*150;
		printf("importo = %d\n", importo[0]);
		
		prezzo[0] = 150;
		printf("comunico il prezzo di un trancio di pizza.\n");
		write(fd2, prezzo, sizeof(prezzo));  // comunicazione del prezzo
		
		sem_post(mutex);  // avverte il cliente che ha concluso 
		sleep(4);
		sem_getvalue(mutex, val_mutex);
		printf("mutex = %d\n", *val_mutex);
		sem_wait(mutex);  // attende risposta del cliente
		
		printf("leggo la risposta del cliente.\n");
		read(fd1, buf0, 2);
		if(buf0[0] == '1'){                 // verifica della risposta
			printf("il cliente conferma il prezzo.\n");
			printf("comunico l'importo.\n");
			write(fd2, importo, sizeof(importo)); // comunico importo
			
			sem_post(mutex); // avverte il cliente che ha concluso
			sleep(4);
			sem_getvalue(mutex, val_mutex);
			printf("mutex = %d\n", *val_mutex);
			sem_wait(mutex);  // attende risposta dal cliente
		
			printf("leggo la risposta del cliente.\n");		
			read(fd1, buf0, 2);
			if(buf0[0] == '1'){   // verifica risposta del cliente
			printf("il cliente ha accettato, mi metto a lavorare!\n");
			sleep(3);
			}	
		}else{
			printf("cliente ha rifiutato.\n");
		}			
		printf("mi rimetto ad aspettare il prossimo cliente.\n");
		close(fd1); close(fd2); // chiusura connessioni
		sem_post(mutex);		// avviso di disponibilità
	}
	return 0;
}