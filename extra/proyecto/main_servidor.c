/*Standard Library*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
/*Headers Library*/
#include "libCommon/memoria.h"
#include "libCommon/memoria_ini.h"
#include "libCommon/cola.h"
#include "libCommon/hilos.h"
#include "libCommon/aleatorio.h"
#include "libCommon/pantalla.h"
#include "libCommon/signals.h"
#include "libCore/defines.h"
#include "libCore/globals.h"
#include "libCore/funciones.h"
#include "libCore/productor.h"
#include "libCore/consumidor.h"
#include "libCore/memoria_core.h"
#include "libThread/thread_partido.h"

int idColaMensajes;
int idMemoria;
dato_memoria* memoria;
int idMemoriaIni;
dato_memoria_ini* memoriaIni;
pthread_attr_t atributos;

void liberarRecursos(void)
{
  liberarMemoria(idMemoriaIni, (char*)memoriaIni);
  liberarMemoria(idMemoria, (char*)memoria);
  liberarColaMensajes(idColaMensajes);
  destruirAttr(&atributos);
}

void manejadorSignals(int signum)
{
    if (signum == SIGINT)
    {
      puts("Liberando Recursos...");
      liberarRecursos();
      puts("Proceso Finalizado");
      exit(0);
    }
}

int main(int argc, char *argv[])
{
  pthread_t idHilo;
  partido datosThread;

  if (argc != 1)
  {
    puts("Uso: ./partido");
    return -1;
  }

  memoriaIni = 0;
  idMemoriaIni = 0;
  memoria = 0;
  idMemoria = 0;
  idColaMensajes = 0;
  idHilo = 0;
  cantidadJugadores = 2;

  srand(time(0));

  iniciarAttr(&atributos);
  asignarEstadoJoinableAttr(&atributos);

  crearSignal(SIGINT, manejadorSignals);

  idColaMensajes = crearColaMensajes();
  borrarMensajes(idColaMensajes);
  memoria = (dato_memoria*)crearMemoria(sizeof(dato_memoria)*cantidadJugadores, &idMemoria);
  configurarMemoria(memoria, cantidadJugadores);
  memoriaIni = crearMemoriaIni(&idMemoriaIni);
  configurarMemoriaIni(memoriaIni);

  limpiarPantalla();

  printf("Cantidad Jugadores %d\n", cantidadJugadores);

  datosThread.idColaMensajes = idColaMensajes;
  datosThread.memoria = memoria;

  if (!crearThread(&idHilo, &atributos, partidoThread, (void*)&datosThread))
  {
    puts("Error: No se pude crear el thread");
    return -1;
  }

  joinThread(&idHilo);

  verificarFinalizarMemoriaIni(memoriaIni, "jugador");

  liberarRecursos();
  return 0;
}
