/*Standard Library*/
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
/*File Header*/
#include "signals.h"

/*Ejemplo uso:
void manejador(int signum)
{
    if (signum == SIGINT)
    {
        puts("señal capturada");
    }
}
crearSignal(SIGINT, manejador);
*/
void (*crearSignal(int numSignal, void (*manejador)(int)))(int)
{
    return signal(numSignal, manejador);
}

/*
Ejemplo:
#include <unistd.h>
pid_t getpid(void);
printf("sending SIGUSR1 to pid %d\n", (int)getpid());
enviarSignal(getpid(), SIGUSR1);
*/
/*Signals configurables: SIGUSR1, SIGUSR2*/
/*Signals que no pueden ser ignoradas: SIGSTOP, SIGKILL*/
int enviarSignal(pid_t pid, int numSignal)
{
    return kill(pid, numSignal);
}

/*Ejemplo:
crearSignal(SIGALRM, manejador);
crearAlarma(10);
*/
/*Envía el SIGALRM al mismo proceso que la invoca*/
unsigned int crearAlarma(unsigned int segundos)
{
    return alarm(segundos);
}

unsigned int cancelarAlarma(void)
{
    return alarm(0);
}

void (*ignorarSignal(int numSignal))(int)
{
    return crearSignal(numSignal, SIG_IGN);
}

void (*restaurarSignal(int numSignal))(int)
{
    return crearSignal(numSignal, SIG_DFL);
}
