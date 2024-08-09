#ifndef _MEMORIA_CORE_H
#define _MEMORIA_CORE_H
/*Headers Library*/
#include "defines.h"
struct tipo_dato_memoria
{
  int intentos;
  int goles;
  int finPartido;
};
typedef struct tipo_dato_memoria dato_memoria;
void configurarMemoria(dato_memoria*, int);
int leerIntentos(dato_memoria*, int);
void escribirIntentos(dato_memoria*, int, int);
int leerGoles(dato_memoria*, int);
void escribirGoles(dato_memoria*, int, int);
int leerFinPartido(dato_memoria*, int);
void escribirFinPartido(dato_memoria*, int, int);
#endif
