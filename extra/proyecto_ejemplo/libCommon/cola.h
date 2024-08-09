#ifndef _COLA_H
#define _COLA_H
#define LARGO_MENSAJE 1024
struct tipo_mensaje
{
  long longDest; /*Destinatario*/
  int intRte; /*Remitente*/
  int intEvento; /*NroEvento*/
  char charMensaje[LARGO_MENSAJE]; /*Mensaje*/
};
typedef struct tipo_mensaje mensaje;
int crearColaMensajesConClave(int);
int crearColaMensajes(void);
int borrarMensajes(int);
int recibirMensaje(int, long, mensaje*);
int recibirMensajeSinEspera(int, long, mensaje*);
int enviarMensaje(int, long, int, int, char*);
void liberarColaMensajes(int);
#endif
