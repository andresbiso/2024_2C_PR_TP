/*Standard Library*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*File Header*/
#include "archivos.h"

/*Variable global*/
FILE* cfptr;

int abrirArchivo(char* ruta, char* modo)
{
  return (cfptr=fopen(ruta,modo))!=0;
}

int escribirArchivo(char* linea)
{
  return fprintf(cfptr,"%s",linea);
}

int leerStringArchivo(char* linea)
{
  return fscanf(cfptr,"%s",linea);
}

int leerNumeroArchivo(int* numero)
{
  return fscanf(cfptr,"%d",numero);
}

char* leerLineaArchivo(char* linea)
{
  return fgets(linea, LARGO_LINEA, cfptr);
}

void cerrarArchivo(void)
{
  if (cfptr != 0)
  {
    fclose(cfptr);
  }
}

int abrirLectura(char* ruta)
{
  return abrirArchivo(ruta, "r");
}

int abrirEscritura(char* ruta)
{
  return abrirArchivo(ruta, "w");
}

int abrirAdicion(char* ruta)
{
  return abrirArchivo(ruta, "a");
}

int esFinArchivo(void)
{
  return feof(cfptr);
}

int limpiarArchivo(char* ruta)
{
  if (!abrirEscritura(ruta))
  {
    puts("Hubo un error al querer abrir el archivo");
    return -1;
  }
  cerrarArchivo();
  return 0;
}

int renombrarArchivo(char* ruta, char* rutaNuevoNombre)
{
  return rename(ruta, rutaNuevoNombre);
} 

int backupArchivo(char* ruta, char* rutaNuevoNombre)
{
  int cantLineasCopiadas;
  char* linea;
  FILE* cfptrBak;

  cantLineasCopiadas = 0;
  linea = (char*)malloc(sizeof(char)*LARGO_LINEA);
  memset(linea,0x00,sizeof(char)*LARGO_LINEA);

  if (!abrirLectura(ruta))
  {
   puts("Hubo un error al querer abrir el archivo");
   return -1;
  }

  if ((cfptrBak=fopen(rutaNuevoNombre,"a"))==0)
  {
     puts("Hubo un error al querer abrir el archivo de backup");
     return -1;
  }

  while(leerLineaArchivo(linea) != 0)
  {
    fprintf(cfptrBak,"%s",linea);
    cantLineasCopiadas++;
    memset(linea,0x00,sizeof(char)*LARGO_LINEA);
  }

  cerrarArchivo();
  fclose(cfptrBak);
  if (cantLineasCopiadas > 0)
  {
    printf("Ruta Backup: %s\n", rutaNuevoNombre);
  }
  free(linea);
  return 0;
}

int obtenerTamanioArchivo(char* ruta)
{
  int tamanioArchivo;
  tamanioArchivo = 0;
  if (!abrirLectura(ruta))
  {
   puts("Hubo un error al querer abrir el archivo");
   return -1;
  }

  /* SEEK_SET = 0; Principio */
  /* SEEK_END = 2; Fin */
  fseek(cfptr, 0, 2);
  tamanioArchivo = ftell(cfptr);

  /*En caso de querer volver al principio*/
  /*Descomentar la siguiente línea*/
  /*fseek(cfptr, 0, 0);*/

  /*En caso de querer mostrar el tamaño del archivo por pantalla*/
  /*printf("Tamaño Archivo: %d\n", tamanioArchivo);*/

  cerrarArchivo();
  return tamanioArchivo;
}

int obtenerUltimaLineaArchivo(char* ruta, char* linea)
{
  if (!abrirLectura(ruta))
  {
   puts("Hubo un error al querer abrir el archivo");
   return -1;
  }

  while(1)
  {
    if (leerLineaArchivo(linea) == 0)
    {
      break;
    }
  }

  cerrarArchivo();
  return 0;
}

int obtenerTotalLineasArchivo(char* ruta)
{
  char* linea;
  int cantidadLineas;
  cantidadLineas = 0;
  linea = (char*)malloc(sizeof(char)*LARGO_LINEA);
  memset(linea,0x00,sizeof(char)*LARGO_LINEA);

  if (!abrirLectura(ruta))
  {
   puts("Hubo un error al querer abrir el archivo");
   return -1;
  }

  while(1)
  {
    if (leerLineaArchivo(linea) == 0)
    {
      break;
    }
    cantidadLineas++;
  }
  cerrarArchivo();
  return cantidadLineas;
}
