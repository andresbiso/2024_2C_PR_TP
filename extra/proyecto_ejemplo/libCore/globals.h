#ifndef _GLOBALS_H
#define _GLOBALS_H
/*Standard Library*/
#include <pthread.h>
/*Message Queue*/
enum destinos
{
  MSG_NADIE, /*0*/
  MSG_PARTIDO,
  MSG_JUGADOR
};
enum eventos
{
  EVT_NINGUNO, /*0*/
  EVT_GOL,
  EVT_FUERA,
  EVT_PALO,
  EVT_ATAJA
};
typedef enum destinos Destinos;
typedef enum eventos Eventos;
/*Shared Global Variables*/
extern pthread_mutex_t mutex;
extern int cantidadJugadores;
#endif
