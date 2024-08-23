#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/poll.h>



#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

#ifndef CONST
#define CONST               const
#endif

typedef unsigned long 		ULONG;
typedef 					ULONG *PULONG;
typedef unsigned short 		USHORT;
typedef 					USHORT *PUSHORT;
typedef unsigned char 		UCHAR;
typedef 					UCHAR *PUCHAR;
typedef char 				*PSZ;
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;
typedef FLOAT 				*PFLOAT;
typedef void				VOID, *PVOID;
typedef char				CHAR, *PCHAR;
typedef int                 INT;
typedef unsigned int        UINT;
typedef unsigned int        *PUINT;
typedef short 				SHORT;
typedef long 				LONG;
typedef double 				LONGLONG;
typedef double 				ULONGLONG;
typedef BYTE				BOOLEAN;
typedef char				**PPCHAR;