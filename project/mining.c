#include <sys/types.h> // per utilizzare variabili di tipo pthread_t
#include <stdlib.h> // utilità generale (allocazione, controllo processi, etc.)
#include <pthread.h> // per utilizzo dei thread
#include <string.h> // per elaborazione su stringhe
#include <math.h>

#include <gsl/gsl_vector.h> // per usare i vettori
#include <gsl/gsl_matrix.h> // per usare le matrici

#include <gsl/gsl_eigen.h> // per calcolare autovalori e autovettori

#include <gsl/gsl_statistics.h> // per usare funzioni di probabilità

#include <gsl/gsl_linalg.h> // per usare funzioni su matrici
#include <gsl/gsl_permutation.h> // per usare le permutazioni

#define debug 0 // variabile per settare le stampe di controllo
#define troncamento 100 // per approssimazione alla 3° cifra significativa dopo la virgola

int controllo = 1; // variabile per verificare la chiamata di funzioni

double **data_set_origine, **data_set, **data_compression, **autovettori;
double *autovalori;
int dimensione = 0;

struct registro{ // puntatori alle celle per eseguire il prodotto matriciale A*B
	double *mtx_1, *mtx_2; //mtx_1: riga di A, mtx_2: colonna di B
	int *indici; // posizione in cui salvare il singolo valore dell'operazione
} catalogo;

struct gsl{ // puntatori agli autovalori e autovettori di tipo gsl
	gsl_vector *vector; // autovalori
	gsl_matrix *matrix; // autovettori
} puntatori_gsl;

void stampa(int tipo, double **mt1, double *mt2, char nome[], int nr, int nc){ // il tipo indica che tipo di stampa effettuo
	int i,j = 0; 
	switch(tipo){
		case 0: //stampa dimensione
			printf("%s: %d \n", nome, nr); //Dimensioni 
		break;
		case 1: //stampa la matrice con il proprio nome
			printf("Stampa matrice %s: \n", nome);
			for(i = 0; i < nr; i++){
				for(j = 0; j < nc; j++){
					if(j == 0){
						printf("|   ");
					}
					printf("%2.2f\t", mt1[i][j]);
					if(j == nc-1){
						printf("|");
					}
				}
				printf("\n");
			}
		break;
		case 2://stampa media in forma estesa
			printf("Stampa valor medio: \n");
			for(i = 0; i < nr; i++){
				printf("media[%d]=%2.2f \n", i, mt2[i]);
			}
		break;
		case 3: //stampa gli autovalori in forma matriciale
			printf("Stampa autovalori(covarianza): \n");
			for(i = 0; i < dimensione; i++){
				if(i == 0){
					printf("|   ");
				}
				printf("%2.4g\t", autovalori[i]);
				if(i == dimensione-1){
					printf("| \n");
				}
			}
		break;
		case 5: //stampa errori verifica
			printf("Errore la verifica non è andata a buon fine\n");
			printf("La matrice Verifica è diversa dalla matrice di partenza \n");
		break;
		case 6://stampa matrici monodimensionali in maniera estesa
			printf("Stampa Matrice Estesa %s: \n", nome);
			for(i = 0; i < dimensione; i++){
				printf("%s[%d]=%2.2f \n", nome, i, mt2[i]);
			}
		break;
		case 7: //stampa matrici bidimensionali in maniera estesa
			printf("Stampa Matrice Estesa %s : \n", nome);
			for(i = 0; i < dimensione; i++){
				for(j = 0; j < dimensione; j++){
					printf("%s[%d][%d]=%2.2f \n", nome, i, j, mt1[i][j]);
				}
			}
		break;
	}
	printf("\n");
}

void *thread_function(void *candidato){ // singola iterazione del prodotto matriciale
	int i = 0;
    int *indici;
	double *mtx_1, *mtx_2;
	struct registro *catalogo;
	
	// casting degli argomenti della thread_function
	catalogo=(struct registro*)candidato;
	mtx_1=catalogo->mtx_1;
	mtx_2=catalogo->mtx_2;
	indici=catalogo->indici;
	
	/*
		esecuzione del prodotto matriciale (a singoli elementi): si esegue il singolo prodotto, il risultato
		lo si salva sulla posizione corrente del'array tmp[0]; infine si somma il precedente risultato al valore presente
		nella cella 0 di tmp (tmp[0][0] svolgerà il ruolo di sommatore, senza dover creare una variabile temporanea).
	*/
	for(i=0; i<dimensione; i++){
		if(debug==1){printf("mtx_1[%d] = %2.2f  ,  mtx_2[%d] = %2.2f\n", i, mtx_1[i], i, mtx_2[i]);}
		mtx_1[i]=mtx_1[i]*mtx_2[i];
		if(debug==1){printf("singolo prodotto: %2.2f\n", mtx_1[i]);}
		if(i>0){
			mtx_1[0]=mtx_1[0]+mtx_1[i];
			if(debug==1){printf("somma: %2.2f\n", mtx_1[0]);}
		}
	} i=0;
	
	// inserimento della somma nell'apposita cella della matrice globale: data_compression
	data_compression[indici[0]][indici[1]]=mtx_1[0];	
	if(debug==1){
		printf("data_compression[%d][%d]=%2.2f\n", indici[0], indici[1], data_compression[indici[0]][indici[1]]);
	}
	
	return NULL;
}

double *media(int nrighe){ // media sulle righe del data set
	int i, j = 0;
	double somma = 0;
	double *media;
	
	media=malloc(nrighe*sizeof(double)); // alloccamento dinamico
	if(media==NULL){printf("ERRORE: media non alloccata.\n");}
	
	for(i=0; i<nrighe; i++){
		for(j=0; j<dimensione; j++){
			somma = somma+data_set[i][j]; // somma su riga i-esima del data_set
			if(debug==1){printf("somma = %2.2f\n", somma);}
		}
		media[i]=somma/dimensione; // valor medio della i-esima riga del data_set
		somma=0; // reset per riga successiva
		if(debug){printf("Media: %2.2f \n", media[i]);}
	}
	
	return media;
}

double **covarianza(double *media, int nrighe){ // matrice di covarianza del data set
	int i, j, k = 0;
	double **covarianza;
	double	*tmp1, *tmp2;
	
	// alloccamento dinamico
	tmp1=malloc(nrighe*sizeof(double));
	if(tmp1==NULL){printf("ERRORE - covarianza: tmp1 non alloccato.\n"); exit(0);}
	
	tmp2=malloc(nrighe*sizeof(double));
	if(tmp2==NULL){printf("ERRORE - covarianza: tmp2 non alloccato.\n"); exit(0);}

	covarianza=malloc(dimensione*sizeof(double*));
	if(covarianza==NULL){printf("ERRORE - covarianza: covarianza non alloccato.\n"); exit(0);}

	for(i=0; i<dimensione; i++){
		covarianza[i]=malloc(dimensione*sizeof(double));
		if(covarianza[i]==NULL){printf("ERRORE - covarianza: covarianza[%d] non alloccato.\n", i); exit(0);}
	}
	
	/*
		dei 4 for, definiamo "principali" quelli riferiti alla variabile 'i' e 'j' in quanto vanno ad identificare la cella  (i, j) della matrice di
		covarianza; i for riferiti alla variabile 'k' sono for di settaggio dei due array temporanei, che andranno a considerare le colonne
		del data set.
		Ad ogni ciclo dell' i-for si fisserà una riga (partendo dalla riga 0), e si andrà a "combinare" con tutte le colonne del data set (a 
		partire dalla riga 0).
	*/
	
	for(i=0; i<dimensione; i++){
		for(k=0; k<nrighe; k++){ // fissiamo colonna i-esima
			tmp1[k]=data_set[k][i];
			if(debug==1){
				stampa(6, NULL, tmp1, "tmp1", 0, 0);
			}
		} 
		for(j=0; j<dimensione; j++){ 
			for(k=0; k<nrighe; k++){ // consideriamo colonna j-esima
				tmp2[k]=data_set[k][j];
				if(debug==1){
					stampa(6, NULL, tmp2, "tmp2", 0, 0);
				}
			} 
			covarianza[i][j]=gsl_stats_covariance(tmp1, 1, tmp2, 1, nrighe); // calcolo della covarianza
		}
		if(debug==1){
			stampa(7, covarianza, NULL, "Covarianza", 0, 0);
		}
	}
	
	free(tmp1);
	free(tmp2);
	
	return covarianza;
}

struct gsl eigen(double **covarianza){ // calcolo autovalori e autovettori della matrice di covarianza
	int i=0, j=0;
	gsl_matrix *matrice, *evec;
	gsl_vector *evel;
	gsl_eigen_symmv_workspace *campo;
	struct gsl pointer_gsl;
	
	// allocazione
	matrice=gsl_matrix_alloc(dimensione, dimensione); // alloccamento matrice bid: "dimensione" righe e "dimensione" colonne
	if(matrice==NULL){printf("ERRORE - eigen: matrice non alloccato"); exit(0);}
	
	evec=gsl_matrix_alloc(dimensione, dimensione); // alloccamento matrice bid: "dimensione" righe e "dimensione" colonne
	if(evec==NULL){printf("ERRORE - eigen: evec non alloccato"); exit(0);}
	
	evel=gsl_vector_calloc(dimensione); // alloccamento vettore: "dimensione" posizioni
	if(evel==NULL){printf("ERRORE - eigen: evel non alloccato"); exit(0);}
	
	campo=gsl_eigen_symmv_alloc(dimensione); // alloccamento del workspace, dimensione pari a: "dimensione"
	if(campo==NULL){printf("ERRORE - eigen: campo non alloccato"); exit(0);}
	
	// copiamo "covarianza" nella "matrice"
	for(i=0; i<dimensione; i++){
		for(j=0; j<dimensione; j++){
			if(debug==1){printf("covarianza[%d][%d] = %2.2f\n", i, j, covarianza[i][j]);}
			gsl_matrix_set(matrice, i, j, covarianza[i][j]); // inserimento del valore nella "matrice"
			if(debug==1){printf("matrice[%d][%d] = %2.2f\n", i, j, gsl_matrix_get(matrice, i, j));}
		}
	}
	
	controllo=gsl_eigen_symmv(matrice, evel, evec, campo); // calcolo degli autovalori e autovettori
	if(controllo!=0){printf("ERRORE - eigen: gsl_eigen_symmv fallito.\n"); exit(0);}
	
	// salvataggio dei puntatori agli autovalori  e autovettori nella struttura "gsl"
	pointer_gsl.vector=evel;
	pointer_gsl.matrix=evec;
	
	gsl_matrix_free(matrice);
	gsl_eigen_symmv_free(campo);
	
	return pointer_gsl;
}

void ordinamento(gsl_matrix *vettori){ // ordinamento decrescente degli autovalori
	int i,j = 0;
	double tmp = 0;
	
	for(i=0; i<dimensione; i++){
		if(autovalori[i]<0){
			autovalori[i]=autovalori[i]*(-1);
		}
	}
	
	/*
		dato che c'è una corrispondenza biunivoca fra l'array degli autovalori e la matrice degli autovettori, allora
		ordinando gli autovalori si ordinano anche gli autovettori, per mantenere la precedente proprietà.
	*/
	for(i = 0; i < dimensione; i++){
		for(j = 0; j < dimensione; j++){
			if(debug==1){printf("autovalori[%d] = %2.2f  ,  autovalori[%d] = %2.2f\n", j, autovalori[j], i, autovalori[i]);}
			if(autovalori[j] < autovalori[i]){
				tmp=autovalori[i];
				if(debug==1){printf("tmp = %2.2f\n", tmp);}
				autovalori[i]=autovalori[j];
				autovalori[j]=tmp;
				if(debug==1){printf("autovalori[%d] = %2.2f  ,  autovalori[%d] = %2.2f\n", j, autovalori[j], i, autovalori[i]);}
				gsl_matrix_swap_columns(vettori, i, j);
				if(controllo!=0){printf("ERRORE - ordinamento: gsl_matrix_swap_columns fallito.\n"); exit(0);}
			}
		}
		tmp=0;
	}
}

double **sottrazione_matriciale(int nrighe, double *media){ // data set "differenziato"
	int i, j = 0;
	double **sottrazione;
	
	// alloccamento dinamico
	sottrazione=malloc(nrighe*sizeof(double*));
	if(sottrazione==NULL){printf("ERRORE - sottrazione_matriciale: sottrazione non alloccato.\n"); exit(0);}
	for(i=0; i<nrighe; i++){
		sottrazione[i]=malloc(dimensione*sizeof(double));
		if(sottrazione[i]==NULL){printf("ERRORE - sottrazione_matriciale: sottrazione[%d] non alloccato.\n", i); exit(0);}
	}
	
	// ogni riga del data set viene differenziato per la media della riga stessa
	for(i=0; i<nrighe; i++){
		for(j=0; j<dimensione; j++){
			sottrazione[i][j]=data_set[i][j]-media[i];
			if(debug==1){printf("data_set[%d][%d] = %2.2f  ,  media[%d] = %2.2f  ,  sottrazione[%d][%d] = %2.2f\n",
					i, j, data_set[i][j], i, media[i], i, j, sottrazione[i][j]);
			}
		}
	}
	
	return sottrazione;
}

int prodotto_matriciale(int righe_1, int colonne_1, int righe_2, int colonne_2, double **matrice_1, double **matrice_2){ //Funzione per fare il prodotto matriciale
	int i, i_1, j_1, contatore, fine = 0;
	int *indici;
	double *mtx_1, *mtx_2;
	pthread_t *thread;
	
	struct registro *enciclopedia; // array di strutture registro
	
	if(colonne_1 != righe_2){
		printf("ERRORE - prodotto_matriciale: non è soddisfatta la condizione del prodotto matriciale.\n");
		return -1;
	}
	
	enciclopedia=malloc(righe_1*colonne_2*sizeof(struct registro)); //creazione struttura
	if(enciclopedia==NULL){printf("ERRORE - prodotto_matriciale: enciclopedia non alloccata.\n"); exit(0);}
	
	/*
		Definiamo come "prodotto", il prodotto dei valori (in stessa posizione) fra due array, e successiva somma dei singoli
		prodotti.
		Definiamo come "singola iterazione" o "singola procedura" il prodotto fra la riga i-esima della matrice A per la colonna
		j-esima della matrice B, al variare di i e j.
	*/
		
		// settaggio dei parametri da inviare ai thread (obbligatori, altrimenti parte il segmentation fault)
		contatore=0;
		i_1=0;
		j_1=0;
		
		/*
			Il seguente while serve a riempire l'array di strutture che contengono: la riga di A, la colonna di B e gli indici di posizione
			in cui salvare il risultato della singola procedura del prodotto matriciale.
		*/
		while(fine==0){ // alloccamento dinamico di due array "temporanei" e salvataggio indirizzi nell'array tmp
		
			mtx_1=malloc(colonne_1*sizeof(double)); // riga di A
			if(mtx_1==NULL){printf("ERRORE - prodotto_matriciale: mtx_1 non alloccato.\n"); exit(0);}
			
			mtx_2=malloc(righe_2*sizeof(double*)); // colonna di B
			if(mtx_2==NULL){printf("ERRORE - prodotto_matriciale: mtx_2 non alloccato.\n"); exit(0);}
			
			indici=malloc(2*sizeof(int)); // array degli indici di posizione del data_compression
			if(indici==NULL){printf("ERRORE - prodotto_matriciale: indici non alloccato.\n"); exit(0);}
			
			if(debug==1){
				printf("contatore = %d, colonne_2=%d, righe_1=%d, i_1=%d, j_1=%d\n", 
																	contatore, colonne_2, righe_1, i_1, j_1);
				}
			
			for(i = 0; i < colonne_1; i++){
				mtx_1[i]=matrice_1[i_1][i]; // copia della riga i_1 di A in mtx_1
				mtx_2[i]=matrice_2[i][j_1]; // copia della colonna j_1 di B in mtx_2
			}
			
			if(debug==1){
				stampa(6, NULL, mtx_1, "MTX_1", 0, 0);
				stampa(6, NULL, mtx_2, "MTX_2", 0, 0);
			}
			
		
			// salvataggio dimensioni ed indici di posizione nella singola struttura
			indici[0]=i_1; 
			indici[1]=j_1;
			
			// salvataggio dei parametri in una struttura "registro"
			enciclopedia[contatore].mtx_1 = mtx_1;
			enciclopedia[contatore].mtx_2 = mtx_2;
			enciclopedia[contatore].indici = indici;
			
			contatore++; // rappresenta l'indice dell'array di struttura
			if(debug==1){
				printf("contatore = %d\n", contatore);
			}
			
			if(debug==1){
				printf("j_1+1 = %d  ,  colonne_2 = %d  ,  i_1+1 = %d  ,  righe_1 = %d\n", j_1+1, colonne_2, i_1+1, righe_1);
				}
			if(j_1+1<colonne_2){ // andiamo alla colonna successiva di B
				j_1++;
				if(debug==1){
					printf("j_1 = %d\n", j_1);
				}
			}else if(i_1+1<righe_1){ // andiamo alla riga successiva di A
				i_1++;
				j_1=0; // la riga parte dalla colonna 0
				if(debug==1){
					printf("i_1 = %d\n", i_1);
				}
			}else{
				fine=1; // sono stati passati tutti i valori, il while è finito
			} 
		} 
		contatore=0;
		i=0;
		i_1=0;
		j_1=0;
		fine=0;
	
		// alloccamento array di thread
		thread=malloc(righe_1*colonne_2*sizeof(pthread_t));
		if(thread==NULL){
			printf("ERRORE - prodotto_matriciale: thread non alloccato.\n"); 
			exit(0);
		}
	
		// verifica del corretto contenuto delle strutture
		if(debug==1){
			int j = 0;
			for(i=0; i<righe_1*colonne_2; i++){
				printf("Stampa del contenuto delle strutture.\n");
				printf("------------\n");
				for(j=0; j<colonne_1; j++){			
					printf("enciclopedia[%d].mtx_1[%d]= %2.2f\n", i, j, enciclopedia[i].mtx_1[j]);
				}
				for(j=0; j<colonne_1; j++){
					printf("enciclopedia[%d].mtx_2[%d]= %2.2f\n", i, j, enciclopedia[i].mtx_2[j]);
				}
				for(j=0; j<2; j++){
					printf("enciclopedia[%d].indici[%d]=%d\n", i, j, enciclopedia[i].indici[j]);
				}
			}
			printf("------------\n");
		}
		
		for(i=0; i<righe_1*colonne_2; i++){
			
			/*
			chiamata del singolo thread, gli argomenti sono: riga della matrice 1, colonna della matrice 2, indice della riga considerata e
			indice della  colonna considerata.
			*/
			//printf("------------\n");
			controllo=pthread_create(&thread[i], NULL, (void*)thread_function, (void*)&enciclopedia[i]);
			if(controllo!=0){printf("ERRORE - prodotto_matriciale: pthread_create fallito.\n"); exit(0);}
			//printf("------------\n");
		}
		controllo=pthread_join(thread[(righe_1*colonne_2)-1], NULL); // il main attende l'ultimo thread prima di proseguire
		if(controllo!=0){printf("ERRORE - prodotto_matriciale: pthread_join fallito.\n"); exit(0);}
		
		free(mtx_1);
		free(mtx_2);
		free(indici);
		free(enciclopedia);
		
		return 0;
}

double **inversione_matriciale(gsl_matrix *evec){ // matrice degli autovettori invertita
	int i, j = 0;
	int *segno;
	double **inversione;
	gsl_matrix *inverse;
	gsl_permutation *permutazione;
	
	// alloccamento dinamico
	inversione=malloc(dimensione*sizeof(double*));
	if(inversione==NULL){printf("ERRORE - inversione_matriciale: inversione non alloccato.\n"); exit(0);}
	for(i=0; i<dimensione; i++){
		inversione[i]=malloc(dimensione*sizeof(double));
		if(inversione[i]==NULL){printf("ERRORE - inversione_matriciale: inversione[%d] non alloccato.\n", i); exit(0);}
	}
	
	inverse=gsl_matrix_alloc(dimensione, dimensione); //alloccamento della matrice inversa
	if(inverse==NULL){printf("ERRORE - inversione_matriciale: inverse non alloccato.\n"); exit(0);}
	
	permutazione=gsl_permutation_calloc(dimensione); // alloccamento dell'array di permutazione
	if(permutazione==NULL){printf("ERRORE - inversione_matriciale: permutazione non alloccato.\n"); exit(0);}
	
	segno=malloc(1*sizeof(int)); // inizializzazione della variabile
	if(segno==NULL){printf("ERRORE - inversione_matriciale: segno non alloccato.\n"); exit(0);}
	
	controllo=gsl_linalg_LU_decomp(evec, permutazione, segno); //decompoposizione (obbligatorio)
	if(controllo!=0){printf("ERRORE - inversione_matriciale: gsl_linalg_LU_decomp fallito.\n");}
	
	controllo=gsl_linalg_LU_invert(evec, permutazione, inverse); //inversione matriciale
	if(controllo!=0){printf("ERRORE - inversione_matriciale: gsl_linalg_LU_invert fallito.\n");}
	
	// matrice inversa inserita in una matrice "non di libreria"
	for(i=0; i<dimensione; i++){
		for(j=0; j<dimensione; j++){
			inversione[i][j]=gsl_matrix_get(inverse, i, j);
		}
	}
	gsl_matrix_free(inverse);
	gsl_permutation_free(permutazione);
	free(segno);
	
	return inversione;
}

double **addizione_matriciale(int nrighe, double *media){ // data compression "addizionato"
	int i, j = 0;
	double **addizione;
	
	// alloccamento dinamico
	addizione=malloc(nrighe*sizeof(double*));
	if(addizione==NULL){printf("ERRORE - addizione_matriciale: addizione non alloccato.\n"); exit(0);}
	for(i=0; i<nrighe; i++){
		addizione[i]=malloc(dimensione*sizeof(double));
		if(addizione[i]==NULL){printf("ERRORE - addizione_matriciale: addizione[%d] non alloccato.\n", i); exit(0);}
	}
	
	// ogni riga del data compression viene addizzionato per la media (della stessa riga) del data set
	for(i=0; i<nrighe; i++){
		for(j=0; j<dimensione; j++){
			addizione[i][j]=data_compression[i][j]+media[i];
			if(debug==1){printf("data_compression[%d][%d] = %2.2f  ,  media[%d] = %2.2f  ,  addizione[%d][%d] = %2.2f\n",
					i, j, data_compression[i][j], i, media[i], i, j, addizione[i][j]);
			}
		}
	}
	
	return addizione;
}

double **inversione_operazione(int nrighe, double *media, gsl_matrix *evec){ // invertiamo l'algoritmo
	double **verifica;
	
	// verifica = data_compression*autovettori(invertiti) + media_righe_data_set = data_set
	
	// operazione di inversione della matrice
	verifica=inversione_matriciale(evec); 
	//stampa(1, verifica, NULL, "Inversa", dimensione, 0);
	
	// operazione di prodotto matriciale
	if(0 != prodotto_matriciale(nrighe, dimensione, dimensione, dimensione, data_compression, verifica)){
		printf("ERRORE - inversione_operazione: funzione prodotto_matriciale ha riportato un errore, esecuzione arrestata.\n");
		exit(0);
	} 
	
	//stampa(1, data_compression, NULL, "Prodotto Matriciale", nrighe, 0);
	
	verifica=addizione_matriciale(nrighe, media); // operazione di somma matriciale
	
	return verifica;
}

void trasponi(int nr, int nc){ // esegue la trasposizione della matrice
	int i, j;
	j = 0;
	
	for(j = 0; j < nc; j++){
		for(i = 0; i < nr; i++){
				data_set[j][i] = data_set_origine[i][j];
			}
	}
}

void ottieni(char stringa[], int riga){ // estrazione del data set
	int set=0, last_set=0;
	int i, j=0, k;
	char buf[100];
	double a;
	
	k=0;
	for(i = 0; i < strlen(stringa); i++){ //per leggere tutti i caratteri
		if( (((int)stringa[i]) == 43) || (((int)stringa[i]) == 45) || (((int)stringa[i]) == 46) || ((((int)stringa[i]) > 47) && (((int)stringa[i]) < 58))){
			set=1; //dico che erano caratteri 
			buf[j]=stringa[i]; //salvo il numero
			j++; //incremento il puntatore
		}else{
			set=0;
		}
		if((set==0 && last_set==1) || (set==1 && i == (strlen(stringa)-1))){ //controllo se ho letto tutto il numero
			a=atof(buf); //converto la stringa in un double
			data_set_origine[riga][k]=a; //salvo il numero
			k++; //incremento la colonna
			strcpy(buf, ""); //svuoto il buf
			j=0;//pongo il puntatore del buf a 0 così da poter leggere il numero seguente
		}
		last_set=set; //dico qual'era il valore precedente
	}
}

int colonne(char stringa[]){ //conto il numero di colonne
	int set=0, last_set=0; //variabili per puntare il carattere e il penultimo carattere
	int i, j=0; //variabili locali, j è il numero di colonne
	
	for(i = 0; i < strlen(stringa); i++){
		/*controllo se il carattere in questione è un numero, il segno + o -, oppure il .*/
		if( (((int)stringa[i]) == 43) || (((int)stringa[i]) == 45) || (((int)stringa[i]) == 46) || ((((int)stringa[i]) > 47) && (((int)stringa[i]) < 58))){ 
			set=1; 
		}else{
			set=0;
		}
		if((last_set == 1 && set == 0)||(set == 1 && i == (strlen(stringa) -1))){ //controllo se ho finito di leggere il numero
			j++;
		}
		last_set=set;
	}
	return j; //ritorno il numero di colonne
}

int lettura(){ // funzione per leggere il file in input
	FILE *fp; // file per la lettura
	char pathname[1000]; //percorso file
	char buf[2000]; 
	char *s;
	int i=0, nrighe=0, ncolonne=1;
	int j=0;
	
	/*prendo in ingresso il nome del file*/
	printf("Inserisci l'indirizzo del file: \n");
	scanf("%s", pathname);
	if( (fp = fopen(pathname, "r")) == NULL ){ // apro il file e controllo se è avvenuto con successo
		printf("ERRORE - lettura: apertura file %s fallito.\n", pathname);
		exit(0);
	}
	
	/*inizio la lettura del file*/
	while(1){ // conto il numero di righe e colonne
		stpcpy(buf, "");
		s = fgets(buf, sizeof(buf), fp); 
		if(s == NULL) break; // se la linea è vuota ho letto tutto il file
		nrighe++; // conto le righe
		if(j == 0){
			ncolonne=colonne(buf);
			j++;
		}
	}
	
	data_set_origine=malloc(nrighe*sizeof(double*));//allocco la memoria del data set
	data_set=malloc(ncolonne*sizeof(double*));
	
	if(data_set == NULL){ 
		printf("ERRORE - lettura: data_set non alloccato.\n"); 
		exit(0);
	}
	
	if(data_set_origine == NULL){ 
		printf("ERRORE - lettura: data_set non alloccato.\n"); 
		exit(0);
	}
	
	for(i = 0; i < nrighe; i++){//alloco tutta la memoria
		data_set_origine[i]=malloc(ncolonne*sizeof(double));
		data_set[i]=malloc(nrighe*sizeof(double));
		
		if(data_set[i] == NULL){
			printf("ERRORE - lettura: data_set[%d] non alloccato.\n", i); 
			exit(0);
		}
		
		if(data_set_origine[i] == NULL){
			printf("ERRORE - lettura: data_set_origine[%d] non alloccato.\n", i); 
			exit(0);
		}
	}
	
	rewind(fp);
	
	for(i = 0; i < nrighe; i++){	
		stpcpy(buf, "");
		s = fgets(buf, sizeof(buf), fp); 
		ottieni(buf, i);
	}
	
	fclose(fp);
	
	trasponi(nrighe, ncolonne);
	
	dimensione = nrighe;
	return ncolonne;
}

int main(int argc, char **argv){
	int nrighe, i, j = 0; //variabili di gestione 
	double *valor_medio, **matrice_covarianza, **sottrazione, **verifica; //array per il salvataggio dei dati
	struct gsl pointer_gsl; //inizializzazione struttura
	
	if(debug){printf("MODALITA' DEBUG ATTIVA\n");}
	 
	nrighe=lettura(); // apertura e lettura del data set, nrighe=ncolonne perchè abbiamo trasposto la matrice
	
	stampa(0, NULL, NULL, "nrighe", nrighe, 0);
	stampa(0, NULL, NULL, "Dimensione del Data_set", dimensione, 0); // stampa dimensioni del data set
	
	//stampa(1, data_set_origine, NULL, "Data Set", dimensione, nrighe);
	stampa(1, data_set, NULL, "Data Set Trasposto", nrighe, dimensione);
	
	// alloccamento dinamico
	data_compression=malloc(nrighe*sizeof(double*));
	if(data_compression==NULL){printf("ERRORE - main: data_compression non alloccato.\n"); exit(0);}
	for(i=0; i<nrighe; i++){
		data_compression[i]=malloc(dimensione*sizeof(double));
		if(data_compression[i]==NULL){printf("ERRORE - main: data_compression[%d] non alloccato.\n", i); exit(0);}
	}
	
	autovalori=malloc(dimensione*sizeof(double));
	if(autovalori==NULL){printf("ERRORE - main: autovalori non alloccato.\n"); exit(0);}
	
	autovettori=malloc(dimensione*sizeof(double*));
	if(autovettori==NULL){printf("ERRORE - main: autovettori non alloccato.\n"); exit(0);}
	for(i=0; i<dimensione; i++){
		autovettori[i]=malloc(dimensione*sizeof(double));
		if(autovettori==NULL){printf("ERRORE - main: autovettori[%d] non alloccato.\n", i); exit(0);}
	}
	
	valor_medio=media(nrighe); // calcolo dei valor medi
	//stampa(2, NULL, valor_medio, NULL, nrighe, 0);
	
	matrice_covarianza=covarianza(valor_medio, nrighe); // calcolo della matrice di covarianza
	//stampa(1, matrice_covarianza, NULL, "Covarianza", dimensione, dimensione);
	
	pointer_gsl=eigen(matrice_covarianza); // calcolo degli autovalori e autovettori
	
	for(i=0; i<dimensione; i++){ // settaggio degli autovalori nell'apposito array		
		autovalori[i]=gsl_vector_get(pointer_gsl.vector, i);
	}
	
	//stampa(3, NULL, NULL, NULL, 0, 0); // stampa autovalori (non ordinanti)
	
	gsl_vector_free(pointer_gsl.vector);
	
	ordinamento(pointer_gsl.matrix); // ordinamento decrescente di autovalori e autovettori
	
	for(i=0; i<dimensione; i++){ // settaggio degli autovettori negll'apposito array
		for(j=0; j<dimensione; j++){
			autovettori[i][j]=gsl_matrix_get(pointer_gsl.matrix, i, j); // estrazione del valore da "evec"
			if(autovettori[i]==NULL){printf("ERRORE - main: gsl_matrix_get fallito.\n"); exit(0);}
		}
	}
	
	//stampa(3, NULL, NULL, NULL, 0, 0); // stampa autovalori
	//stampa(1, autovettori, NULL, "Autovettori", dimensione, dimensione); // stampa autovettori
	
	sottrazione=sottrazione_matriciale(nrighe, valor_medio); // operazione di sottrazione matriciale
	//stampa(1, sottrazione, NULL, "Sottrazione", nrighe, 0);
	
	if(0 != prodotto_matriciale(nrighe, dimensione, dimensione, dimensione, sottrazione, autovettori)){ // operazione di prodotto matriciale
		printf("ERRORE - main: funzione prodotto_matriciale ha riportato un errore, esecuzione arrestata.\n");
		return 0;
	} 
	
	stampa(1, data_compression, NULL, "Data Compression", nrighe, dimensione);
	
	free(sottrazione);
	
	verifica=inversione_operazione(nrighe, valor_medio, pointer_gsl.matrix); // operazione inversa (ricavo il data set)
	stampa(1,  verifica, NULL, "Verifica", nrighe, dimensione);
	
	gsl_matrix_free(pointer_gsl.matrix);
	
	stampa(1, data_set, NULL, "Dataset", nrighe, dimensione);
	
	for(i=0; i<nrighe; i++){ // verifica della buona riuscita dell'operazione d'inversione
		for(j=0; j<dimensione; j++){
			//tronco alla 3° cifra decimale per trascurare piccole variazioni, dovute ad errori di approssimazione
			if((floor(data_set[i][j]*troncamento)/troncamento) != ((floor(verifica[i][j]*troncamento))/troncamento)){
				stampa(5, NULL, NULL, NULL, 0, 0);
			}
		}
	}
	printf("programma terminato.\n");
	
	return 0;
}