#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pgm.h"
#include <pthread.h>

unsigned char** Original;
unsigned char** Salida;
int** nucleo;
int numHilos;

struct trabajo {
	int largo;
	int alto;
	int ini;
	int fin;
	int hilo;
};

void* convolucion(void *arguments) {
	struct trabajo *obj = (struct trabajo*) arguments;
	int x, y;
	int suma;
	int k = 0;
	int i, j;
	for (i = 0; i < 3; i++)
 		for (j = 0; j < 3; j++)
      			k = k + nucleo[i][j];

  	if(obj->ini == 0)
    		obj->ini = 1;
  	if(obj->fin == obj->largo)
		obj->fin = obj->fin-1;

  	for (x = obj->ini; x < obj->fin; x++){
		for (y = 1; y < obj->alto-1; y++){
      			suma = 0;
      			for (i = 0; i < 3; i++){
        			for (j = 0; j < 3; j++){
        				suma = suma + Original[(x-1)+i][(y-1)+j] * nucleo[i][j];
        			}
      			}
      			//Escribimos la foto en salida
      			if(k==0) Salida[x][y] = suma;
      			else     Salida[x][y] = suma/k;
    		}
  	}
}

int main(int argc, char *argv[]){
	int Largo, Alto;
	numHilos = atoi(argv[1]);
	int i, j;
  
	Original = pgmread("lena_original.pgm", &Largo, &Alto);
	Salida   = (unsigned char**)GetMem2D(Largo, Alto, sizeof(unsigned char));

	nucleo = (int**) GetMem2D(3, 3, sizeof(int));
  	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
      			nucleo[i][j] = -1;
  	nucleo[1][1] = 1;

  	//Declaramos los hilos
  	pthread_t hilo[numHilos];

  	//Division largo
  	int trabajo = Largo/numHilos;
  	int resto = Largo%numHilos;
  	int inicio = 0;
  	int fin = 0;
  	//--------

  	struct trabajo obj[numHilos];

  	for(int i = 0; i < numHilos; i++) {
		inicio = i * trabajo + (i < resto ? i : resto);
		fin = inicio + trabajo + (i < resto);
    	obj[i].ini = inicio;
    	obj[i].fin = fin;
    	obj[i].alto = Alto;
    	obj[i].largo = Largo;
    	obj[i].hilo = i;
	}

  	int error;
	for (int i = 0; i<numHilos; i++) {
		error = pthread_create(&hilo[i], NULL, convolucion, (void*)&obj[i]);
        	if (error != 0)
        		printf("\nerror al crear el hilo[%d]", i);
     	}

  	for (int i = 0; i<numHilos; i++) {
		pthread_join(hilo[i], NULL);
  	}

  	pgmwrite(Salida, "lena_procesada.pgm", Largo, Alto);

  	Free2D((void**) nucleo, 3);

  	Free2D((void**) Original, Largo);
  	Free2D((void**) Salida, Largo);
}
