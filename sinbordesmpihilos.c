#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pgm.h"
#include "mpi.h"
#include <pthread.h>

#define MAX 100
#define TAG 10
#define numHilos 2

MPI_Status status;

int inicio, fin;
unsigned char** Original;
unsigned char** Salida;
int** nucleo;
unsigned char** aux;
unsigned char* vector;

struct args_struct {
	int alto;
	int largo;
	int rank;
	int size;
	int inicio1;
	int fin1;
	int hilo;
};

//Metodo usado para transformar una matriz en un vector
void fromMatrixToVector (unsigned char** matrix, unsigned char* vector, int Largo, int Alto){
	int p = 0;
	for (int i=0; i<Largo; i++){
		for (int j=0; j<Alto; j++) {
			vector[p] = matrix[i][j];
			p++;
		}
	}
}

//Metodo usado para transformar un vector a una matriz
void fromVectorToMatrix (unsigned char** matrix, unsigned char* vector, int Largo, int Alto) {
	int p = 0;
	for (int i=0; i<Largo; i++) {
		for (int j=0; j<Alto; j++) {
			matrix[i][j] = vector[p];
			p++;
		}
	}
}

void* convolucion(void *arguments) {
	struct args_struct *args = (struct args_struct*) arguments;
	int k = 0;
	int suma = 0;
	int i, j;
	int x, y;

	//Operaciones de convolucion de imagen
	for (i = 0; i < 3; i++)
	  for (j = 0; j < 3; j++)
	    k = k + nucleo[i][j];

	for (x = args->inicio1; x < args->fin1; x++){	  
	  for (y = 1; y < (args->alto-1); y++){
	    suma = 0;
	    for (i = 0; i < 3; i++){
	      for (j = 0; j < 3; j++){
	        suma = suma + Original[(x-1)+i][(y-1)+j] * nucleo[i][j];
	      }
	    }
      
      	    if(k==0)
              Salida[x][y] = suma;
            else
              Salida[x][y] = suma/k;
	  }
	}
}

/* * * * *          * * * * *          * * * * *          * * * * */

int main(int argc, char *argv[]){
	int rank, size;
	int Largo, Alto;
	int suma = 0;
	int inicioAux, finAux;
	int i, j, y, x;
	MPI_Status status;

	MPI_Init (&argc, &argv);
	MPI_Comm_size (MPI_COMM_WORLD, &size); 
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);


	/*Finalizamos la ejecucion si ejecutamos el programa en un procesador, ya que	*/
	/*el procesador 0 solo realiza las operaciones necesarias para juntar el	*/
	/*resultados de los diferentes procesadores					*/
	if(size == 1) {
		if(rank == 0) {
			printf("Por favor. Ejecuta el programa en mas de un procesador.\n");
		}
		MPI_Finalize();
		return 0;
	}

	//Declaramos los hilos
  	pthread_t tid[numHilos];

	//Reservamos memoria para las variables a usar
	Original = pgmread("lena_original.pgm", &Largo, &Alto);
  	Salida   = (unsigned char**)GetMem2D(Largo, Alto, sizeof(unsigned char));
	aux = (unsigned char**)GetMem2D(Largo, Alto, sizeof(unsigned char));
	vector = (unsigned char *)malloc(sizeof(unsigned char)*Largo*Alto);

	//Calculamos el nucleo
	nucleo = (int**) GetMem2D(3, 3, sizeof(int));
	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
			nucleo[i][j] = -1;
		nucleo[1][1] = 1;

	//Operaciones para el procesador 0 (rank 0)
	if(rank == 0){
		for(int i=1; i<size; i++){
			MPI_Recv (vector, sizeof(unsigned char)*Largo*Alto, MPI_UNSIGNED_CHAR, i, TAG, MPI_COMM_WORLD, &status);
        	fromVectorToMatrix (aux, vector, Largo, Alto);
			MPI_Recv (&inicioAux, sizeof(int), MPI_INT, i, TAG, MPI_COMM_WORLD, &status);
      		MPI_Recv (&finAux, sizeof(int), MPI_INT, i, TAG, MPI_COMM_WORLD, &status);
        		
			for (x = inicioAux; x < finAux; x++){
          			for (y = 1; y < Alto-1; y++){
            				Salida[x][y] = aux[x][y];		
         	 		}
        		}
		}

		pgmwrite(Salida, "lena_procesada.pgm", Largo, Alto);

	}else{
		//Calculamos por un lado el INICIO y el FIN para cada procesador
		int division = Largo/(size-1);
		int resto = Largo%(size-1);

		inicio = (rank-1) * division + ((rank-1) < resto ? (rank-1) : resto);
		fin = inicio + division + ((rank-1) < resto);

		if((rank == 1) && (inicio == 0)) {
			inicio = 1;
		}
	
		if(fin == Largo)
		fin = fin-1;
		/**************************************************************/
		//Ahora calculamos el inicio y el fin que pasaremos a cada hilo
		struct args_struct args[numHilos];

		int PTinicio = 0;
		int PTfin = 0;

  		for(int t = 0; t < numHilos; t++) {
			int num = (fin-inicio)/2;
			if(t == 0) {
				PTinicio = inicio;
				PTfin = (inicio+num)-1;
			}else{
				PTinicio = PTfin;
				PTfin = fin;
			}
    		args[t].inicio1 = PTinicio;
    		args[t].fin1 = PTfin;
    		args[t].alto = Alto;
    		args[t].largo = Largo;
			args[t].rank = rank;
			args[t].size = size;
    		args[t].hilo = t;
		}

  		int err;
		for (int i = 0; i<numHilos; i++) {
			err = pthread_create(&tid[i], NULL, convolucion, (void*)&args[i]);
        		if (err != 0)
        			printf("\nerror al crear el hilo[%d]", i);
     		}

  		for (int i = 0; i<numHilos; i++) {
			pthread_join(tid[i], NULL);
  		}
		//Operaciones para los procesadores distintos de 0 (rank != 0)
		fromMatrixToVector(Salida, vector, Largo, Alto);		
      		MPI_Send (vector, sizeof(unsigned char)*Largo*Alto, MPI_UNSIGNED_CHAR, 0, TAG, MPI_COMM_WORLD);
		MPI_Send (&inicio, sizeof(int), MPI_INT, 0, TAG, MPI_COMM_WORLD);
	    	MPI_Send (&fin, sizeof(int), MPI_INT, 0, TAG, MPI_COMM_WORLD);
	}

	Free2D((void**) nucleo, 3);
	Free2D((void**) Original, Largo);
	Free2D((void**) Salida, Largo);
	Free2D((void**) aux, Largo);
	free(vector);

	MPI_Finalize();	
	return 0;
}
