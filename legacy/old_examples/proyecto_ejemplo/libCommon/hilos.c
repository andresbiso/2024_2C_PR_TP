/*Standard Library*/
#include <pthread.h>
/*File Header*/
#include "hilos.h"

int iniciarMutex(pthread_mutex_t* mutex)
{
  return pthread_mutex_init(mutex, 0);
}

int destruirMutex(pthread_mutex_t* mutex)
{
  return pthread_mutex_destroy(mutex);
}

int iniciarAttr(pthread_attr_t* atributos)
{
  return pthread_attr_init(atributos);
}

int destruirAttr(pthread_attr_t* atributos)
{
  return pthread_attr_destroy(atributos);
}

int asignarEstadoJoinableAttr(pthread_attr_t* atributos)
{
  return pthread_attr_setdetachstate(atributos, PTHREAD_CREATE_JOINABLE);
}

int joinThread(pthread_t* hilo)
{
  return pthread_join(*hilo, 0);
}

/*Ejemplo uso: joinThreadValorSalida(&idHilo, (void*)&valorDevuelto)*/
/*char* valorDevuelto; valorDevuelto = 0;*/
/*No responde bien al free(valorDevuelto) luego del pthread_exit()*/

/*Alternativa valor de retorno:*/
/*Permite retornar un valor*/
/*En vez de return utilizamos la siguiente función:*/
/*pthread_exit((void*)0);*/
/*Donde esta el 0 podemos indicar cualquier valor de salida*/
int joinThreadValorSalida(pthread_t* hilo, char* valorDevuelto)
{
  return pthread_join(*hilo, (void**)valorDevuelto);
}

int lockMutex(pthread_mutex_t* mutex)
{
  return pthread_mutex_lock(mutex);
}

int unlockMutex(pthread_mutex_t* mutex)
{
  return pthread_mutex_unlock(mutex);
}

int crearThread(pthread_t* hilo, pthread_attr_t* atributos, void* (*funcionThread)(void*), void* arg)
{
  return pthread_create(hilo, atributos, funcionThread, arg) == 0;
}
