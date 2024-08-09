/*Standard Library*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
/*File Header*/
#include "tiempos.h"

/*Devuelve el tiempo en ms*/
/*Ejemplo uso:
1. tiempo t_inicio; tiempo t_final;
2. obtenerTiempoActual(&t_inicio);
3. obtenerTiempoActual(&t_final);
4. printf("Tiempo empleado: %dms\n", obtenerTiempoTranscurrido(t_inicio, t_final));
*/
int obtenerTiempoTranscurrido(tiempo tiempoInicio, tiempo tiempoFin)
{
    int secInicio, microInicio;
    int inicio;
    int secFin, microFin;
    int fin;

    secInicio = tiempoInicio.tv_sec & 0xFFF;
    microInicio = tiempoInicio.tv_usec;
    inicio = (secInicio * 1000) + (microInicio / 1000);

    secFin = tiempoFin.tv_sec & 0xFFF;
    microFin = tiempoFin.tv_usec;
    fin = (secFin * 1000) + (microFin / 1000);
    return fin - inicio;
}

int obtenerTiempoActual(tiempo* tiempoActual)
{
    return gettimeofday(tiempoActual, 0);
}

