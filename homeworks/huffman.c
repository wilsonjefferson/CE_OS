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
#include <windows.h>

#define START_ASCII_MAX 65
#define END_ASCII_MAX 90
#define START_ASCII_MIN 97
#define ALPHA 26

char **array_bid, **array_bid_1;
int posizione_su_frase, frase_attuale, riga_attuale = 0;
float contatore_caratteri[2][ALPHA];

int* numeri_righe_frasi(int fd){ 
	
	// determino il numero di righe e di frasi del testo
	
	int *descrittore_file = malloc(2*sizeof(int));
	char *sonda = malloc(1*sizeof(char));
	
	descrittore_file[0] = 1; // nella cella 0 si salvano il numero di righe
	descrittore_file[1]= 0; // nella cella 1 si salvano il numero di frasi
	while(read(fd, sonda, 1)>0){
		if(*sonda == '\n'){descrittore_file[0] = descrittore_file[0] + 1;}
		if(*sonda == '.'){descrittore_file[1] = descrittore_file[1] + 1;}
	}
	lseek(fd, 0, SEEK_SET); // per rimettere il puntatore del file nella posizione iniziale
	return descrittore_file;
}

int** numeri_caratteri(int fd, int* array_righe_frasi){
	
	// determiniamo, per ogni riga e per ogni frase, il numero di caratteri
	
	/*
	si osservi che la dimensione di char_per_riga è pari al numero di righe + 1, in quanto nel metodo di conteggio
	del numero di righe siamo partiti da 1 e non da 0 (come per le frasi).
	*/
	
	int i, j, char_per_riga[array_righe_frasi[0]+1], char_per_frase[array_righe_frasi[1]], **descrittore_file;
	char *sonda = malloc(1*sizeof(char));
	descrittore_file = malloc(2*sizeof(int*));
	
	for(i=0; i<array_righe_frasi[0]; i++){char_per_riga[i]=0;} i=0;
	for(i=0; i<array_righe_frasi[1]; i++){char_per_frase[i]=0;} i=0;
	
	i = j = 0;
	while(read(fd, sonda, 1)>0){
		//printf("sonda = %c\n", *sonda);
		if(*sonda != '\n'){char_per_riga[j] = char_per_riga[j] + 1;}else{j++;}
		if(*sonda != '.'){char_per_frase[i] = char_per_frase[i] + 1;}else{i++;}
	} i = j = 0;
	lseek(fd, 0, SEEK_SET);
	
	descrittore_file[0] = char_per_riga;
	descrittore_file[1] = char_per_frase;
	
	// per verifica...
	
	/*
	for(i=0; i<array_righe_frasi[0]; i++){printf("-riga %d: %d\n", i, descrittore_file[0][i]);} i = 0;
	for(i=0; i<array_righe_frasi_f[1]; i++){printf("-frase %d: %d\n", i, descrittore_file[1][i]);} i = 0;
	*/
	
	return descrittore_file;
}

char estrazione(char* riga){
	
	int i, posizione_su_riga = 0;
	//--------INIZIO SOVRASCRITTURA--------//
	int fd = open("/home/Pietro/huffman/testo1.txt", O_RDONLY);
	int *array_righe_frasi = numeri_righe_frasi(fd);
	int **array_caratteri = numeri_caratteri(fd, array_righe_frasi);
	//--------FINE SOVRASCRITTURA--------//
	
	int tot_frasi = array_righe_frasi[1];
	//printf("tot. frasi: %d\n", tot_frasi);
	
	int dimensione_riga =  array_caratteri[0][riga_attuale];
	printf("dimensione riga attuale %d: %d\n", riga_attuale, dimensione_riga);
	
	int dimensione_frase = array_caratteri[1][frase_attuale];
	printf("dimensione frase attuale %d: %d\n", frase_attuale, dimensione_frase);
	
	// stampa della riga con relativa posizione del singolo carattere
	
	for(i=0; i<dimensione_riga; i++){printf("%d\t", i);} i = 0; printf("\n");
	for(i=0; i<dimensione_riga; i++){printf("%c\t", riga[i]);} i = 0; printf("\n");
	
	/*
	da ogni riga che il metodo riceve estraiamo le frasi, in particolare nel caso in cui
	la frase inizi in una riga e si conclude in una delle righe successive, allora si tiene
	traccia della posizione corrente nella frase del testo e, ricevuta la riga successiva,
	si continua il riempimento dell'array_bid rimanendo sulla stessa riga, ma ripartendo
	dalla colonna in cui si era interrotto il metodo.
	
	Nel caso si raggiunge la fine della frase attuale, allora si passa alla frase successiva
	ma spostando l'indice della posizione di riga di due celle in avanti, dato che la frase si
	ferma prima del punto (di conseguenza ci si ritrova direttamente all'inizio della frase dopo).
	*/
	
	while(posizione_su_riga < dimensione_riga){
		//printf("posizione su riga: %d\n", posizione_su_riga);
		//printf("dimensione frase attuale: %d\n", dimensione_frase);
		if((posizione_su_frase < dimensione_frase) && (riga[posizione_su_riga] != '.')){
			//printf("posizione su frase: %d\n", posizione_su_frase);
			array_bid[frase_attuale][posizione_su_frase] = riga[posizione_su_riga];
			//printf("array_bid[%d][%d] = %c\n", frase_attuale, posizione_su_frase, array_bid[frase_attuale][posizione_su_frase]);
			posizione_su_riga++; posizione_su_frase++;
		}else{
			if(frase_attuale < tot_frasi){
				frase_attuale++; posizione_su_riga = posizione_su_riga + 2; posizione_su_frase = 0;
				printf("prossima frase: %d\n", frase_attuale);
				dimensione_frase = array_caratteri[1][frase_attuale];
			}else{printf("siamo in break\n"); break;}
		}
	}
	
	riga_attuale++;
	return '\0';
}

void *thread_function(void *candidato){
	
	/*
	il "candidato" è una lettera ma di tipo puntatore a void, eseguiamo una conversione per
	ottenere il suo equivalente in codice ASCII.
	*/
	
	/*
	possiamo fare anche in quest'altra maniera:
	
	int decode = toascii(*lettera);
	printf("lettera: %c\n", *lettera);
	printf("decode: %d\n", decode);
	*/
	
	int i = 0;
	int decode = *((char*)candidato);
	//printf("conversione automatica della lettera: %d\n", decode);
	//printf("conversione automatica di decode: %c\n", decode);
	
	for(i=0; i<ALPHA; i++){contatore_caratteri[0][i] = i;} i = 0; // n celle pari al n lettere dell'alfabeto
	
	if(decode <= END_ASCII_MAX && decode >= START_ASCII_MAX){ // carattere è maiuscolo
		contatore_caratteri[1][decode-START_ASCII_MAX] = contatore_caratteri[1][decode-START_ASCII_MAX] + 1;
		//printf("contatore_caratteri[%d] = %3.0f\n", decode-START_ASCII_MAX, contatore_caratteri[1][decode-START_ASCII_MAX]);
	}else{ // carattere è minuscolo
		contatore_caratteri[1][decode-START_ASCII_MIN] = contatore_caratteri[1][decode-START_ASCII_MIN] + 1;
		//printf("contatore_caratteri[%d] = %3.0f\n", decode-START_ASCII_MIN, contatore_caratteri[1][decode-START_ASCII_MIN]);
	}
	return NULL;
}

void frequenza(){
	
	/*
	per ogni lettera nell'array_bid creiamo un tread che avrà per arg la lettera stessa, inoltre
	un contatore indipendente tiene traccia di tutte le lettere considerate.
	*/
	
	int i, j, totale_caratteri = 0;
	pthread_t thread;
	
	//--------INIZIO SOVRASCRITTURA--------//
	int fd = open("/home/Pietro/huffman/testo1.txt", O_RDONLY);
	int *array_righe_frasi = numeri_righe_frasi(fd);
	int **array_caratteri = numeri_caratteri(fd, array_righe_frasi);
	//--------FINE SOVRASCRITTURA--------//
	
	int tot_frasi = array_righe_frasi[1];
	//printf("tot. frasi: %d\n", tot_frasi);
	//printf("totale caratteri = %d\n", totale_caratteri);
	
	for(i=0; i<tot_frasi; i++){
		
		//--------INIZIO SOVRASCRITTURA--------//
		close(fd);
		fd = open("/home/Pietro/huffman/testo1.txt", O_RDONLY);
		array_righe_frasi = numeri_righe_frasi(fd);
		array_caratteri = numeri_caratteri(fd, array_righe_frasi);
		//--------FINE SOVRASCRITTURA--------//
		
		int var = array_caratteri[1][i];
		//printf("var: %d\n", var);
		for(j=0; j<var; j++){
			//printf("array_bid[%d][%d] = %c\n",i, j, array_bid[i][j]);
			if(isalpha(array_bid[i][j]) != 0){ // verifica che il carattere sia alfabetico
				totale_caratteri = totale_caratteri + 1;
				//printf("totale caratteri = %d\n", totale_caratteri);
				pthread_create(&thread, NULL, (void*)thread_function, (void*)&array_bid[i][j]);
				pthread_join(thread, NULL);
			}
		}
		printf("-----------------------------\n");
	} i = 0; j = 0;
	
	// calcoliamo la frequenza di ogni lettera dell'alfabeto, riutilizzando il contatore di caratteri.
	
	for(i=0; i<ALPHA; i++){
		contatore_caratteri[1][i] = contatore_caratteri[1][i]/totale_caratteri;
		//printf("frequenza carattere %d: %3.3f\n", i, contatore_caratteri[1][i]);
	} i = 0;
}

void ordina_stampa(){
	
	// ordiniamo il contatore_caratteri in ordine decrescente, tramite insertion sort.
	
	int i, j, indice = 0; 
	float tmp = 0;
	
	printf("avvio ordinamento decrescente delle frequenze.\n");
	
	/*
	per eseguire un ord. crescente sostituiamo nel for annidato la seguente condizione:
	                 j=i-1; (j>=0) && (contatore_caratteri[1][j]>tmp); j--
	*/
	
    for(i=1; i<ALPHA; i++){
		tmp = contatore_caratteri[1][i];
		indice = i;
		for(j=i-1; contatore_caratteri[1][j]<tmp; j--){
			contatore_caratteri[1][j+1] = contatore_caratteri[1][j];
			contatore_caratteri[0][j+1] = contatore_caratteri[0][j];
		}
		contatore_caratteri[1][j+1] = tmp;
		contatore_caratteri[0][j+1] = indice;
	} i = j = indice = tmp = 0;
	
	// cast automatico da intero (codice ASCII) a carattere, usando la prima riga di contatore_caratteri
	
	printf("stampa frequenze.\n");
	for(i=0; i<ALPHA; i++){printf("frequenza lettera %c: %3.3f\n",
			(int)(contatore_caratteri[0][i]+START_ASCII_MIN), contatore_caratteri[1][i]);} i = 0;
}

int main(){
	
	/*
	l'obiettivo è: prendere il numero di righe, di frasi, di caratteri
	per riga e di caratteri per frase; in modo da alloccare una matrice bidimensionale
	con un numero di righe pari al numero di frasi e, per ogni riga, un numero di celle
	pari al numero di caratteri della frase (a cui il numero di riga fa riferimento). Infine
	tale matrice viene riempita e stampata.
	*/
	
	int fd, i, j = 0;
    int	*array_righe_frasi, **array_caratteri;
	char *riga;
	FILE *fp;
	
	fd = open("/home/Pietro/huffman/testo1.txt", O_RDONLY); // lettura del file "testo1.txt"
	printf("fd = %d\n", fd);
	
	fp = fopen("/home/Pietro/huffman/testo1.txt", "r"); // lettura del file "testo1.txt"
	if(fp == NULL){printf("errore in fp.\n"); exit(0);}
	
	array_righe_frasi = numeri_righe_frasi(fd); // array di due celle: n. righe ed n. frasi
	array_caratteri = numeri_caratteri(fd, array_righe_frasi);
	 	
	//	array_caratteri[0][*]: n. caratteri della riga *
	//	array_caratteri[1][*]: n. caratterei della frase *
	
	printf("numero righe: %d\n", array_righe_frasi[0]);
	printf("numero frasi: %d\n", array_righe_frasi[1]);
	
	for(i=0; i<array_righe_frasi[0]; i++){printf("riga %d: %d\n", i, array_caratteri[0][i]);} i = 0;
	for(i=0; i<array_righe_frasi[1]; i++){printf("frase %d: %d\n", i, array_caratteri[1][i]);} i = 0;
	printf("-----------------------------\n");
	
	array_bid = malloc(array_righe_frasi[1]*sizeof(char*)); // tot righe quante sono le frasi
	if(array_bid == NULL){printf("errore nell'allocazione righe.\n"); exit(0);}
	
	//--------INIZIO SOVRASCRITTURA--------//
	close(fd);
	fd = open("/home/Pietro/huffman/testo1.txt", O_RDONLY); 
	array_righe_frasi = numeri_righe_frasi(fd);
	array_caratteri = numeri_caratteri(fd, array_righe_frasi);
	
	array_bid_1 = malloc(array_righe_frasi[1]*sizeof(char*));
	if(array_bid_1 == NULL){printf("errore nell'allocazione righe.\n"); exit(0);}
	//--------FINE SOVRASCRITTURA--------//
	
	//--------INIZIO SOVRASCRITTURA--------//
	close(fd);
	fd = open("/home/Pietro/huffman/testo1.txt", O_RDONLY); 
	array_righe_frasi = numeri_righe_frasi(fd);
	array_caratteri = numeri_caratteri(fd, array_righe_frasi);
	//--------FINE SOVRASCRITTURA--------//
	
	for(i=0; i<array_righe_frasi[1]; i++){	
		array_bid[i] = malloc(array_caratteri[1][i]*sizeof(char)); // tot colonne quanti sono i caratteri per frase
		if(array_bid[i] == NULL){printf("errore in allocazione colonne.\n"); exit(0);}
		
		//--------INIZIO SOVRASCRITTURA--------//
		close(fd);
		fd = open("/home/Pietro/huffman/testo1.txt", O_RDONLY); 
		array_righe_frasi = numeri_righe_frasi(fd);
		array_caratteri = numeri_caratteri(fd, array_righe_frasi);
		
		array_bid_1[i] = malloc(array_caratteri[1][i]*sizeof(char));
		if(array_bid_1[i] == NULL){printf("errore in allocazione colonne.\n"); exit(0);}
		//--------FINE SOVRASCRITTURA--------//
		
	} i = 0;
	
	/*
	allocazione dinamica della singola riga del testo, ma verificando che la riga non
	contenga il carattere '\n': se non lo contiene allora la si invia al metodo "estrazione",
	altrimenti non si fa nulla, e si fa ricominciare il while in modo che passi alla riga "vera".
	*/
	
	while(j<array_righe_frasi[0]){
		//--------INIZIO SOVRASCRITTURA--------//
		close(fd);
		fd = open("/home/Pietro/huffman/testo1.txt", O_RDONLY); 
		array_righe_frasi = numeri_righe_frasi(fd);
		array_caratteri = numeri_caratteri(fd, array_righe_frasi);
		//--------FINE SOVRASCRITTURA--------//
		int var = array_caratteri[0][j]; // n. caratteri della riga j
		
		//printf("allocazione di riga.\n");
		//printf("dimensione riga %d: %d\n", j, var);
		riga = malloc(var*sizeof(char)); // allocco array
		
		fgets(riga, var, fp); // salvo la riga j del testo nell'array "riga"
		
		if(riga[0] == '\n' || riga[1] == '\n'){ //printf("non faccio niente.\n");//non fare nulla, falso positivo
			}else if(riga != NULL){
				printf("riga: #%s#\n", riga);	j++;		
				estrazione(riga); // estrazione frasi dalla riga corrente
			}
	} i = 0; j = 0;
	
	printf("stampa array bidimensionale.\n");
	for(i=0; i<array_righe_frasi[1]; i++){
		//--------INIZIO SOVRASCRITTURA--------//
		close(fd);
		fd = open("/home/Pietro/huffman/testo1.txt", O_RDONLY); 
		array_righe_frasi = numeri_righe_frasi(fd);
		array_caratteri = numeri_caratteri(fd, array_righe_frasi);
		//--------FINE SOVRASCRITTURA--------//
		
		int var = array_caratteri[1][i];
		for(j=0; j<var; j++){	
			printf("%c", array_bid[i][j]);
		}
		printf("\n");
	} i = 0; j = 0;
	
	printf("-----------------------------\n");
	//printf("calcolo frequenza.\n");
	frequenza();
	ordina_stampa();
	
	return 0;
}