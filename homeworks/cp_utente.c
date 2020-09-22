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
	int *shared_memory, shared_open, controllo, random;
	sem_t *account, *utente,*mutex;
	
	srand(time(NULL)); //l'orologio del sistema genera il seme casuale
	errno = 0;
	
    shared_open = shm_open("comunicazione",  O_EXCL | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR); //creazione memoria condivisa
    ftruncate(shared_open, sizeof(int));
    shared_memory = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shared_open, 0); // mappatura in memoria
	
	if(shared_memory == NULL || shared_open == NULL || shared_open == -1){ printf("errore: %d  errore: %s.\n", errno, strerror(errno));}
	
	//sem_unlink("Utente");		// se mando in loop e chiudo da tastiera il semaforo resta a zero
										// se è commentato l'unlink
	
	account = sem_open("Personale", O_CREAT, S_IRUSR|S_IWUSR, 0); //inizializzazione semafori
	mutex = sem_open("Banca", O_CREAT, S_IRUSR|S_IWUSR, 1);
	utente = sem_open("Utente", O_CREAT, S_IRUSR|S_IWUSR, 1);
	
	int val_account[1], val_utente[1], val_mutex[1]; //verifica dei valori dei semafori e shm
	sem_getvalue(account, val_account);
	sem_getvalue(utente, val_utente);
	sem_getvalue(mutex, val_mutex);
	printf("account = %d\n", *val_account);
	printf("utente = %d\n", *val_utente);
	printf("mutex = %d\n", *val_mutex);
	printf("shared_open = %d\n", shared_open);
	printf("shared_memory = %d\n", *shared_memory);
	
	printf("aspetto di poter accedere alla banca.\n");
	sem_wait(utente);
	printf("mi paleso alla banca\n");
	sem_post(account);
	sem_wait(mutex);
	
	printf("inizio delle comunzioni con la banca.\n");
	controllo = 1;
	random = rand()%2;  //scelta casuale fra 0 e 1
	printf("random = %d\n", random);
	if(random != 1){	//prelievo
		printf("richiedo di eseguire l'operazione di prelievo.\n");
		*shared_memory = -1;
		sem_post(mutex);
		printf("attendo il permesso.\n");
		sleep(1);
		sem_wait(mutex);
		
		if(*shared_memory > 0){ 
			printf("l'operazione è stata accettata!\n");
			while(controllo == 1){
				*shared_memory = 10;
				printf("prelievo da richiedere: %d\n", *shared_memory);
				printf("richiedo il prelievo e attendo la risposta.\n");
				sem_post(mutex);
				sleep(1);
				sem_wait(mutex);
				if(*shared_memory > 0){
					printf("ho prelevato!\n");
					sem_post(mutex);
					sem_post(utente);
					controllo = 0;
				}else{
					printf("l'operazione è stata rifiutata ma posso ritentare.\n");
				}
			}
		}else{
			printf("l'operazione è stata rifiutata!\n");
			sem_post(mutex);
			sem_post(utente);
		}
	}else{ //deposito
		printf("richiedo di eseguire l'operazione di deposito.\n");
		*shared_memory = 1;
		sem_post(mutex);
		printf("attendo il permesso.\n");
		sleep(1);
		sem_wait(mutex);
		
		if(*shared_memory > 0){
			printf("l'operazione è stata accettata!\n");
			while(controllo == 1){
				*shared_memory = 10;
				printf("deposito da richiedere: %d\n", *shared_memory);
				printf("richiedo il deposito e attendo la risposta.\n");
				sem_post(mutex);
				sleep(1);
				sem_wait(mutex);
				if(*shared_memory > 0){
					printf("ho depositato!\n");
					sem_post(mutex);
					sem_post(utente);
					controllo = 0;
				}else{
					printf("l'operazione è stata rifiutata ma posso ritentare\n");
				}
			}
		}else{
			printf("l'operazione è stata rifiutata!\n");
			sem_post(mutex);
			sem_post(utente);
		}
	}
	munmap(shared_memory, sizeof(int));
	return 0;
}