/*Standard Library*/
#include <stdio.h>
#include <string.h>
/*File Header*/
#include "cadenas.h"

char letraMayuscula(char valor)
{
  const int delta = 'a' - 'A';
  /* Devuelve 32 en decimal
  ** que es la diferencia entre 'a'
  ** que es en ASCII decimal 97
  ** y 'A' que es en ASCII decimal 65
  */
  if (valor <= 'z' && valor >= 'a')
  {
    valor -= delta;
  }
  return valor;
}

char letraMinuscula(char valor)
{
  const int delta = 'a' - 'A';
  /* Devuelve 32 en decimal
  ** que es la diferencia entre 'a'
  ** que es en ASCII decimal 97
  ** y 'A' que es en ASCII decimal 65
  */
  if (valor >= 'A' && valor <= 'Z')
  {
    valor += delta;
  }
  return valor;
}

void cadenaMayuscula(char* cadena, char* cadenaMayusc)
{
  unsigned long i;
  for (i = 0; i < strlen(cadena); i++)
  {
    cadenaMayusc[i] = letraMayuscula(cadena[i]);
  }
}

void cadenaMinuscula(char* cadena, char* cadenaMinusc)
{
  unsigned long i;
  for (i = 0; i < strlen(cadena); i++)
  {
    cadenaMinusc[i] = letraMinuscula(cadena[i]);
  }
}

char* unirCadenas(char* destino, char* origen)
{
  return strcat(destino, origen);
}

/*Ejemplo: unirCadenasConDelimitador(destino, origen, DELIMITADOR_BASE)*/
char* unirCadenasConDelimitador(char* destino, char* origen, char* delimitador)
{
  unirCadenas(origen, delimitador);
  return unirCadenas(destino, origen);
}

/* Retorna 0 cuando no puede seguir separando la cadena
** se suele utilizar con un while(token != null)
*/
char* separarCadenaConDelimitador(char* cadena, char* delimitador)
{
  if (cadena != 0)
  {
    return strtok(cadena, delimitador);
  }
  else
  {
    return strtok(0, delimitador);
  }
}

/* Ejemplo: podemos utilizar esta función para
** crear una estructura array=(char**)malloc(sizeof(char*)*count)
** y luego separar el string y asignarlo a cada parte del array
** array[i]=(char*)malloc(sizeof(char)*strlen(token))
** sprintf(array[i],"%s\n", token)
*/
int contarSeparacionesCadena(char* cadena, char* delimitador)
{
  int contador;
  contador = 0;
  while (separarCadenaConDelimitador(cadena, delimitador) != 0)
  {
    contador++;
  }
  return contador;
}

int convertirNumeroEnCadena(char* cadena, int numero)
{
  return sprintf(cadena, "%d", numero);;
}
