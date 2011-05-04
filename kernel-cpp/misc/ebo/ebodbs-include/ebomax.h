
/*----UEBERSETZUNGSEINHEIT---------------------------------------------------*/
/*									     */
/* Name: ebomax.h          	 					     */
/*									     */
/* Beschreibung: <Erlaeuterung des Inhalts>				     */
/*									     */
/*   Definition der Maximalwerte fuer EBO Datenhaltung unter Unix            */
/*									     */
/* Autor: <Name>							     */
/*									     */
/* Historie: <projektspezifische Beschreibung von Erstell- und Aenderungs-   */
/*            daten, Version u.s.w.>					     */
/* Vxx.x 21.12.94-so :  neu : MAX_EBO_LEASY				     */
/* 180998-so Maximalwerte fuer Benutzerdaten  		 		     */
/*---------------------------------------------------------------------------*/

#ifndef EBOMAX_INCL


#define MAX_EBO_LEASY   1000    /* Anzahl von Leasy zu verwaltender  */
				                /* Dateien und Dateikennungen */
#define MAX_EBO_DAT	    1000    /* Anzahl Dateien in Katalog */
#define MAX_EBO_INDEX	32      /* Anzahl Indizes je Datei */
#define MAX_EBO_SEGMENT	32      /* Anzahl Segmente je Index */
#define MAX_EBO_KEYLEN	256     /* Laenge Schluessel inkl. Primaer */
#define MAX_EBO_DATLEN	4096    /* Laenge Datensatz */
#define MAX_EBO_DATNAM	8       /* Laenge Dateiname ohne .xxx */
#define MAX_EBO_IDXNAME 8	    /* Laenge Name Index */      
#define MAX_EBO_FMLEN	3       /* Laenge Name Folgemerkmal */
#define MAX_EBO_FM	10          /* Anzahl Folgemerkmal je Datei */

#define MAX_EBO_DB_PFAD	127	    /* max. Laenge Datenbankpfad */
#define MAX_EBO_KATNAME 140     /* max. Laenge Katalogname inkl. Pfad */

#define EBO_DAT_XDT	MAX_EBO_DATLEN	/* Erweiterung Datendatei */
#define EBO_IDX_XDT     2048		/* MAX_EBO_KEY * 10 gerundet */

#define MAX_EBO_DIR	128		/* max. Pfadlaenge in EBO */

#define MAX_EBO_MSGLEN  4096            /* max. Laenge Nachricht EBO <-> SRV */

#define MAX_COPY_NAME   8      /* max. Laenge COPY-Strecke EBO-Datei => odbc */


/*                              Maximalwerte fuer Benutzerdaten 180998-so */
#define MAX_USER_NAME   8               /* Benutzername 	*/
#define MAX_LOG_NAME    8               /* Login-Name   	*/
#define MAX_EBO_USER    8               /* EBO-Benutzer 	*/
#define MAX_HOST_NAME   32              /* Host-Name   		*/
#define MAX_TTY_NAME    32              /* Geraete-Name	   	*/


  

#define EBOMAX_INCL

#endif /* EBOMAX_INCL */
