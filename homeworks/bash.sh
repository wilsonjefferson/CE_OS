#!/bin/bash
while true
do
typeset -a carattere=( ) #carattere: array di caratteri della parola
typeset -a speciali=( ) #speciali: array che individua la posizione delle parole speciali
typeset -a n_speciali=( ) #n_speciali: array che conta le parole speciali, distinguendole
typeset -a mappatura=( ) #mappatura: array che identifica  il tipo di parola speciale
typeset -a parsing=( ) #parsing: array del parsing

typeset -i scorrimento_parola=0 #puntatore_1 sulla parola
typeset -i indice_parola=0 #puntatore_2 sulla parola e lunghezza della parola
typeset -i indice=0 #puntatore sulla riga, distingue le parole della riga di comando

typeset -i test=1 #identifica i caratteri non speciali nella parola
typeset -i allerta=0 #potenziale parola speciale se settato ad uno
typeset -i redirect=0 #parola speciale confermata
typeset -i memoria=0 #tiene traccia dei caratteri che costituiscono una parola speciale
typeset -i fuga=1 #in caso di errori rilevati

typeset -i n=0 #variabile standard: servirà nella fase di esecuzione della riga di comando
typeset -i conteggio_pipe=0 #fa assumere un comportamento diverso in base alla presenza di più pipe
typeset -i tmp1=0 #seleziona la parola precedente nell'array di parsing
typeset -i tmp2=0 #seleziona la parola successiva nell'array di parsing
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
echo "dammi una riga di comando"
read riga
{ #analisi parola per parola della riga
for parola in ${riga}
do
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
echo "parola: ${parola}"
{ #lunghezza parola
while (( indice_parola<${#parola} ))
do
	carattere[indice_parola]=${parola:indice_parola:1} #carattere preso dalla posizione indice_parola
	echo "carattere[${indice_parola}] = ${carattere[${indice_parola}]}"
	(( indice_parola++ ))
done
}
echo "dimensione parola: ${indice_parola}"
{ #analisi carattere per carattere della parola
while (( scorrimento_parola<indice_parola )) 
do
{ #stampa
echo "--------------------------------"
echo "carattere: ${carattere[${scorrimento_parola}]}"
echo "scorrimento su parola: ${scorrimento_parola}"
echo "indice: ${indice}"
echo "allerta: ${allerta}"
echo "redirect: ${redirect}"
echo "memoria: ${memoria}"
echo "test: ${test}"
}
case ${carattere[${scorrimento_parola}]} in
1) 
{	if (( indice!=0 ))
		then
			if (( allerta==0 )) && (( redirect==0 )) && (( memoria==0 )) #nessun evento pregresso
				then
					#il carattere si attacca alla parola precedente e setta parametri
					(( indice-- ))
					parsing[${indice}]=${parsing[${indice}]}${carattere[${scorrimento_parola}]}
					(( memoria++ ))
					allerta=1
					test=1
			elif (( allerta==0 )) && (( redirect==0 )) && (( memoria!=0 )) #evento pregresso in falso positivo
				then
					#il carattere si posiziona nel nuovo slot del parsing, come primo carattere
					#e setta i parametri
					parsing[${indice}]=${carattere[${scorrimento_parola}]}
					memoria=1
					allerta=1
					test=1
			elif (( allerta==1 )) && (( redirect==0 )) #evento pregresso - potenziale
				then			
					if (( test==1 )) #verifica che il carattere precedente non sia un carattere speciale
						then
							#carattere precedente è standard: il carattere attuale si attacca alla
							#parola precedente e setta i parametri
							(( indice-- ))
							parsing[${indice}]=${parsing[${indice}]}${carattere[${scorrimento_parola}]}
							(( memoria++ ))
							allerta=1
						else
							#carattere precendete non è standard: il carattere attuale si posizione nel
							#nuovo slot del parsing e setta i paramtri
							parsing[${indice}]=${carattere[${scorrimento_parola}]}
							speciali[${indice}]=0						
					fi
					test=1
			elif (( redirect==1 )) && (( memoria==3 )) # evento pregresso - redirezione
				then
					#il carattere si attacca alla parola precedente che è speciale e setta i parametri
					(( indice-- ))
					parsing[${indice}]=${parsing[${indice}]}${carattere[${scorrimento_parola}]}
					(( memoria++ ))
					allerta=0
					redirect=0
					test=0
					if [[ ${parsing[$indice]} == "2<&1" ]] #mappa la parola speciale
						then
							mappatura[${indice}]=12
					elif [[ ${parsing[$indice]} == "2>&1" ]]
						then
							mappatura[${indice}]=13
					fi
			else #evento errore
				echo "errore: carattere speciale fuori posizione"
				fuga=0
				break
			fi
		else #il carattere è in prima posizione nel primo slot del parsing: non è un carattere speciale
			parsing[${indice}]=${carattere[${scorrimento_parola}]}
			speciali[${indice}]=0
			test=1
	fi
	(( indice++ ))
} ;;
"&")
{	if (( indice!=0 ))
		then
			if (( allerta==0 )) && (( redirect==0 )) && (( memoria==0 )) #nessun evento pregresso
				then
					#il carattere si attacca alla parola precedente e setta i parametri
					(( indice-- ))
					parsing[${indice}]=${parsing[${indice}]}${carattere[${scorrimento_parola}]}
					speciali[${indice}]=1
					(( memoria++ ))
					allerta=1
					test=1
			elif (( allerta==0 )) && (( redirect==0 )) && (( memoria!=0 )) #evento pregresso in falso positivo
				then
					#il carattere si posizione nel nuovo slot del parsing e setta i parametri
					parsing[${indice}]=${carattere[${scorrimento_parola}]}
					speciali[${indice}]=1
					memoria=1
					allerta=1
					test=1
			elif (( allerta==1 )) && (( redirect==0 )) #evento pregresso - potenziale
				then
					#verifica che la parola precedente per settare i paramtri
					(( indice-- ))
					if [[ ${parsing[${indice}]} = ">" ]] || [[ ${parsing[${indice}]} = "<" ]]
						then
							allerta=0
							redirect=0
							test=0
					else # altrimenti si mette fuga		
						echo "non riconosco il carattere"
						allerta=1
						test=1
					fi		
					#mappatura del tipo di parola speciale
					if [[ "${parsing[${indice}]}" = ">" ]]
						then
							mappatura[${indice}]=6
					elif [[ "${parsing[${indice}]}" = "<" ]]
						then
							mappatura[${indice}]=7
					fi
					#il carattere si attacca alla parola precedente
					parsing[${indice}]=${parsing[${indice}]}${carattere[${scorrimento_parola}]}
					(( memoria++ ))
			elif (( redirect==1 )) #evento pregresso - redirezione
				then
					#confermata parola speciale: il carattere si attacca alla parola precedente e setta
					#i parametri
					(( indice-- ))
					parsing[${indice}]=${parsing[${indice}]}${carattere[${scorrimento_parola}]}
					(( memoria++ ))
					test=0
			else #evento errore
				echo "errore: carattere speciale fuori posizione"
				fuga=0
				break
			fi
		else #il carattere è in prima posizione nel primo slot del parsing: non è un carattere speciale
			parsing[${indice}]=${carattere[${scorrimento_parola}]}
			speciali[${indice}]=0			
			test=1
	fi
	(( indice++ ))
} ;;
2)
{	if (( indice!=0 ))
		then
			if (( allerta==0 )) && (( redirect==0 )) && (( memoria==0 )) #nessun evento pregresso
				then
					#il carattere si attacca alla parola precedente e setta i parametri
					(( indice-- ))
					parsing[${indice}]=${parsing[${indice}]}${carattere[${scorrimento_parola}]}
					speciali[${indice}]=1
					(( memoria++ ))
			elif (( allerta==0 )) && (( redirect==0 )) && (( memoria!=0 ))#evento pregresso in falso positivo
				then
					#il carattere si posiziona nel nuovo slot del parsing e setta i parametri
					parsing[${indice}]=${carattere[${scorrimento_parola}]}
					speciali[${indice}]=1
					memoria=1
			elif (( allerta==1 )) && (( redirect==0 )) #evento pregresso - potenziale
				then
					if (( test==1 )) #verifica che il carattere precedente sia standard
						then
							#il carattere attuale si attacca alla parola precedente
							(( indice-- ))
							parsing[${indice}]=${parsing[${indice}]}${carattere[${scorrimento_parola}]}
							speciali[${indice}]=1
							(( memoria++ ))						
						else
							#parola precedente una falsa parola speciale, correzione e posizionamento
							#del carattere attuale nel nuovo slot del parsing
							(( indice-- ))
							speciali{${indice}}=0
							(( indice++ ))
							parsing[${indice}]=${carattere[${scorrimento_parola}]}
					fi
			else #evento errore
				echo "errore: carattere speciale fuori posizione"
				fuga=0
				break
			fi
		else #il carattere è in prima posizione nel primo slot del parsing: non è un carattere speciale
			parsing[${indice}]=${carattere[${scorrimento_parola}]}
			speciali[${indice}]=1						
			(( memoria++ ))
	fi
	(( indice++ ))
	allerta=1
	test=1
} ;;
"-")
{	if (( indice!=0 ))
		then
			if (( allerta==0 )) && (( redirect==0 )) && (( memoria==0 )) #nessun evento pregresso
				then
					#inserimento del carattere di spazio fra nella parola
					(( indice-- ))	
					parsing[${indice}]=${parsing[${indice}]}" "${carattere[${scorrimento_parola}]}
					allerta=1
				else #evento errore
					echo "errore: carattere speciale fuori posizione"
					fuga=0
					break
			fi
		else #evento errore
			echo "errore: carattere speciale fuori posizione"
			fuga=0		
			break
	fi
	(( indice++ ))
	test=1
}	;;
"<")
{	if (( indice!=0 ))
		then
			if (( allerta==0 )) && (( redirect==0 )) && (( memoria==0 )) #nessun evento pregresso
				then
					#il carattere si posiziona nel nuovo slot del parsing e setta i parametri
					parsing[${indice}]=${carattere[${scorrimento_parola}]}
					speciali[${indice}]=1
					mappatura[${indice}]=3
					allerta=1
			elif (( allerta==0 )) && (( redirect==0 )) && (( memoria!=0 )) #evento pregresso in falso positivo
				then
					#carattere si posiziona nel nuovo slot del parsing e setta i paramtri
					parsing[${indice}]=${carattere[${scorrimento_parola}]}
					speciali[${indice}]=1
					mappatura[${indice}]=3
					memoria=0
					allerta=1
			elif (( allerta==1 )) && (( redirect==0 )) #evento pregresso - potenziale
				then
					#verifica del carattere precedente e assunzione di politiche di gestione
					(( scorrimento_parola-- ))
					if [[ ${carattere[${scorrimento_parola}]} = "2" ]]
						then redirect=1
					elif [[ ${carattere[${scorrimento_parola}]} = ">" ]]
						then 
							echo "errore: carattere speciale fuori posizione"
							fuga=0
							break
					else allerta=0
					fi
					#determinazione lunghezza parola precedente scalata di 1
					(( indice-- ))
					(( tmp=${#parsing[${indice}]}-1 ))
					if (( tmp==0 )) #verifica se la lunghezza è nulla
						then
							# carattere attuale si attacca alla parola precedente e setta i parametri
							(( scorrimento_parola++ ))
							parsing[${indice}]=${parsing[${indice}]}${carattere[${scorrimento_parola}]}
							mappatura[${indice}]=5
						else
							#estrazione dalla parola precedente di caratteri speciali, poi inseriti nel
							#nuovo slot del parsing e risettaggio parametri
							parsing[${indice}]=${parsing[${indice}]:0:tmp}
							speciali[${indice}]=0
							(( indice++ ))
							if (( scorrimento_parola<0 ))
								then  scorrimento_parola=0
							fi
							parsing[${indice}]=${carattere[${scorrimento_parola}]}	
							(( scorrimento_parola++ ))
							#identificazione del carattere speciale precedente
							if [[ ${parsing[${indice}]} == "1" ]]
								then
									mappatura[${indice}]=8
							elif [[ ${parsing[${indice}]} == "&" ]]
								then
									mappatura[${indice}]=10
							fi
							#concatenamento carattere attuale alla parola precedente
							parsing[${indice}]=${parsing[${indice}]}${carattere[${scorrimento_parola}]}
							speciali[${indice}]=1
					fi		
			else #evento errore
				echo "errore: carattere speciale fuori posizione"
				fuga=0
				break
			fi
		else #evento errore
			echo "errore: carattere speciale fuori posizione"
			fuga=0
			break
	fi
	(( indice++ ))
	(( memoria++ ))
	(( n_speciali[2]++ ))	
	test=0
} ;;
">")
{	if (( indice!=0 ))
		then
			if (( allerta==0 )) && (( redirect==0 )) && (( memoria==0 )) #nessun evento pregresso
				then
					#carattere posizionato nel nuovo slot del parsing e setta i parametri
					parsing[${indice}]=${carattere[${scorrimento_parola}]}
					speciali[${indice}]=1
					mappatura[${indice}]=2
					allerta=1
			elif (( allerta==0 )) && (( redirect==0 )) && (( memoria!=0 )) #evento pregresso in falso positivo
				then
					#carattere inserito nel nuovo slot del parsing e settaggio parametri
					parsing[${indice}]=${carattere[${scorrimento_parola}]}
					speciali[${indice}]=1
					mappatura[${indice}]=2
					memoria=0
					allerta=1
			elif (( allerta==1 )) && (( redirect==0 )) #evento pregresso - potenziale
				then
					#verifica del carattere precedente e assunzione di politiche di gestione
					(( scorrimento_parola-- ))
					if [[ ${carattere[${scorrimento_parola}]} = "2" ]]
						then redirect=1
					elif [[ ${carattere[${scorrimento_parola}]} = "<" ]]
						then 
							echo "errore: carattere speciale fuori posizione"
							fuga=0
							break
					else allerta=0
					fi
					#determinazione lunghezza parola precedente scalata di 1
					(( indice-- ))
					(( tmp=${#parsing[${indice}]}-1 ))
					if ((tmp==0)) #verifica se la lunghezza è nulla
						then
							# carattere attuale si attacca alla parola precedente e setta i parametri
							(( scorrimento_parola++ ))
							parsing[${indice}]=${parsing[${indice}]}${carattere[${scorrimento_parola}]}
							mappatura[${indice}]=4
						else
							#estrazione dalla parola precedente di caratteri speciali, poi inseriti nel
							#nuovo slot del parsing e risettaggio parametri
							parsing[${indice}]=${parsing[${indice}]:0:tmp}
							speciali[${indice}]=0
							(( indice++ ))
							if (( scorrimento_parola<0 ))
								then  scorrimento_parola=0
							fi
							parsing[${indice}]=${carattere[${scorrimento_parola}]}	
							(( scorrimento_parola++ ))
							#identificazione del carattere speciale precedente
							if [[ ${parsing[${indice}]} == "1" ]]
								then
									mappatura[${indice}]=9
							elif [[ ${parsing[${indice}]} == "&" ]]
								then
									mappatura[${indice}]=11
							fi
							#concatenamento carattere attuale alla parola precedente
							parsing[${indice}]=${parsing[${indice}]}${carattere[${scorrimento_parola}]}
							speciali[${indice}]=1
					fi		
			else #evento errore
				echo "errore: carattere speciale fuori posizione"
				fuga=0
				break
			fi
		else #evento errore
			echo "errore: carattere speciale fuori posizione"
			fuga=0
			break
	fi
	(( indice++ ))
	(( memoria++ ))
	(( n_speciali[2]++ ))	
	test=0
} ;;
"|")
{	if (( indice!=0 )) && (( allerta==0 )) && (( memoria==0 )) #nessun evento pregresso
		then
			#carattere si posiziona nel nuovo slot del parsing e setta i parametri
			parsing[${indice}]=${carattere[${scorrimento_parola}]}
			speciali[${indice}]=1
			mappatura[${indice}]=1
			(( n_speciali[0]++ ))		
			(( indice++ ))
			(( memoria++ ))
			test=0
		else
			#verifica se il carattere precedente è di tipo standard
			if (( test==1 ))
				then
					#carattere si posiziona nel nuovo slot del parsing e setta i paramentri
					parsing[${indice}]=${carattere[${scorrimento_parola}]}
					speciali[${indice}]=1
					mappatura[${indice}]=1
					(( n_speciali[0]++ ))
					(( indice++ ))
					(( memoria++ ))
					test=0
				else #evento errore
					echo "errore: carattere speciale fuori posizione"
					fuga=0
					break
			fi
	fi	
} ;;
*)
{	if ((( indice!=0 )) && (( redirect==0 )) && (( memoria!=0 )) && (( test==1 ))) || ((( indice!=0 )) && (( test==1 )))
		then
			
			(( scorrimento_parola-- ))
			(( indice-- ))
			(( tmp=${#parsing[${indice}]}-1 )) #calcolo della lunghezza della parola, scalata di 1
			#echo "tmp = ${tmp}"
			if (( tmp==0 )) || (( redirect==0 )) || (( memoria==0 )) #nessun evento pregresso
				then
					if (( test==1 )) #verifica che il carattere precedente sia di tipo standard
						then speciali[${indice}]=0
					fi
					#carattere attuale si attacca alla parola precedente e setta i paramentri
					(( scorrimento_parola++ ))
					parsing[${indice}]=${parsing[${indice}]}${carattere[${scorrimento_parola}]}
					mappatura[${indice}]=0
				else	
					#estrazione di caratteri dalla parola precedente in nuovo slot del parsing echo
					#concatenamento del carattere attuale, con relatico settaggio dei parametri
					parsing[${indice}]=${parsing[${indice}]:0:tmp}
					(( scorrimento_parola++ ))
					parsing[${indice}]=${parsing[${indice}]}${carattere[${scorrimento_parola}]}
					speciali[${indice}]=0				
					mappatura[${indice}]=0
			fi
		elif (( allerta==1 )) && (( redirect==1 ))
			then #evento errore
				echo "errore: carattere speciale fuori posizione"
				fuga=0
				break
		else
			#carattere si posiziona nel nuovo slot del parsing
			parsing[${indice}]=${carattere[${scorrimento_parola}]}
			speciali[${indice}]=0
			mappatura[${indice}]=0
	fi
	(( indice++ ))
	allerta=0
	redirect=0
	memoria=0
	test=1
} ;;
esac
(( scorrimento_parola++ ))
done
(( indice-- ))
echo "mappatura[${indice}] = ${mappatura[${indice}]}"
echo "parola: ${parsing[${indice}]}"
indice_parola=0
scorrimento_parola=0
parsing[${indice}]=${parsing[${indice}]}" " #inserimento del carattere di spaziatura nella parola
(( indice++ ))
}
done
}
{ #controllo ultima posizione del parsing
if (( test==0 )) #verifichiamo che il carattere in ultima posiziona sia speciale
	then
		#solo due parole speciali sono ammesse in ultima posizione: comandi di redirezione
		if [[ ${parsing[${indice}]} != "2&>1" ]] || [[ ${parsing[${indice}]} != "2&<1" ]]
			then echo "errore(ultima posizione): carattere speciale fuori posizione"
		fi
fi
}
{ #stampa informazioni generali
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
echo "caratteri speciali di tipo \"|\" sono: ${n_speciali[0]}"
echo "caratteri speciali di tipo \"<\" sono: ${n_speciali[1]}"
echo "caratteri speciali di tipo \">\" sono: ${n_speciali[2]}"
echo "dimensione dell'array di parsing: ${indice}"
echo "---------------"
while (( n<${indice} ))
do
	echo "parsing[${n}] = ${parsing[${n}]}"
	(( n++ ))
done
n=0
echo "---------------"
while (( n<${indice} ))
do
	echo "speciali[${n}] = ${speciali[${n}]}"
	(( n++ ))
done
n=0
echo "---------------"
while (( n<${indice} ))
do
	echo "mappatura[${n}] = ${mappatura[${n}]}"
	(( n++ ))
done
n=0
echo "---------------"
echo "riga ricevuta: ${riga}"
echo "parsing eseguito: ${parsing[*]}"
echo "posizione caratteri speciali: ${speciali[*]}"
echo "mappatura caratteri speciali: ${mappatura[*]}"
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
}
{ #esecuzione della shell bash
while  (( n<${indice} ))
do
	#echo "n = ${n}"	
	#echo "tmp1 = ${tmp1}"
	#echo "tmp2 = ${tmp2}"
	echo "mappatura[${n}] = ${mappatura[${n}]}"
	echo "parsing[${n}] = ${parsing[${n}]}"	
	case ${mappatura[${n}]} in 
	1) 
		echo "conteggio_pipe = ${conteggio_pipe}"
		if (( conteggio_pipe==0 ))
			then
				parsing[${n}]=$( ${parsing[${tmp1}]} | ${parsing[${tmp2}]} )
				conteggio_pipe=1
			else
				parsing[${n}]=$(echo ${parsing[${tmp1}]} | ${parsing[${tmp2}]} )
		fi
		echo ${parsing[${n}]}
		(( n=$n+2 ))
		(( tmp1=${n}-2 ))
		(( tmp2=${n}+1 ))	;;
	2) 
		$(echo ${parsing[${tmp1}]} > ${parsing[${tmp2}]} )
		cat ${parsing[${tmp2}]}
		(( n=$n+2 ))
		(( tmp1=${n}-1 ))
		(( tmp2=${n}+1 ))	;;
	3) 
		$(echo ${parsing[${tmp1}]} < ${parsing[${tmp2}]} )
		cat ${parsing[${tmp2}]}
		(( n=$n+2 ))
		(( tmp1=${n}-1 ))
		(( tmp2=${n}+1 ))	;;
	4) 
		$(echo ${parsing[${tmp1}]} >> ${parsing[${tmp2}]})
		cat ${parsing[${tmp2}]}
		(( n=$n+2 ))
		(( tmp1=${n}-1 ))
		(( tmp2=${n}+1 ))	;;
	5) 
		$(echo ${parsing[${tmp1}]} << ${parsing[${tmp2}]} )
		cat ${parsing[${tmp2}]}
		(( n=$n+2 ))
		(( tmp1=${n}-1 ))
		(( tmp2=${n}+1 ))	;;
	6) 
		$(echo ${parsing[${tmp1}]} >& ${parsing[${tmp2}]})
		cat ${parsing[${tmp2}]}
		(( n=$n+2 ))
		(( tmp1=${n}-1 ))
		(( tmp2=${n}+1 ))	;;
	7) 
		$(echo ${parsing[${tmp1}]} <& ${parsing[${tmp2}]} )
		cat ${parsing[${tmp2}]}
		(( n=$n+2 ))
		(( tmp1=${n}-1 ))
		(( tmp2=${n}+1 ))	;;
	8) 
		$(echo ${parsing[${tmp1}]} 1< ${parsing[${tmp2}]})
		cat ${parsing[${tmp2}]}
		(( n=$n+2 ))
		(( tmp1=${n}-1 ))
		(( tmp2=${n}+1 ))	;;
	9) 
		$(echo ${parsing[${tmp1}]} 1> ${parsing[${tmp2}]} )
		cat ${parsing[${tmp2}]}
		(( n=$n+2 ))
		(( tmp1=${n}-1 ))
		(( tmp2=${n}+1 ))	;;
	10) 
		$(echo ${parsing[${tmp1}]} &< ${parsing[${tmp2}]})
		cat ${parsing[${tmp2}]}
		(( n=$n+2 ))
		(( tmp1=${n}-1 ))
		(( tmp2=${n}+1 ))	;;
	11) 
		$(echo ${parsing[${tmp1}]} &> ${parsing[${tmp2}]} )
		cat ${parsing[${tmp2}]}
		(( n=$n+2 ))
		(( tmp1=${n}-1 ))
		(( tmp2=${n}+1 ))	;;
	12) 
		$(echo ${parsing[${tmp1}]} 2<&1 ${parsing[${tmp2}]})
		cat ${parsing[${tmp2}]}
		(( n=$n+2 ))
		(( tmp1=${n}-1 ))
		(( tmp2=${n}+1 ))	;;
	13)
		$(echo ${parsing[${tmp1}]} 2>&1 ${parsing[${tmp2}]} )
		cat ${parsing[${tmp2}]}
		(( n=$n+2 ))
		(( tmp1=${n}-1 ))
		(( tmp2=${n}+1 ))	;;
	0) 
		(( tmp1=${n}-1 ))
		(( tmp2=${n}+1 ))
		#(( ${mappatura[${tmp2}]}==1 )) ||
		if [ "${mappatura[${tmp2}]}" ]
			then
				echo "il prossimo token è una pipe!"
			else
				parsing[${n}]=$(${parsing[${n}]})
				echo ${parsing[${n}]}
		fi
		(( n++))
		(( tmp1=${n}-1 ))
		(( tmp2=${n}+1 ))	;;
	esac
	echo "---------------"
done
}
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
done
