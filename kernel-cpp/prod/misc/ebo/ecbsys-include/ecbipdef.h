
/*----UEBERSETZUNGSEINHEIT---------------------------------------------------*/
/*									     */
/* Name: <Name der Einheit>						     */
/*									     */
/* Beschreibung: <Erlaeuterung des Inhalts>				     */
/*									     */
/* Autor: <Name>							     */
/*									     */
/* Historie: <projektspezifische Beschreibung von Erstell- und Aenderungs-   */
/*            daten, Version u.s.w.>					     */
/*									     */
/*---------------------------------------------------------------------------*/
#ifndef ECBIPCDEF_INCL

#define MAX_IPCMSGLEN	4096  /* max. Datenlaenge einer Nachricht in Bytes */

typedef struct ecb_message {/* message header structure			*/
	LONG	mpntr;	/* Process number of client                     */
	LONG	mvlen;	/* variable length count			*/
	COUNT   musrn;	/* user number					*/
	COUNT   mfiln,	/* c-tree file number				*/
		mfunc,	/* function number				*/
		mdlen,	/* byte count for mdata buffer			*/
		merrn,	/* c-tree error code				*/
		mretc;	/* return code					*/
      IPCMSGLEN mseqn;	/* length of data without header		*/ 
      IPCHOST   mhost;  /* Name des Clientrechners im Netz */
} MESSAGE;

typedef struct nachricht {
	MESSAGE kopf;
	TEXT	daten[MAX_IPCMSGLEN];
} NACHRICHT;
/* 
MSGBUF muss vor Einbinden dieses Headers als Makro auf die entsprechende
Threadglobale Variable gesetzt werden, die auf den Nachrichtenpuffer des
Threads zeigt
*/

/*
#define CLIENT_PID	MSGBUF->kopf.mpntr
#define MVLEN		MSGBUF->kopf.mvlen
#define AUFTRAG		MSGBUF->kopf.mfunc
#define CLIENT  	MSGBUF->kopf.musrn
#define DATEI 	 	MSGBUF->kopf.mfiln
#define DATALEN 	MSGBUF->kopf.mdlen
#define ERRNO		MSGBUF->kopf.merrn
#define RETURN  	MSGBUF->kopf.mretc
#define MSGLEN      MSGBUF->kopf.mseqn
#define HOST		MSGBUF->kopf.host
#define DATEN		MSGBUF->daten
*/

typedef struct ipc_clnthandle {
	/* Socketnummer Client nach socket() */
	ECBSOCKET	destSock;
	/* max. Laenge einer Clientnachricht */
	IPCMSGLEN	apSize;
    /* Zeiger auf Threadlokalen Nachrichtenpuffer */
    NACHRICHT * msgbuf;
} ECBIPC_CLNTHANDLE_S;


#define ECB_IPC_NOWAIT	0
#define ECB_IPC_WAIT	-1

/* alle Sockets im select() ueberwachen */
#define IPC_SELECT_ALL	-1

/* nur den Serversocket im select() ueberwachen */
#define IPC_SELECT_SRV	0

/* einen bestimmten Client im select() ueberwachen => >0 */

/* Auftragsnummer fuer Clientlogon an Server bei IPC */
#define ECB_LOGON	1

/* Auftragsnummer fuer Abmelden Client */
#define ECB_LOGOFF      2

/* letzte Auftragsnummer fuer alle IPC Anwendungen der ECB */
#define ECB_LASTIPCFUNC 2


#define ECBIPCDEF_INCL

#endif /* ECBIPCDEF_INCL */
