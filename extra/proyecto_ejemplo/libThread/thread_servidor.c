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
#include "libCore/menus.h"
/*File Header*/
#include "thread_partido.h"

int validarGanador(partido* datosThread)
{
  int i;
  int j;
  int golesGanador;
  int intentosGanador;
  int golesJugador;
  int intentosJugador;
  int posicionGanador;
  for (i = 0; i < cantidadJugadores; i++)
  {
    golesJugador = leerGoles(datosThread->memoria, i);
    if (golesJugador >= MAX_GOLES)
    {
      golesGanador = golesJugador;
      intentosGanador = leerIntentos(datosThread->memoria, i);
      posicionGanador = i;
      for (j = 0; j < cantidadJugadores; j++)
      {
        intentosJugador = leerIntentos(datosThread->memoria, j);
        /*Si al menos un jugador no alcanzó la misma cantidad de intentos*/
        if (intentosJugador != intentosGanador)
        {
          return -1;
        }
      }
      for (j = 0; j < cantidadJugadores; j++)
      {
        golesJugador = leerGoles(datosThread->memoria, j);
        if (golesJugador > golesGanador)
        {
          golesGanador = golesJugador;
          posicionGanador = j;
        }
        else if (posicionGanador != j && golesJugador == golesGanador) 
        {
          return -1;
        }
      }
      return posicionGanador;
    }
  }
  return -1;
}

int partidoAcciones(int opcion, int idColaMensajes, int numJugador)
{
  int evento;
  evento = -1;
  switch (opcion) {
    case AC_PATEAR:
      evento = obtenerNumeroAleatorio(EVENTO_MIN, EVENTO_MAX);
      enviarMensaje(idColaMensajes, MSG_JUGADOR + numJugador, MSG_PARTIDO, evento, "");
      usleep(100 * 1000);
      break;
    default:
      break;
  }
  return evento;
}

void* partidoThread(void* parametro)
{
  mensaje* msg;
  partido* datosThread;
  int opcion;
  int numJugador;
  int intentosJugador;
  int ganador;
  int i;
  int jugadorMensaje;
  char nomJugador[LARGO_NOMBRE];

  datosThread = (partido*)parametro;
  opcion = -1;
  numJugador = 0;
  intentosJugador = 0;
  ganador = -1;
  jugadorMensaje = 0;
  memset(nomJugador, 0x00, sizeof(char)*LARGO_NOMBRE);

  msg = (mensaje*)malloc(sizeof(mensaje));

  intentosJugador = leerIntentos(datosThread->memoria, 0);
  strcpy(nomJugador, obtenerNombreJugadorPorNumero(0));
  intentosJugador++;
  printf("Intento Nº %d\n", intentosJugador);
  opcion = mostrarMenuPartido(nomJugador);
  partidoAcciones(opcion, datosThread->idColaMensajes, 0);

  while(1)
  {
    memset(msg, 0x00, sizeof(mensaje));
    msg->longDest = MSG_NADIE;
    msg->intRte = MSG_NADIE;
    msg->intEvento = EVT_NINGUNO;

    recibirMensajeSinEspera(datosThread->idColaMensajes, MSG_PARTIDO, msg);
    usleep(100 * 1000);

    numJugador = msg->intRte - MSG_JUGADOR;
    strcpy(nomJugador, obtenerNombreJugadorPorNumero(numJugador));

    switch(msg->intEvento)
    {
      case EVT_NINGUNO:
        break;
      case EVT_GOL:
        printf("Gol de %s\n", nomJugador);
        for (i = 0; i < cantidadJugadores; i++)
        {
          printf("Jugador %s: %d goles\n", obtenerNombreJugadorPorNumero(i), leerGoles(datosThread->memoria, i));
        }
        break;
      case EVT_FUERA:
        printf("Jugador %s: fuera\n", nomJugador);
        break;
      case EVT_PALO:
        printf("Jugador %s: palo\n", nomJugador);
        break;
      case EVT_ATAJA:
        printf("Jugador %s: atajado\n", nomJugador);
        break;
      default:
        break;
    }

    if (msg->intEvento != EVT_NINGUNO)
    {
      ganador = validarGanador(datosThread);
      if (ganador >= 0)
      {
        strcpy(nomJugador, obtenerNombreJugadorPorNumero(ganador));
        printf("Jugador %s: ganó el partido con %d goles\n", nomJugador, leerGoles(datosThread->memoria, ganador));
        for (i = 0; i < cantidadJugadores; i++)
        {
          escribirFinPartido(datosThread->memoria, i, 1);
          usleep(100 * 1000);
        }
        break;
      }

      jugadorMensaje = 0;
      for(i = 0; i < cantidadJugadores; i++)
      {
        if (leerIntentos(datosThread->memoria, jugadorMensaje) > leerIntentos(datosThread->memoria, i))
        {
          jugadorMensaje = i;
        }
      }

      intentosJugador = leerIntentos(datosThread->memoria, jugadorMensaje);
      strcpy(nomJugador, obtenerNombreJugadorPorNumero(jugadorMensaje));
      intentosJugador++;
      printf("Intento Nº %d\n", intentosJugador);
      opcion = mostrarMenuPartido(nomJugador);
      partidoAcciones(opcion, datosThread->idColaMensajes, jugadorMensaje);
    }

    usleep(INTERVALO_PARTIDO_MS * 1000);
  }
  free(msg);
  return 0;
}
