/*Standard Library*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
/*Headers Library*/
#include "libCommon/aleatorio.h"
#include "libCommon/hilos.h"
#include "libCommon/cola.h"
#include "libCore/defines.h"
#include "libCore/globals.h"
#include "libCore/funciones.h"
/*File Header*/
#include "thread_jugador.h"

void* jugadorThread(void* parametro)
{
  mensaje* msg;
  jugador* datosThread;
  int golesJugador;
  int intentosJugador;
  int finPartido;

  datosThread = (jugador*)parametro;
  golesJugador = 0;
  intentosJugador = 0;
  finPartido = 0;

  msg = (mensaje*)malloc(sizeof(mensaje));

  while(1)
  {
    finPartido = leerFinPartido(datosThread->memoria, datosThread->nroJugador);

    if (finPartido == 1)
    {
      printf("Jugador %s: finalizó de jugar\n", datosThread->nombreJugador);
      break;
    }
    
    memset(msg, 0x00, sizeof(mensaje));
    msg->longDest = MSG_NADIE;
    msg->intRte = MSG_NADIE;
    msg->intEvento = EVT_NINGUNO;

    lockMutex(&mutex);
    recibirMensajeSinEspera(datosThread->idColaMensajes, MSG_JUGADOR + datosThread->nroJugador, msg);
    unlockMutex(&mutex);
    usleep(100 * 1000);

    if (msg->intEvento != EVT_NINGUNO)
    {
      intentosJugador = leerIntentos(datosThread->memoria, datosThread->nroJugador);
      intentosJugador++;
      lockMutex(&mutex);
      escribirIntentos(datosThread->memoria, datosThread->nroJugador, intentosJugador);
      unlockMutex(&mutex);
      printf("Jugador %s: intento -> %d\n", datosThread->nombreJugador, intentosJugador);
    }
  
    switch(msg->intEvento)
    {
      case EVT_NINGUNO:
        break;
      case EVT_GOL:
        printf("Jugador %s: metió gol\n", datosThread->nombreJugador);
        golesJugador = leerGoles(datosThread->memoria, datosThread->nroJugador);
        golesJugador++;
        lockMutex(&mutex);
        escribirGoles(datosThread->memoria, datosThread->nroJugador, golesJugador);
        enviarMensaje(datosThread->idColaMensajes, MSG_PARTIDO, MSG_JUGADOR + datosThread->nroJugador, EVT_GOL, "");
        unlockMutex(&mutex);
        usleep(100 * 1000);
        printf("Jugador %s: total goles -> %d\n", datosThread->nombreJugador, golesJugador);
        break;
      case EVT_FUERA:
        printf("Jugador %s: fuera\n", datosThread->nombreJugador);
        lockMutex(&mutex);
        enviarMensaje(datosThread->idColaMensajes, MSG_PARTIDO, MSG_JUGADOR + datosThread->nroJugador, EVT_FUERA, "");
        unlockMutex(&mutex);
        usleep(100 * 1000);
        break;
      case EVT_PALO:
        printf("Jugador %s: palo\n", datosThread->nombreJugador);
        lockMutex(&mutex);
        enviarMensaje(datosThread->idColaMensajes, MSG_PARTIDO, MSG_JUGADOR + datosThread->nroJugador, EVT_PALO, "");
        unlockMutex(&mutex);
        usleep(100 * 1000);
        break;
      case EVT_ATAJA:
        printf("Jugador %s: atajado\n", datosThread->nombreJugador);
        lockMutex(&mutex);
        enviarMensaje(datosThread->idColaMensajes, MSG_PARTIDO, MSG_JUGADOR + datosThread->nroJugador, EVT_ATAJA, "");
        unlockMutex(&mutex);
        usleep(100 * 1000);
        break;
      default:
        break;
    }

    usleep(INTERVALO_JUGADOR_MS * 1000);
  }
  free(msg);
  return 0;
}
