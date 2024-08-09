#ifndef _SIGNALS_H
#define _SIGNALS_H
#define TIEMPO_ALARMA_DEFAULT 15
/*Standard Library*/
#include <signal.h>
/*declare crearSignal as function (int, pointer to function (int) returning void)*/
/*returning pointer to function (int) returning voi*/
void (*crearSignal(int, void (*)(int)))(int);
int enviarSignal(pid_t, int);
unsigned int crearAlarma(unsigned int);
unsigned int cancelarAlarma(void);
void (*ignorarSignal(int numSignal))(int);
void (*restaurarSignal(int numSignal))(int);
#endif

