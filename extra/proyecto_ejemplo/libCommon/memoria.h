#ifndef _MEMORIA_H
#define _MEMORIA_H
#define POS_INI_MEM 0
void* crearMemoriaConClaveRuta(int, int*, int, const char*);
void* crearMemoriaConClave(int, int*, int);
void* crearMemoria(int, int*);
void liberarMemoria(int, char*);
#endif
