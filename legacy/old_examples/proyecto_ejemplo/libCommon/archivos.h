#ifndef _ARCHIVOS_H
#define _ARCHIVOS_H
#define LARGO_LINEA 100
int abrirArchivo(char*, char*);
int escribirArchivo(char*);
int leerStringArchivo(char*);
int leerNumeroArchivo(int*);
char* leerLineaArchivo(char*);
void cerrarArchivo(void);
int abrirLectura(char*);
int abrirEscritura(char*);
int abrirAdicion(char*);
int esFinArchivo(void);
int limpiarArchivo(char*);
int renombrarArchivo(char*, char*);
int backupArchivo(char*, char*);
int obtenerTamanioArchivo(char*);
int obtenerUltimaLineaArchivo(char*, char*);
int obtenerTotalLineasArchivo(char*);
#endif
