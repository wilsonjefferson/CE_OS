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
#include <time.h>
#include <errno.h>

extern int errno;

int main(){
	int *shared_memory, conto_corrente, controllo, shared_open;
	sem_t *account, *mutex;
	
	errno = 0;
	
	shm_unlink("comunicazione"); // unlink della memoria condivisa e mappatura
	
	shared_open = shm_open("comunicazione",  O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR); // creazione memoria condivisa
    ftruncate(shared_open, sizeof(int));
    shared_memory = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shared_open, 0);
	
	if(shared_memory == NULL || shared_open == NULL || shared_open == -1){ printf("errore: %d  errore: %s.\n", errno, strerror(errno));}
	
	sem_unlink("Personale"); // unlink dei semafori
	sem_unlink("Banca");
	
	account = sem_open("Personale", O_CREAT, S_IRUSR|S_IWUSR, 0); // inizializzazione semafori
	mutex = sem_open("Banca", O_CREAT, S_IRUSR|S_IWUSR, 1);
	
	int val_account[1], val_mutex[1]; // verifica dei valori dei semafori e shared_memory
	sem_getvalue(account, val_account);
	sem_getvalue(mutex, val_mutex);
	printf("account = %d\n", *val_account);
	printf("mutex = %d\n", *val_mutex);
	printf("shared_open = %d\n", shared_open);
	
	if(shared_memory == MAP_FAILED){ printf("errore in mappatura.\n"); exit(0);}
	printf("shared_memory = %d\n", *shared_memory);
	printf("entro nel loop\n");
	
	conto_corrente = 100; //saldo di base
	while(1){
		controllo = 1;
		printf("attendo un account\n");
		sem_wait(account);          // attesa di un account
		printf("è arrivato un account!\nattendo che mi invii un messaggio\n");
		sleep(0);
		sem_wait(mutex);  // attesa di poter accedere alla shared_memory
		printf("mi è arrivato un messaggio\n");
		if(*shared_memory < 0){            // verifica del valore nella memoria condivisa
			printf("il messaggio è una richiesta di prelievo\nrispondo che è ammesso\n");
			*shared_memory = 1;
			sem_post(mutex);				// avviso che è possibile accedere alla shm
			while(controllo == 1){	
				printf("attendo di sapere quat'è il prelievo\n");
				sleep(2);
				sem_wait(mutex);			 // attendo di poter accedere all'area critica
				printf("prelievo richiesto: %d\n", *shared_memory);
				if(*shared_memory <= 0 || *shared_memory > conto_corrente){
					printf("il prelievo non è ammissibile!\n");
					*shared_memory = -1;   // avviso che l'operazione è fallita
					sem_post(mutex);       // avviso che è possibile accedere alla shm
				}else{
					printf("il prelievo è ammissibile\n");
					printf("valore attuale del conto corrente = %d\n", conto_corrente);
					printf("prelievo: %d\n", *shared_memory);
					conto_corrente = conto_corrente - *shared_memory; // modifica del conto corrente
					printf("valore aggiornato del conto corrente = %d\n", conto_corrente);
					*shared_memory = 1;    // avviso che l'operazione è andata a buon fine
					controllo = 0;         // fermo il ciclo
					sem_post(mutex);      // avviso che è possibile accedere alla shm
				}
			}
		}else{    // richiesta di operazione di deposito
			printf("il messaggio è una richiesta di deposito\nrispondo che è ammesso\n");
			*shared_memory = 1;    // valore di operazione consentita
			sem_post(mutex);       // avviso che è possibile accedere alla shm
			while(controllo == 1){			
				printf("attendo di sapere quant'è il deposito\n");
				sleep(2);
				sem_wait(mutex);   // attendo di accedere alla shm
				if(*shared_memory <= 0){   // controllo del valore da depositare nel conto corrente
					printf("il deposito non è ammissibile!");					
					*shared_memory = -1;    // operazione fallita
					sem_post(mutex);        // avviso che è possibile accedere alla shm
				}else{
					printf("il deposito è ammissibile\n");
					printf("valore attuale del conto corrente = %d\n", conto_corrente);
					printf("deposito: %d\n", *shared_memory);
					conto_corrente = conto_corrente + *shared_memory;  // modifica del conto corrente
					printf("valore aggiornato del conto corrente = %d\n", conto_corrente);
					*shared_memory = 1;  // operazione avvenuta con successo
					controllo = 0;		// fermo il ciclo
					sem_post(mutex);   // avviso che è possibile accedere alla shm
				}
			}
		}
	}
	return 0;
}