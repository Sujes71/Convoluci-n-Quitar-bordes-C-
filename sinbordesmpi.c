#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pgm.h"
#include "mpi.h"
#define TAG 5
#define MAX 100



int inicio, fin;



//Metodo usado para transformar una matriz en un vector
void fromMatrixToVector (unsigned char** matriz, unsigned char* vector, int Largo, int Alto){
	int p = 0;
	for (int i=0; i<Largo; i++){
		for (int j=0; j<Alto; j++) {
			vector[p] = matriz[i][j];
			p++;
		}
	}
}

//Metodo usado para transformar un vector a una matriz
void fromVectorToMatrix (unsigned char** matriz, unsigned char* vector, int Largo, int Alto) {
	int p = 0;
	for (int i=0; i<Largo; i++) {
		for (int j=0; j<Alto; j++) {
			matriz[i][j] = vector[p];
			p++;
		}
	}
}

void convolucion(int alto, int largo, int rank, int size, unsigned char** Original, unsigned char** Salida, int** nucleo){
	int k = 0;
	int suma = 0;
	int i, j;
	int x, y;

	//Calculamos el INICIO y el FIN
	int division = largo/(size-1);
	int resto = largo%(size-1);

	inicio = (rank-1) * division + ((rank-1) < resto ? (rank-1) : resto);
	fin = inicio + division + ((rank-1) < resto);

	if((rank == 1) && (inicio == 0)) {
		inicio = 1;
	}
	
	if(fin == largo)
		fin = fin-1;

	/******************************/


	//Operaciones de convolucion de imagen
	for (i = 0; i < 3; i++)
	  for (j = 0; j < 3; j++)
	    k = k + nucleo[i][j];

	for (x = inicio; x <fin; x++){	  
	  for (y = 1; y <alto-1; y++){
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
			printf("Debe utilizarse más de 1 procesador.\n");
		}
		MPI_Finalize();
		return 0;
	}
	
		
	
	
	
	//Reservamos memoria para las variables a usar
	unsigned char** Original = pgmread("lena_original.pgm", &Largo, &Alto);
  	unsigned char** Salida = (unsigned char**)GetMem2D(Largo, Alto, sizeof(unsigned char));
	unsigned char** aux = (unsigned char**)GetMem2D(Largo, Alto, sizeof(unsigned char));
	unsigned char* vector = (unsigned char *)malloc(sizeof(unsigned char)*Largo*Alto);

	//Calculamos el nucleo
	int** nucleo = (int**) GetMem2D(3, 3, sizeof(int));
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
		//Operaciones para los procesadores distintos de 0 (rank != 0)
		convolucion (Alto, Largo, rank, size, Original, Salida, nucleo);
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
