/*Standard Library*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*File Header*/
#include "pantalla.h"

void limpiarPantalla(void)
{
  int i;
  i = 0;
  while (i<LINEAS_PANTALLA)
  {
    puts("");
    i++;
  }
}

void mostrarErrorEntrada(void)
{
  puts("Valor Inválido!");
}

char* solicitarEntrada(char* entrada, const char* mensaje)
{
  puts(mensaje);
  return fgets(entrada, LARGO_ENTRADA, stdin);
}

int solicitarCadena(char* cadena, const char* mensaje)
{
  int valorRetorno;
  char* entrada;
  valorRetorno = 0;
  entrada = (char*)malloc(sizeof(char)*LARGO_ENTRADA);
  memset(entrada,0x00,sizeof(char)*LARGO_ENTRADA);
  if (solicitarEntrada(entrada, mensaje) != 0)
  {
    if (sscanf(entrada, "%s", cadena) != 1)
    {
      valorRetorno = -1;
      mostrarErrorEntrada();
    }
  }
  else
  {
    valorRetorno = -1;
    mostrarErrorEntrada();
  }
  free(entrada);
  return valorRetorno;
}

int solicitarEntero(int* entero, const char* mensaje)
{
  int valorRetorno;
  char* entrada;
  valorRetorno = 0;
  entrada = (char*)malloc(sizeof(char)*LARGO_ENTRADA);
  memset(entrada,0x00,sizeof(char)*LARGO_ENTRADA);
  if (solicitarEntrada(entrada, mensaje) != 0)
  {
    if (sscanf(entrada, "%d", entero) != 1)
    {
      valorRetorno = -1;
      mostrarErrorEntrada();
    }
  }
  else
  {
    valorRetorno = -1;
    mostrarErrorEntrada();
  }
  free(entrada);
  return valorRetorno;
}

int solicitarCaracter(const char* mensaje)
{
  puts(mensaje);
  return getchar();
}

