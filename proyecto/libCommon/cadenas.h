#ifndef _CADENAS_H
#define _CADENAS_H
#define DELIMITADOR_BASE "|"
char letraMayuscula(char);
char letraMinuscula(char);
void cadenaMayuscula(char*, char*);
void cadenaMinuscula(char*, char*);
char* unirCadenas(char*, char*);
char* unirCadenasConDelimitador(char*, char*, char*);
char* separarCadenaConDelimitador(char*, char*);
int contarSeparacionesCadena(char*, char*);
int convertirNumeroEnCadena(char*, int);
#endif
