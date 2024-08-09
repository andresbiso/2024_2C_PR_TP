#ifndef _PANTALLA_H
#define _PANTALLA_H
#define LINEAS_PANTALLA 80
#define LARGO_ENTRADA 100
void limpiarPantalla(void);
void mostrarErrorEntrada(void);
char* solicitarEntrada(char*, const char*);
int solicitarCadena(char*, const char*);
int solicitarEntero(int*, const char*);
int solicitarCaracter(const char*);
#endif
