#ifndef _MENUS_H
#define _MENUS_H
#define TECLA_ENTER '\n'
enum acciones_menu_partido
{
  AC_NINGUNA, /*0*/
  AC_PATEAR
};
typedef enum acciones_menu_partido AccionesMenuPartido;
int mostrarMenuPartido(char*);
#endif
