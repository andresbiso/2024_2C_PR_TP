/*Standard Library*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
/*Headers Library*/
#include "claves.h"
/*File Header*/
#include "cola.h"

int crearColaMensajesConClave(int clave)
{
  int idColaMensajes = msgget(crearClaveConId(clave), 0600 | IPC_CREAT);
  if (idColaMensajes == -1)
  {
    puts("Error: no se ha podido obtener identificador para cola de mensajes");
  }
  return idColaMensajes;
}

int crearColaMensajes(void)
{
  return crearColaMensajesConClave(CLAVE_BASE);
}

int borrarMensajes(int idColaMensajes)
{
  mensaje* msg;
  int res;
  msg = (mensaje*)malloc(sizeof(mensaje));
  do
  {
    memset(msg, 0x00, sizeof(mensaje));
    /*(struct msgbuf*)&msg*/
    res = msgrcv(idColaMensajes, &msg, sizeof(msg->intRte)+sizeof(msg->intEvento)+sizeof(msg->charMensaje), 0, IPC_NOWAIT);
  }
  while (res>0);
  free(msg);
  return res;
}

int recibirMensaje(int idColaMensajes, long rLongDest, mensaje* rMsg)
{
  mensaje* msg;
  int res;
  msg = (mensaje*)malloc(sizeof(mensaje));
  memset(msg, 0x00, sizeof(mensaje));
  /*(struct msgbuf*)&msg*/
  res = msgrcv(idColaMensajes, msg, sizeof(msg->intRte)+sizeof(msg->intEvento)+sizeof(msg->charMensaje), rLongDest, 0);
  if (res >= 0)
  {
    rMsg->longDest = msg->longDest;
    rMsg->intRte = msg->intRte;
    rMsg->intEvento = msg->intEvento;
    strcpy(rMsg->charMensaje, msg->charMensaje);
  }
  free(msg);
  return res;
}

int recibirMensajeSinEspera(int idColaMensajes, long rLongDest, mensaje* rMsg)
{
  mensaje* msg;
  int res;
  msg = (mensaje*)malloc(sizeof(mensaje));
  memset(msg, 0x00, sizeof(mensaje));
  /*(struct msgbuf*)&msg*/
  res = msgrcv(idColaMensajes, msg, sizeof(msg->intRte)+sizeof(msg->intEvento)+sizeof(msg->charMensaje), rLongDest, IPC_NOWAIT);
  if (res >= 0)
  {
    rMsg->longDest = msg->longDest;
    rMsg->intRte = msg->intRte;
    rMsg->intEvento = msg->intEvento;
    strcpy(rMsg->charMensaje, msg->charMensaje);
  }
  free(msg);
  return res;
}

int enviarMensaje(int idColaMensajes, long rLongDest, int rIntRte, int rIntEvento, char* rpCharMsg)
{
  mensaje* msg;
  int res;
  msg = (mensaje*)malloc(sizeof(mensaje));
  memset(msg, 0x00, sizeof(mensaje));
  msg->longDest = rLongDest;
  msg->intRte = rIntRte;
  msg->intEvento = rIntEvento;
  strcpy(msg->charMensaje, rpCharMsg);
  /*(struct msgbuf*)&msg*/
  res = msgsnd(idColaMensajes, msg, sizeof(msg->intRte)+sizeof(msg->intEvento)+sizeof(msg->charMensaje), IPC_NOWAIT);
  free(msg);
  return res;
}

void liberarColaMensajes(int idColaMensajes)
{
  msgctl(idColaMensajes, IPC_RMID, (struct msqid_ds*) 0);
}
