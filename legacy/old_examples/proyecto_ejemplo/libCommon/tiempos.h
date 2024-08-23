#ifndef _TIEMPOS_H
#define _TIEMPOS_H
/*Standard Library*/
#include <sys/time.h>
typedef struct timeval tiempo;
int obtenerTiempoTranscurrido(tiempo, tiempo);
int obtenerTiempoActual(tiempo*);
#endif
