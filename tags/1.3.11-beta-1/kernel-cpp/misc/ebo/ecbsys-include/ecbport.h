/*----UEBERSETZUNGSEINHEIT---------------------------------------------------*/
/*									     */
/* Name: ecbport.h         						     */
/*									     */
/* Beschreibung: zu portierende Datentypen 				     */
/*									     */
/*									     */
/* Funktionen: keine                                     		     */
/*									     */
/* Autor: <Name>							     */
/*									     */
/* Historie: <projektspezifische Beschreibung von Erstell- und Aenderungs-   */
/*            daten, Version u.s.w.>					     */
/*									     */
/*---------------------------------------------------------------------------*/
#ifndef ECBPORT_INCL

#if defined(WIN32) || defined(MSDOS)
#include <winsock2.h>

#else  /* UNIX */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/times.h>
#include <sys/select.h>
#include <sys/socket.h>

#endif /* WIN32 */


#if defined(WIN32) || defined(MSDOS)
#define FAST
#define PFAST
#else
#define	FAST	register    			/* register variable	   */
#define PFAST	register			/* register parameter	   */
#endif /* WIN32 */

#define	GLOBAL	/* */				/* Global variable	   */
#define	LOCAL	static				/* Local to module	   */


/* !!!!!!!!!!!!!!!!!!!!
	TEXT	1 byte
	COUNT	2 byte
	LONG	4 byte
	
*/

#if defined(WIN32) || defined(MSDOS)
typedef long            ECBERROR;
typedef DWORD           SYSERROR;
#else
typedef long            ECBERROR;
typedef int             SYSERROR;
typedef char          * BSTR;
typedef unsigned char   BYTE;
typedef int             COLORREF;
typedef unsigned int    DWORD;
typedef int             LPARAM;
typedef char *          LPCSTR;
typedef char *          LPSTR;
typedef char *          LPCTSTR;
typedef char *          LPTSTR;
typedef void *          LPVOID;
typedef int             LRESULT;
typedef int             WNDPROC;
#endif  /* defined(WIN32) || defined(MSDOS) */

typedef short           COUNT;
typedef short *         pCOUNT;
typedef pCOUNT *        ppCOUNT;

typedef unsigned short  UCOUNT;         /* some 16 bit compilers might  */
typedef unsigned short *pUCOUNT;
typedef pUCOUNT *       ppUCOUNT;


typedef int		NINT;	/* natural integer */
typedef int *           pNINT;
typedef unsigned int	UINT;
typedef unsigned int *  pUINT;


#if defined(WIN32) || defined(MSDOS)
typedef long            VRLEN;
typedef long *          pVRLEN;
#else
typedef int             VRLEN;
typedef int *           pVRLEN;
#endif


typedef char		TEXT;
typedef char *          pTEXT;
typedef char **         ppTEXT;
typedef unsigned char	UTEXT;	
typedef unsigned char *	pUTEXT;
typedef unsigned char **ppUTEXT;
            
#if defined(WIN32) || defined(MSDOS)
typedef VOID *	        pVOID;
typedef VOID **         ppVOID;
#else
#define VOID		void		/* statt typedef */
typedef VOID *	        pVOID;
typedef VOID **         ppVOID;
#endif

#ifdef MSDOS       
typedef long *          pLONG;
typedef long **         ppLONG;

#else
typedef long            LONG;    
typedef long *          pLONG;
typedef long **         ppLONG;

#endif

			
typedef LONG            POINTER;	/* Verschoben nach hinten 220698-so */
typedef POINTER *       pPOINTER;	/* Verschoben nach hinten 220698-so */

typedef unsigned long   ULONG;
typedef unsigned long * pULONG;
typedef unsigned long **ppULONG;


#ifdef YES
#undef YES
#endif
#ifdef NO
#undef NO
#endif

#if defined(WIN32) || defined(MSDOS)
#define	YES	TRUE
#define	NO	FALSE	
#else
#define	YES	1
#define	NO	0	
#ifndef TRUE
#define	TRUE	YES		
#endif

#ifdef FALSE
#undef FALSE
#endif
#define	FALSE	NO			

#endif

#if defined(WIN32) || defined(MSDOS)
#else
typedef enum /*jz18.9.2000 bool*/ {no=NO,yes=YES} BOOL;	
#define BOOLEAN			   BOOL
#endif

#if defined(WIN32) || defined(MSDOS)
typedef short	IPCKEY;
#else
typedef key_t	IPCKEY;
#endif

#define FOREVER	for (;;)

#if defined(SIY) || defined(SIN) || defined(SIZ) || defined(HPUX)
#define ALIGNMENT	4	
#endif

#if defined(WIN32) || defined(MSDOS)
#define ALIGNMENT	4	
#endif

#ifndef ALIGNMENT
#define ALIGNMENT	2
#endif


#if defined(WIN32) || defined(MSDOS)
#define DLLIMPORT _declspec( dllimport )
#define DLLEXPORT _declspec( dllexport )
#else
#define DLLIMPORT
#define DLLEXPORT
#endif


#ifdef WIN32
typedef int     PIDNR;
#else
typedef long	PIDNR;
#endif

/**************** Socketbibliothek ***************************/

#if defined(WIN32)
typedef SOCKET			ECBSOCKET;
#define ECB_FD_SET		fd_set
#else
typedef int			ECBSOCKET;
#define ECB_FD_SET		fd_set
typedef struct sockaddr		SOCKADDR;
typedef struct sockaddr_in	SOCKADDR_IN;

#define INVALID_SOCKET	   -1
#define SOCKET_ERROR       -1
#endif

/* Laenge einer Interprozesskommunikationsnachricht */
#ifdef MSDOS /* es gibt sie noch, die 16 bit. IPCMSGLEN == 32bit */
typedef long            IPCMSGLEN;
#else
typedef int             IPCMSGLEN;
#endif
                             
/* Konvertierung Host Byte order nach network byte order und vice versa */
#define HTON_IPCMSGLEN  htonl
#define NTOH_IPCMSGLEN  ntohl 
#define HTON_COUNT	htons
#define NTOH_COUNT	ntohs
#define HTON_LONG	htonl
#define NTOH_LONG	ntohl


/* Name des Hosts bei der Interprozesskommunikation */
typedef char		IPCHOST[40];

#ifdef WIN32
#define ECB_SIGHUP  1       /* hangup */
#define ECB_SIGINT  2       /* interrupt (rubout) */
#define ECB_SIGQUIT 3       /* quit (ASCII FS) */
#define ECB_SIGILL  4       /* illegal instruction (not reset when caught) */
#define ECB_SIGTRAP 5       /* trace trap (not reset when caught) */
#define ECB_SIGIOT  6       /* IOT instruction */
#define ECB_SIGABRT 6       /* used by abort, replace SIGIOT in the future */
#define ECB_SIGEMT  7       /* EMT instruction */
#define ECB_SIGFPE  8       /* floating point exception */
#define ECB_SIGKILL 9       /* kill (cannot be caught or ignored) */
#define ECB_SIGBUS  10      /* bus error */
#define ECB_SIGSEGV 11      /* segmentation violation */
#define ECB_SIGSYS  12      /* bad argument to system call */
#define ECB_SIGPIPE 13      /* write on a pipe with no one to read it */
#define ECB_SIGALRM 14      /* alarm clock */
#define ECB_SIGTERM 15      /* software termination signal from kill */
#else
#define ECB_SIGHUP  SIGHUP	       /* hangup */
#define ECB_SIGINT  SIGINT      /* interrupt (rubout) */
#define ECB_SIGQUIT SIGQUIT     /* quit (ASCII FS) */
#define ECB_SIGILL  SIGILL      /* illegal instruction (not reset when caught) */
#define ECB_SIGTRAP SIGTRAP     /* trace trap (not reset when caught) */
#define ECB_SIGIOT  SIGIOT      /* IOT instruction */
#define ECB_SIGABRT SIGABRT   /* used by abort, replace SIGIOT in the future */
#define ECB_SIGEMT  SIGEMT 	/* EMT instruction */
#define ECB_SIGFPE  SIGFPE  	/* floating point exception */
#define ECB_SIGKILL SIGKILL 	/* kill (cannot be caught or ignored) */
#define ECB_SIGBUS  SIGBUS     	/* bus error */
#define ECB_SIGSEGV SIGSEGV    	/* segmentation violation */
#define ECB_SIGSYS  SIGSYS     	/* bad argument to system call */
#define ECB_SIGPIPE SIGPIPE    	/* write on a pipe with no one to read it */
#define ECB_SIGALRM SIGALRM    	/* alarm clock */
#define ECB_SIGTERM SIGTERM  	/* software termination signal from kill */
#endif

#define ECBPORT_INCL

#endif	/* ECBPORT_INCL */

