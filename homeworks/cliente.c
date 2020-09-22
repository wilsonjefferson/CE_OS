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
	short int fd1, fd2, valore_fifo = 0;
	int tagli[1], prezzo[1];
	double portafoglio, tmp = 0.0;

	sem_t *pizzaiolo, *cliente, *sala_attesa, *mutex, *mutex2;
	
	unlink("dialogo_pizzaiolo");  // unlink della pipe
	valore_fifo = mkfifo("dialogo_pizzaiolo", 0777);
	printf("valore fifo = %d\n", valore_fifo);
	
	fd2 = open("dialogo_pizzaiolo", 0522); // connessione alla pipe
	printf("fd2 = %d\n", fd2);
	
	sem_unlink("semaSA"); sem_unlink("semaM2"); // unlink dei semafori: andrebbero commentati
	
	pizzaiolo = sem_open("semaP", O_CREAT, S_IWUSR|S_IRUSR, 0); // inizializzazione semafori
	cliente = sem_open("semaC", O_CREAT, S_IWUSR|S_IRUSR, 0);
	sala_attesa = sem_open("semaSA", O_CREAT, S_IWUSR|S_IRUSR, 10);
	mutex = sem_open("semaM", O_CREAT, S_IWUSR|S_IRUSR, 1);
	mutex2 = sem_open("semaM2", O_CREAT, S_IWUSR|S_IRUSR, 1);
	
	int val_pizzaiolo[1], val_cliente[1], val_sala[1], val_mutex[1], val_mutex2[1];
	sem_getvalue(pizzaiolo, val_pizzaiolo); // controllo valori assunti dai semafori
	sem_getvalue(cliente, val_cliente);
	sem_getvalue(sala_attesa, val_sala);
	sem_getvalue(mutex, val_mutex);
	sem_getvalue(mutex2, val_mutex2);
	printf("pizzaiolo = %d\n", *val_pizzaiolo);
	printf("cliente = %d\n", *val_cliente);
	printf("sala_attesa = %d\n", *val_sala);
	printf("mutex = %d\n", *val_mutex);
	printf("mutex2 = %d\n", *val_mutex2);
	
	srand((unsigned int)time(0));  // creazione random di portafoglio e tagli di pizza
	portafoglio = (double)(int)((((double)(1 + rand())/RAND_MAX)*50)*100)/100; // valore compreso fra 1 e 50
	//portafoglio = ((double)(1 + rand())/RAND_MAX)*50; // alternativa
	
	tagli[0] = 1 + rand()%30; // valore compreso fra 1 e 30
	printf("portafoglio = %f\n", portafoglio);
	printf("tagli = %d\n", tagli[0]);
	
	printf("attendo di entrare in negozio\n");
	sem_wait(sala_attesa);  // attesa di entrare nella sala d'aspetto
	printf("sono in sala d'attesa\n");
	sem_wait(mutex2);       // attesa di poter essere in prima posizione fra i clienti
	printf("è il mio turno di essere servito!\n");
	sem_post(cliente);     // avviso il pizzaiolo che c'è un cliente
	
	sem_getvalue(mutex, val_mutex);
	printf("mutex = %d\n", *val_mutex);
	
	sem_wait(mutex);    // attesa per informare del numero di tagli
	printf("dentro area critica.\n");
	
	/*eseguito e terminato il 1° cliente, il 2° cliente si ferma in questo punto
	  non proseguendo nell'esecuzione del programma: per qualche motivo non apre
	  in modalità scrittura la fifo "dialogo_cliente", creata dal processo pizzaiolo;
	  in ogni caso non restituisce alcun tipo di errore o va in shutdown, resta fermo.*/
	
	fd1 = open("dialogo_cliente", 0355); // apertura connessione alla pipe
	if(fd1 == 0){ printf("errore in pipe\n"); abort(); }
	printf("fd1 = %d\n", fd1);
	
	printf("comunico al pizzaiolo il numero di tagli.\n"); // invio comunicato
	write(fd1, tagli, sizeof(tagli));
	
	sem_post(mutex); // avviso fine sequenza istruzioni
	sleep(2);
	sem_getvalue(mutex, val_mutex);
	printf("mutex = %d\n", *val_mutex);
	sem_wait(mutex);  // attesa di ricezione permesso
	
	prezzo[0] = 0;
	printf("leggo la risposta del pizzaiolo\n");
	read(fd2, prezzo, sizeof(prezzo)); // lettura risposta del pizzaiolo
	//prezzo[0] = atof(buf0);
	tmp = ((double)prezzo[0])/100.00;  // conversione prezzo da int a double
	printf("prezzo = %2.2f\n", tmp);
	
	if(tmp < 1 + rand()%2){ // decisione se il singolo trancio ha un prezzo accettabile
		printf("prezzo accettabile.\n");
		printf("comunico al pizzaiolo la risposta.\n");
		write(fd1, "1", 2);  // comunico esito positivo
		
		sem_post(mutex); // avviso fine sequenza istruzioni
		sleep(4);
		sem_getvalue(mutex, val_mutex);
		printf("mutex = %d\n", *val_mutex);
		sem_wait(mutex);  // attesa di ricezione permesso
		
		printf("leggo la risposta del pizzaiolo.\n");
		read(fd2, prezzo, sizeof(prezzo));  // lettura risposta del pizzaiolo
		//prezzo[0] = atof(buf0);
		tmp = (double)prezzo[0]/100;  // conversione dell'importo da int a double
		printf("prezzotot = %2.2f\n", tmp);
		
		if(portafoglio < tmp){ // verifica se il cliente è in grado di pagare
		printf("mi costa troppo!\n");
		printf("comunico al pizzaiolo la risposta.\n");
		write(fd1, "0", 2);
		}else{
			printf("ok ho abbastanza soldi!\n");
			printf("comunico al pizzaiolo la risposta.\n");
			write(fd1, "1", 2);	
		}
	}else{
		printf("mi costa troppo!\n");
		printf("comunico al pizzaiolo la risposta.\n");
		write(fd1, "0", 2);
	}
	close(fd1); close(fd2); // chiusura connessioni
	
	sem_wait(pizzaiolo); // fa sedere il pizzaiolo
	sem_post(mutex);     // avviso il pizzaiolo che ho terminato
	sleep(3);
	printf("me ne vado dal negozio\n");
	sem_post(mutex2);  // avviso che la prima posizione tra i clienti è libera
	sem_post(sala_attesa); // lascio il negozio
	return 0;
}