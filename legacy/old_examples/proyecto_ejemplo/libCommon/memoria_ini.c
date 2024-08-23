/*Standard Library*/
#include <stdio.h>
/*Headers Library*/
#include "memoria.h"
#include "pantalla.h"
/*File Header*/
#include "memoria_ini.h"

dato_memoria_ini* crearMemoriaIni(int* extIdMemoria)
{
  return (dato_memoria_ini*)crearMemoriaConClaveRuta(sizeof(dato_memoria_ini), extIdMemoria, CLAVE_BASE_INI, RUTA_ARCHIVO_FTOK_INI);
}

void configurarMemoriaIni(dato_memoria_ini* memoria)
{
  memoria[POS_INI_MEM].inicializado = 1;
  memoria[POS_INI_MEM].finalizado = 0;
}

void verificarMemoriaIni(dato_memoria_ini* memoria, char* nombrePrograma)
{
  while (memoria[POS_INI_MEM].inicializado != 1)
  {
    printf("Atención: Recuerde levantar el programa inicial \"%s\". Presione \"enter\" para continuar...\n", nombrePrograma);
    getchar();
  }
}

void configurarFinalizarMemoriaIni(dato_memoria_ini* memoria)
{
  memoria[POS_INI_MEM].finalizado = 1;
}

void verificarFinalizarMemoriaIni(dato_memoria_ini* memoria, char* nombrePrograma)
{
  while (memoria[POS_INI_MEM].finalizado != 1)
  {
    printf("Atención: Esperando finalización de programa \"%s\". Presione \"enter\" para continuar...\n", nombrePrograma);
    getchar();
  }
}
