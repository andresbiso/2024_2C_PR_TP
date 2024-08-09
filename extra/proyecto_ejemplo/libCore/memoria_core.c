/*Standard Library*/
#include <stdio.h>
#include <string.h>
/*Headers Library*/
#include "libCommon/pantalla.h"
/*File Header*/
#include "memoria_core.h"

void configurarMemoria(dato_memoria* memoria, int cantidad)
{
  int i;
  for(i = 0; i < cantidad; i++)
  {
    memoria[i].intentos = 0;
    memoria[i].goles = 0;
    memoria[i].finPartido = 0;
  }
}

int leerIntentos(dato_memoria* memoria, int posicion)
{
  return memoria[posicion].intentos;
}

void escribirIntentos(dato_memoria* memoria, int posicion, int valor)
{
  memoria[posicion].intentos = valor;
}

int leerGoles(dato_memoria* memoria, int posicion)
{
  return memoria[posicion].goles;
}

void escribirGoles(dato_memoria* memoria, int posicion, int valor)
{
  memoria[posicion].goles = valor;
}

int leerFinPartido(dato_memoria* memoria, int posicion)
{
  return memoria[posicion].finPartido;
}

void escribirFinPartido(dato_memoria* memoria, int posicion, int valor)
{
  memoria[posicion].finPartido = valor;
}
