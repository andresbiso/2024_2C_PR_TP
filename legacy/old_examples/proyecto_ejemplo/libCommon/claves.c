/*Standard Library*/
#include <stdio.h>
#include <sys/ipc.h>
/*File Header*/
#include "claves.h"

key_t crearClaveConIdRuta(int idClave, const char* ruta)
{
  /* Igual que en cualquier recurso compartido (memoria compartida,
   semáforos o colas) se obtiene una clave a partir de un fichero
   existente cualquiera y de un entero cualquiera.
   Todos los procesos que quieran compartir este recurso,
   deben usar el mismo fichero y el mismo entero.
  */
  key_t clave;
  clave = ftok(ruta, idClave);
  if (clave == (key_t)-1)
  {
    puts("No se pudo conseguir clave");
    return 0;
  }
  return clave;
}

key_t crearClaveConId(int idClave)
{
  return crearClaveConIdRuta(idClave, RUTA_ARCHIVO_FTOK);
}

key_t crearClave(void)
{
  return crearClaveConId(CLAVE_BASE);
}
