/*
 * esercizio-C-2020-05-19-procs.c
 *
 *  Created on: May 19, 2020
 *      Author: marco
 */

/***********TESTO ESERCIZIO***********
N = 10

un processo padre crea N processi figli

shared variables: countdown, process_counter[N], shutdown

usare mutex per regolare l'accesso concorrente a countdown

dopo avere avviato i processi figli, il processo padre dorme 1 secondo e poi imposta il valore di countdown al valore 100000.

quando countdown == 0, il processo padre imposta shutdown a 1.

aspetta che terminino tutti i processi figli e poi stampa su stdout process_counter[].

i processi figli "monitorano" continuamente countdown:

    processo i-mo: se countdown > 0, allora decrementa countdown ed incrementa process_counter[i]

    se shutdown != 0, processo i-mo termina

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>

#define CHECK_ERR(a,msg) {if ((a) == -1) { perror((msg)); exit(EXIT_FAILURE); } }
#define N 10

sem_t * mutex;
sem_t * sem;

int * countdown;
int * process_counter;
int * shutdown;

int main(int argc, char * argv[]) {
	int s;

	mutex = malloc(sizeof(sem_t));
	sem = malloc(sizeof(sem_t));

	countdown = mmap(NULL, // NULL: è il kernel a scegliere l'indirizzo
			sizeof(int), // dimensione della memory map
			PROT_READ | PROT_WRITE, // memory map leggibile e scrivibile
			MAP_SHARED | MAP_ANONYMOUS, // memory map condivisibile con altri processi e senza file di appoggio
			-1,
			0);
	*countdown = 0;

	shutdown = mmap(NULL, // NULL: è il kernel a scegliere l'indirizzo
			sizeof(int), // dimensione della memory map
			PROT_READ | PROT_WRITE, // memory map leggibile e scrivibile
			MAP_SHARED | MAP_ANONYMOUS, // memory map condivisibile con altri processi e senza file di appoggio
			-1,
			0);
	*shutdown = 0;

	process_counter = mmap(NULL, // NULL: è il kernel a scegliere l'indirizzo
			N*sizeof(int), // dimensione della memory map
			PROT_READ | PROT_WRITE, // memory map leggibile e scrivibile
			MAP_SHARED | MAP_ANONYMOUS, // memory map condivisibile con altri processi e senza file di appoggio
			-1,
			0);
	for(int i = 0; i < N; i++)
		process_counter[i] = 0;

	mutex = mmap(NULL, // NULL: è il kernel a scegliere l'indirizzo
			sizeof(sem_t), // dimensione della memory map
			PROT_READ | PROT_WRITE, // memory map leggibile e scrivibile
			MAP_SHARED | MAP_ANONYMOUS, // memory map condivisibile con altri processi e senza file di appoggio
			-1,
			0);
	s = sem_init(mutex,
					1, // 1 => il semaforo è condiviso tra processi,
					   // 0 => il semaforo è condiviso tra threads del processo
					1 // valore iniziale del semaforo (se mettiamo 0 che succede?)
				  );

	CHECK_ERR(s,"sem_init")

	sem = mmap(NULL, // NULL: è il kernel a scegliere l'indirizzo
				sizeof(sem_t), // dimensione della memory map
				PROT_READ | PROT_WRITE, // memory map leggibile e scrivibile
				MAP_SHARED | MAP_ANONYMOUS, // memory map condivisibile con altri processi e senza file di appoggio
				-1,
				0);
	s = sem_init(sem,
					1, // 1 => il semaforo è condiviso tra processi,
					   // 0 => il semaforo è condiviso tra threads del processo
					0 // valore iniziale del semaforo (se mettiamo 0 che succede?)
				  );

	CHECK_ERR(s,"sem_init")

	for(int i = 0; i < N; i++){
		int res = fork();
		if(res == -1){
			printf("errore fork()!\n");
			exit(EXIT_FAILURE);
		}
		if(res == 0){
			if (sem_wait(sem) == -1) {
				perror("sem_wait");
				exit(EXIT_FAILURE);
			}

			while(1){
				if (sem_wait(mutex) == -1) {
					perror("sem_wait");
					exit(EXIT_FAILURE);
				}
				//printf("\ncountdown = %d", *countdown);
				(*countdown)--;

				if (sem_post(mutex) == -1) {
					perror("sem_post");
					exit(EXIT_FAILURE);
				}

				process_counter[i]++;

				if(*shutdown != 0)
					break;
			}
			exit(EXIT_SUCCESS);
		}
	}

	sleep(1);
	if (sem_wait(mutex) == -1) {
		perror("sem_wait");
		exit(EXIT_FAILURE);
	}

	*countdown = 100000;

	if (sem_post(mutex) == -1) {
		perror("sem_post");
		exit(EXIT_FAILURE);
	}

	for(int i = 0; i < N; i++){
		if (sem_post(sem) == -1) {
			perror("sem_post");
			exit(EXIT_FAILURE);
		}
	}

	while(1){

		if(*countdown == 0){
			*shutdown = 1;
			break;
		}
	}

	for(int i = 0; i < N; i++){
		if (wait(NULL) == -1) {
			perror("wait error");
		}
	}
	for(int i = 0; i < N; i++){
		printf("process_counter[%d] = %d\n", i, process_counter[i]);
	}

	s = sem_destroy(mutex);
	CHECK_ERR(s,"sem_destroy")

	printf("bye\n");

	return 0;
}
