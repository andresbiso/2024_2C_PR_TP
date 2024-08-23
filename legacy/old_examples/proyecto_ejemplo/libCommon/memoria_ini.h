#ifndef _MEMORIA_INI_H
#define _MEMORIA_INI_H
#define RUTA_ARCHIVO_FTOK_INI "/bin/cat"
#define CLAVE_BASE_INI 34
struct tipo_dato_memoria_ini
{
  int inicializado;
  int finalizado;
};
typedef struct tipo_dato_memoria_ini dato_memoria_ini;
dato_memoria_ini* crearMemoriaIni(int*);
void configurarMemoriaIni(dato_memoria_ini*);
void verificarMemoriaIni(dato_memoria_ini*, char*);
void configurarFinalizarMemoriaIni(dato_memoria_ini*);
void verificarFinalizarMemoriaIni(dato_memoria_ini*, char*);
#endif
