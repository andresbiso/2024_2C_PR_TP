/*Standard Library*/
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/sem.h>
/*Headers Library*/
#include "claves.h"
/*File Header*/
#include "semaforos.h"

int crearSemaforoMultipleConClave(int idClave, int cantidad)
{
  key_t clave;
  int idSemaforo;
  clave = crearClaveConId(idClave);
  idSemaforo = semget(clave, cantidad, 0600|IPC_CREAT);
  /*
   0600: permiso de lectura y escritura para el owner
   El 0 adelante es para que lo tome como octal
   El segundo parámetro es la cantidad de semáforos
   Los semáforos son numerados empezando por el 0
  */
  if (idSemaforo == -1)
  {
    puts("Error: no se ha podido crear el semáforo");
    return 0;
  }
  return idSemaforo;
}

int crearSemaforoMultiple(int cantidad)
{
  return crearSemaforoMultipleConClave(CLAVE_BASE, cantidad);
}

int crearSemaforoConClave(int idClave)
{
  return crearSemaforoMultipleConClave(idClave, SEM_CANT_DEFAULT);
}

int crearSemaforo(void)
{
  return crearSemaforoConClave(CLAVE_BASE);
}

int iniciarSemaforoMultiple(int idSemaforo, int numeroSemaforo, int valor)
{
  return semctl(idSemaforo, numeroSemaforo, SETVAL, valor);
}

int iniciarSemaforo(int idSemaforo, int valor)
{
  return iniciarSemaforoMultiple(idSemaforo, SEM_DEFAULT, valor);
}

int levantarSemaforoMultiple(int idSemaforo, int numeroSemaforo)
{
  struct sembuf operacion;
  /*printf("Levanto Semáforo: %d\n", idSemaforo);*/
  operacion.sem_num = numeroSemaforo;
  /* Para levantar el semáforo, lo incremento en 1 */
  operacion.sem_op = 1;
  operacion.sem_flg = 0;
  return semop(idSemaforo, &operacion, 1);
}

int levantarSemaforo(int idSemaforo)
{
  return levantarSemaforoMultiple(idSemaforo, SEM_DEFAULT);
}

int esperarSemaforoMultiple(int idSemaforo, int numeroSemaforo)
{
  struct sembuf operacion;
  /*printf("Espera Semáforo: %d\n", idSemaforo);*/
  operacion.sem_num = numeroSemaforo;
  /* Para esperar el semáforo, lo decremento en 1 */
  operacion.sem_op = -1;
  operacion.sem_flg = 0;
  return semop(idSemaforo, &operacion, 1);
}

int esperarSemaforo(int idSemaforo)
{
  return esperarSemaforoMultiple(idSemaforo, SEM_DEFAULT);
}

int eliminarSemaforoMutiple(int idSemaforo, int numeroSemaforo)
{
  return semctl(idSemaforo, numeroSemaforo, IPC_RMID);
}

int eliminarSemaforos(int idSemaforo, int cantidad)
{
  int i;
  int result;
  result = 0;
  for (i = 0; i < cantidad; i++)
  {
    result = eliminarSemaforoMutiple(idSemaforo, i);
  }
  return result;
}

int eliminarSemaforo(int idSemaforo)
{
  return eliminarSemaforoMutiple(idSemaforo, SEM_DEFAULT);
}
