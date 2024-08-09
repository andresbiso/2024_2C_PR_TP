#ifndef _HILOS_H
#define _HILOS_H
/*Standard Library*/
#include <pthread.h>
int iniciarMutex(pthread_mutex_t*);
int destruirMutex(pthread_mutex_t*);
int iniciarAttr(pthread_attr_t*);
int destruirAttr(pthread_attr_t*);
int asignarEstadoJoinableAttr(pthread_attr_t*);
int joinThread(pthread_t*);
int joinThreadValorSalida(pthread_t*,char*);
int lockMutex(pthread_mutex_t*);
int unlockMutex(pthread_mutex_t*);
int crearThread(pthread_t*, pthread_attr_t*, void* (*f)(void*), void*);
#endif
