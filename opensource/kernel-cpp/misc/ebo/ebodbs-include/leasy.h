
/*----UEBERSETZUNGSEINHEIT---------------------------------------------------*/
/*									     */
/* Name: <Name der Einheit>						     */
/*									     */
/* Beschreibung: <Erlaeuterung des Inhalts>				     */
/*  									     */
/* Zuvor sind zu includen :                                                  */
/*      leaport.h 							     */
/*	ebomax.h							     */
/*									     */
/* Funktionen: <Aufzaehlung aller enthaltenen Funktionen>		     */
/*									     */
/* Autor: <Name>							     */
/*									     */
/* Historie: <projektspezifische Beschreibung von Erstell- und Aenderungs-   */
/*            daten, Version u.s.w.>					     */
/* 211294-so  neu : CLTR-Modi, EBO_KATALOG_NAME	        		     */
/* 120195-so  OPTR_FILE_S dateimode statt dateinmode     		     */
/* 180998-so  Benutzerfunktionen :                       		     */
/* 040299-so  UINF_S erweitert um zeit                   		     */
/*---------------------------------------------------------------------------*/

/* 
   Schluesselfelder sind !! nicht !!  '\0' terminiert,             12.01.95-so
   da Schluesselteile LOW_VALUE == '\0' enthalten.

   Schluesselwerte werden an den Server immer als zusammenhaengende
   Schluesselfelder uebergeben.
   Der Server muss diese Werte noch "padden".

   Bei Leseauftrag "Lese nur Schluesselfelder" liefert Server den
   gelesenen Schluesselwert ebenfalls als zusammenhaengendes Schluesselfeld
   an den Client zurueck.

   Bei Angabe von Datenbankdateien wird eine Datei immer durch ihren Index
   in der Komponente 'datei_liste' der Struktur 'seg_infos' des
   Shared Memory Segmentes angegeben. Dieser Index entspricht der Nummer
   der Datei im Katalog - 1, da der Index 0 fuer die erste Datei steht.

   Bei variabler Satzlaenge enthaelt der Datensatz in den ersten beiden
   Bytes (S9(4) COMP) die Laenge des Satzes inkl.Satzlaengenfeld.

   Sekundaerindizes werden immer mit dem Primaerindex als letzter
   Schluesselkomponente definiert.

   Bei RDIR ueber Sekundaerindex wird die Komponente des Primaerindex
   im Sekundaerschluesselwert mit 0x00 gefuellt und als ein Schluessel-
   feld an den Server uebergeben. Dieser liest in diesem Fall ueber
   die Datenbank mit dem kompletten Schluesselwert. Wird ein Schluessel
   gefunden, der in den SI-Feldern nicht mit dem uebergebenen Schluessel
   uebereinstimmt, so wird "Satz nicht gefunden" an den Client zurueck
   gegeben.

   Bei DLET wird immer der Primaerschluessel des zu loeschenden Satzes
   an den Server uebergeben.

*/
/*
030899-so Trace-Funktionen 
*/

#include <time.h>		/* 040299-so */
#ifndef LEASY_INCL


#ifndef ECBPORT_INCL
#include "ecbport.h"
#endif
#ifndef ECBIPCDEF_INCL
#include "ecbipdef.h"
#endif

#define MAINITEM 1	/* Primaerindex hat bei ctree immer die lfd. Nummer
			   Datendatei + 1 */

/* die Leasybefehle an den Server */ 
#define EBO_CATD	(ECB_LASTIPCFUNC + 1)
#define	EBO_OPTR	(ECB_LASTIPCFUNC + 2)
#define EBO_CLTR	(ECB_LASTIPCFUNC + 4)
#define EBO_RDIR	(ECB_LASTIPCFUNC + 6)
#define EBO_RHLD	(ECB_LASTIPCFUNC + 7)
#define EBO_RNGE	(ECB_LASTIPCFUNC + 8)    /* groesser gleich */
#define EBO_RNGT        (ECB_LASTIPCFUNC + 9)    /* nur groesser */
#define EBO_RPLE        (ECB_LASTIPCFUNC + 10)   /* kleiner gleich */
#define EBO_RPLT	(ECB_LASTIPCFUNC + 11)   /* nur kleiner */
#define EBO_INSR	(ECB_LASTIPCFUNC + 12)
#define EBO_STOR	(ECB_LASTIPCFUNC + 13)
#define EBO_REWR	(ECB_LASTIPCFUNC + 14)
#define EBO_DLET	(ECB_LASTIPCFUNC + 15)
#define EBO_LOCK	(ECB_LASTIPCFUNC + 16)
#define EBO_UNLK	(ECB_LASTIPCFUNC + 17)
#define EBO_UNLK_ALL	(ECB_LASTIPCFUNC + 18)
#define EBO_LFDNR	(ECB_LASTIPCFUNC + 19)

#define EBO_STATUS	(ECB_LASTIPCFUNC + 20)	/* Client fraegt Serverstatus ab */
#define EBO_SHUTDOWN    (ECB_LASTIPCFUNC + 21)  /* Client schickt SHUTDOWN an Server */
#define LIST_TATAB_USER (ECB_LASTIPCFUNC + 22)	/* Schreiben Transaktionstabelle USER */
#define LIST_TATAB	(ECB_LASTIPCFUNC + 23)  /* Schreiben Transaktionstab. aller USER */
#define LIST_LOCK_DATEI (ECB_LASTIPCFUNC + 24)  /* Locktabelle fuer eine Datei */
#define LIST_LOCK_USER  (ECB_LASTIPCFUNC + 25)  /* Locktabelle fuer einen User */
#define LIST_LOCKTAB    (ECB_LASTIPCFUNC + 26)  /* Locktabelle fuer alle Dateien */

#define DELEBOFILE      (ECB_LASTIPCFUNC + 28)  /* EBO-Datei bei aktivem Server loeschen */

#define EBO_GETSVBLEN   (ECB_LASTIPCFUNC + 29)  /* Client holt Laenge SVB-Tabelle ueber Sockets */
#define EBO_GETSVB      (ECB_LASTIPCFUNC + 30)  /* Client holt SVB-Tabelle ueber Sockets */

/************************************************************************/
/*     ! ! ! !         A C H T U N G  ! ! ! ! !				*/
/* Die Umstellung der ES-Software ist nur mit Einsatz vor Ort und       */
/* einem hohen Arbeitsaufwand moeglich sehr Arbeitsintensiv             */
/* Deshalb sind alle nachfolgenden Server-Auftraege mit einem Offset    */
/* von 100 festgelegt                                                   */
/* #define MAX_FUNC	....      entfaellt                             */

#define OFFSET_ESFKT	32		




#define OFFSET_SRVFKT_NEU 100
/*					neu : Benutzerfunktionen 	     */
#define EBO_UINF	(OFFSET_SRVFKT_NEU + 1)  /* welcher Benutzer hat     */
					         /* Sperre gesetzt 	     */
#define NXT_USERTAB	(OFFSET_SRVFKT_NEU + 2)  /* naechste Benutzerinfo    */
#define LIST_USERTAB	(OFFSET_SRVFKT_NEU + 3)  /* Benutzerinformationfo    */
#define KILL_USER       (OFFSET_SRVFKT_NEU + 4)  /* Benutzer killen/abmelden */
#define TIME_TRACE_ON   (OFFSET_SRVFKT_NEU + 5)  /* Zeittrace einschalten    */
#define FKT_TRACE_ON    (OFFSET_SRVFKT_NEU + 6)  /* Funktionstrace einschalt.*/
#define TRACE_OFF       (OFFSET_SRVFKT_NEU + 7)  /* Trace ausschalten        */

#define EBO_ADM_KENNZ	0x8000		/* Kennzeichen fuer ADM-Auftrag */



/*************  Definitionen zur Messagestruktur			*/
/*      	CLTR-Modi 						*/
#define COMMIT          1	/* Transaktion beenden mit Commit 	*/
				/* ohne Folgetransaktion		*/
#define ROLLBACK        2	/*      "         "     "  Rollback 	*/
				/* ohne Folgetransaktion		*/
#define SAVEPOINT	3	/* Transaktion beenden mit COMMIT	*/
				/* mit Eroeffnen Folgetransaktion	*/
#define UNDO		4       /* Transaktion beenden mit Rollback     */
				/* mit Eroeffnen Folgetransaktion	*/

/* 		msgdata bei OPTR 					*/
/*              es werden maximal MAX_LEA_LIST Elemente uebergeben	*/
typedef struct optrfile {
	COUNT 	dateinr;	/* Nummer in Dateiliste im SHM (Basis 0)*/
	COUNT 	dateimode;	/* SHARED (==0) oder EXCLUSIVE wie von  */
				/* von ctree definiert		 	*/
} OPTR_FILE_S;

/* 		msgdata bei Leseoperationen				*/
typedef struct readkey {
	TEXT	keyval[MAX_EBO_KEYLEN];	    /* Key nicht '\0'-terminiert  */
	COUNT	readmode;			/* Lesemodus 		*/
} READ_KEY_S;
/*              readmode :						*/
#define ALLFIELDS 	0	/* Kompletter Datensatz			*/
#define ONLY_KEYS	1	/* Nur Schluessel			*/

	
/***** Zustaende, in der sich eine geoeffnete EBO Datei befinden kann */
/*  definiert in ctport.h
#define EXCLUSIVE 0x0000
#define SHARED    0x0001
*/
#define CLOSE     0x0002	        /* Datei physikalisch geschlossen */
				        /* bzw. nicht von User i geoeffnet */
#define CORRUPT   0x0003 	/* Datei wegen IO-Error gesperrt */

#define OPEN      0x0004     	/* Benutzer hat Datei eroeffnet */

/***** Datenstruktur Serverstatusbericht */

typedef struct {
	LONG	srqsts,
		    susers,
		    strans,
		    trc,
		    updates,
		    scts_wl,
		    scts_rl,
		    scts_ul,
		    sredo,
		    ssets,
		    swaks,
		    suwaks,
		    sretry;
        COUNT	    shtdwn;
	COUNT	    max_user;
	COUNT       max_msg;
        COUNT       filmods[MAX_EBO_DAT];  /* Dateizustaende */
        TEXT        katname[MAX_EBO_KATNAME+1];
	LONG        anz_ebodateien;
	LONG	    mem_kat_len;
} SSTATUS;


/***** Datenstruktur Benutzerinformation                180998-so */
typedef struct {
        PIDNR   pid;
        TEXT    usr_name[MAX_USER_NAME+1];
        TEXT    log_name[MAX_LOG_NAME+1];
        TEXT    ebo_user[MAX_EBO_USER+1];
        TEXT    host_name[MAX_HOST_NAME+1];
        TEXT    tty_name[MAX_TTY_NAME+1];
	time_t	zeit; 					/* 040299-so */
}UINF_S;



#define LEASY_INCL

#endif /* LEASYINCL */
