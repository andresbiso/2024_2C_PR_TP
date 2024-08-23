/*Standard Library*/
#include <stdio.h>
#include <sys/shm.h>
#include <sys/ipc.h>
/*Headers Library*/
#include "claves.h"
/*File Header*/
#include "memoria.h"

void* crearMemoriaConClaveRuta(int size, int* extIdMemoria, int clave, const char* ruta)
{
  void* ptrMemoria;
  int idMemoria;
  
  idMemoria = shmget(crearClaveConIdRuta(clave, ruta), size, 0777 | IPC_CREAT);
  /*0777: permisos de rwx para owner/group/other*/
  /*IPC_CREAT: crea la memoria si no existe*/
  if (idMemoria == -1)
  {
    puts("Error: no se ha podido conseguir id para la memoria compartida");
    return 0;
  }
  ptrMemoria = (void *)shmat(idMemoria, (char*)0, 0);

  if (ptrMemoria == 0)
  {
    puts("Error: no se ha podido conseguir memoria compartida");
    return 0;
  }
  *extIdMemoria = idMemoria;
  return ptrMemoria;
}

void* crearMemoriaConClave(int size, int* extIdMemoria, int clave)
{
  return crearMemoriaConClaveRuta(size, extIdMemoria, clave, RUTA_ARCHIVO_FTOK);
}

void* crearMemoria(int size, int* extIdMemoria)
{
  return crearMemoriaConClave(size, extIdMemoria, CLAVE_BASE);
}

void liberarMemoria(int idMemoria, char* memoria)
{
  shmdt(memoria);
  shmctl(idMemoria, IPC_RMID, (struct shmid_ds*)0);
}
