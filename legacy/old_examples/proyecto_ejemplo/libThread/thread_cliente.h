#ifndef _THREAD_JUGADOR_H
#define _THREAD_JUGADOR_H
/*Headers Library*/
#include "libCore/defines.h"
#include "libCore/memoria_core.h"
struct tipo_jugador
{
  int idColaMensajes;
  dato_memoria* memoria;
  /*custom*/
  int nroJugador;
  char nombreJugador[LARGO_NOMBRE];
};
typedef struct tipo_jugador jugador;
void* jugadorThread(void*);
#endif
