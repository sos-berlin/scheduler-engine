
/*----UEBERSETZUNGSEINHEIT---------------------------------------------------*/
/*									     */
/* Name: svbdef.h          						     */
/*									     */
/* Beschreibung: Strukturdefinitionen fuer die shmkat-Funktionen	     */
/*									     */
/* Autor: B.Sommerfeld							     */
/*									     */
/* Historie:                                                                 */
/*									     */
/*---------------------------------------------------------------------------*/
#ifndef SVBDEF_INCL

#ifndef ctifilH
#include "ctifil.h"
#endif

typedef struct {
	IIDX *iidx_zeiger;
	IFIL *ifil_zeiger;
	ISEG *iseg_zeiger;
}SVB_ZEIGER_S;

typedef struct {
	COUNT datei;		/* Aufrufparameter wird uebernommen */
	COUNT datno;		/* Filenummer Datendatei 	*/
	COUNT rectype;		/* Recordtyp : FIXED / VLENGTH	*/
	COUNT reclen;		/* Recordlaenge                 */
	COUNT prkeylen;		/* Laenge Primaerschluessel     */
	COUNT indexnr;		/* Indexnummer wird uebernommen */
	COUNT filno;		/* Filenummer Indexdatei 	*/
	COUNT keylen;		/* Schluessellaenge zu Index	*/
}ALL_CTREE_S;
                        
typedef struct {
   IFIL   *ifil_zeiger;	   /* Startadresse IFIL Struktur in SHM */
   TEXT   kurzname[MAX_EBO_DATNAM + 1];  /* Dateiname + '\0' */
   TEXT   name[MAX_EBO_DB_PFAD + MAX_EBO_DATNAM + 2]; /* Pfad + Dateiname + '/' + '\0' */
} KAT_DATEI_INFO_S;


/* Nachrichtenstruktur fuer Synchronisation zwischen EBO und Server */
typedef struct msgstructure {
    LONG  msgtype;    /* Nachrichtentyp. Unter UNIX Prozess-ID (immer Client) */
    COUNT connect_id; /* Nummer Verbindung zwischen EBO und Server */
} MSGSTRUCT_S;


/* Verwaltungsinformation Shared Memory Segmente */
typedef struct svb_orga_seg {
  COUNT   max_users;		              /* max. Anzahl Benutzer */
  COUNT   max_msgsiz;		              /* max. Laenge Nachricht */
  TEXT    katname[MAX_EBO_KATNAME + 1];   /* Katalogname + '\0' */
  LONG    mem_kat_len;		              /* benutzte Laenge Katalog */
  LONG    anz_ebo_dateien; 		          /* Anz. Dateien im Katalog */
  LONG    ausrmid_offset;	   	          /* Offset Start Verbindungstabelle */
  LONG    msg_offset;  		              /* Offset Start Nachrichtentabelle */
  LONG	  datei_liste_offset;             /* Offset Start Dateiliste */
  LONG	  kat_start_offset;               /* Offset Start Katalog */
  LONG	  kat_ende_offset;		          /* Offset Ende Ende Katalog */
  TEXT *  srv_svb_start;                  /* Startadresse im Serverprozess fuer SVB => Offset */
} SVBORGASEG_S;


typedef struct svb_info {
     IPCKEY            ipc_key_1;    /* IPC-Key 1 */
     IPCKEY            ipc_key_2;    /* IPC-Key Nachrichtensegment */
     NINT              shm_id;       /* Systemkennung Shared Memory */
     SVBORGASEG_S     *svb_start;   /* Startadresse Speicherverwaltungsblock */
} SVBINFO_S;


#define SVBDEF_INCL

#endif /* SVBDEF_INCL */
