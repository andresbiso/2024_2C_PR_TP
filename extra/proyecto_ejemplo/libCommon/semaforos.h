#ifndef _SEMAFOROS_H
#define _SEMAFOROS_H
#define ROJO 0
#define VERDE 1
#define SEM_CANT_DEFAULT 1
#define SEM_DEFAULT 0
int crearSemaforoMultipleConClave(int, int);
int crearSemaforoMultiple(int);
int crearSemaforoConClave(int);
int crearSemaforo(void);
int iniciarSemaforoMultiple(int, int, int);
int iniciarSemaforo(int, int);
int levantarSemaforoMultiple(int, int);
int levantarSemaforo(int);
int esperarSemaforoMultiple(int, int);
int esperarSemaforo(int);
int eliminarSemaforoMutiple(int, int);
int eliminarSemaforos(int, int);
int eliminarSemaforo(int);
#endif
