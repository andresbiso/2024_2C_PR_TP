#ifndef _CLAVES_H
#define _CLAVES_H
#define RUTA_ARCHIVO_FTOK "/bin/ls"
#define CLAVE_BASE 33
/*Standard Library*/
#include <sys/ipc.h>
key_t crearClaveConIdRuta(int, const char*);
key_t crearClaveConId(int);
key_t crearClave(void);
#endif
