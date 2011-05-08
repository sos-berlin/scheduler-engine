#ifndef DBERROR_INCL

EXT DLL COUNT uerr_cod;
EXT DLL COUNT isam_err;
EXT DLL COUNT Uerr_fil;

#define SRV_NOT_STARTED         600     /* Server laeuft nicht               */
#define SHUTDOWN_ERROR		601     /* Anfrage wegen SHUTDOWN Server     */
					/* abgelehnt			     */
#define PID_CHANGED		602     /* PID auf Verbindung != PID Auftrag */
#define SERVER_LOCKED		603     /* Server fuer EBO Benutzer gesperrt */
#define MSGSIZE_TOO_SMALL       604     /* Datensatz laenger als Nachrichten-*/
					/* puffer			     */
#define NO_LICENSE		605     /* Keine freie Lizenz moeglich !     */
#define CLIENT_LOGOFF		606     /* Client hat Verbindung abgebaut */
#define SRV_ALR_STARTED         607     /* Server fuer diese Kennung bereits */
					/* aktiv */

#define TA_ACTIVE		608     /* Anfrage abgelehnt, weil Trans-    */
					/* aktion aktiv                      */
#define NO_TA_ACTIVE		609

#define WRONG_SHUTVAL		610

/****************************** Wertebereiche Serveraufrufparameter **********/
#define USRNO_TOO_LARGE		611	/* Anzahl Benutzer >  SHRT_MAX       */
#define FILNO_TOO_LARGE		612	/* Anzahl ctree-Dateien > SHRT_MAX   */
#define LOCKTAB_TOO_LARGE	613	/* Gr”sse Sperrtabelle               */
#define TATAB_TOO_LARGE         614     /* Gr”sse transaktionstabelle        */
#define LOGRECNO_TOO_LARGE      615     /* Anzahl Updatesaetze               */
#define BUFNO_TOO_LARGE         616     /* Anzahl Indexpuffer                */
#define SHMKAT_TOO_LARGE        617     /* L„nge SHM-Katalog                 */

#define UNKNOWN_LEASYOP		618	/* unbekannter Operationscode	 */
#define RECTYPE_NOT_DEFINED     619     /* unzulaessiger Satztyp */
#define NULL_RECLEN		620	/* uebergebene Datensatzlaenge == 0 */
#define NULL_KEYLEN		621	/* uebergebene Schluessellaenge == 0 */
#define RECLEN_TOO_SMALL	622     /* Satzlaenge < 5 */
#define RECLEN_TOO_BIG          623     /* Satzlaenge > MAX_EBO_DATLEN  */
#define KEYLEN_TOO_BIG          624     /* angegebene Schluessellaenge */
					/* > MAX_EBO_KEYLEN */
#define KEY_ALR_EXISTS		625	/* Schluessel existiert bereits    */
#define KEY_NOT_FOUND		626	/* Schluessel nicht gefunden       */
#define NO_MEMORY		627     /* kleinerer malloc() fehlgeschlagen */
#define DBS_NO_MEMORY           NO_MEMORY


/****  D A T E I - V E R W A L T U N G *************************************/

#define UNKNOWN_FILOPNMOD       630     /* unbekannter OPEN-Modus bei OPTR */
#define FIL_OPNMOD_ERROR        631     /* Datei konnte im gewuenschten  */
					/* Modus nicht eroeffnet werden  */
#define FIL_ALROPN_ERROR        632	/* Datei bereits von Anwendung   */
					/* eroeffnet, Progr.fehler Leasy */
#define FIL_CORRUPT		633	/* IO_ERROR bei Datei, s. uerr_cod */
#define FIL_NOT_OPN_ERROR	634	/* Benutzer hat Datei n. eroeffnet */
#define CLFL_ERROR		635	/* CLFL gescheitert		   */
#define READ_MOD_ERROR          636     /* Lesemodus falsch              */

/************************* Verwaltung einer laufenden Nummer *****************/

#define TOO_BIG_FOR_LFDNR       638     /* Schluessellaenge zu gross fuer    */
					/* Funktion EBO_LFDNR                */
#define OVERFLOW_LFDNR          639     /* Ueberlauf laufende Nummer         */


/*************************  S P E R R - V E R W A L T U N G ******************/

#define KEY_ALR_LOCKED		640	/* Primaerschluesselwert bereits von */
					/* anderem Benutzer gesperrt */
#define DEL_USER_LOCKS_ERROR    641     /* Fehler bei Loeschen aller Sperren */
					/* eines Benutzers 		     */
#define KEY_LOCK_MOD            642     /* Primaerschluessel in Transaktion  */
                                        /* geaendert.			     */
#define KEY_NOT_LOCKED          643     /* Primaerschluessel war nicht       */
                                        /* (durch den user) gesperrt         */
#define LOCKTAB_VOLL            644     /* Sperrtabelle voll                 */
#define NO_LOCKTAB_MEMORY       645     /* Speicherplatzanforderung          */
                                        /* Sperrtabelle fehlgeschlagen       */
#define LOCK_NOT_FOUND          646     /* keine Sperre fuer diesen Primaer- */
					/* schluessel vorhanden              */

/******** T R A N S A K T I O N S - V E R W A L T U N G **********************/

/*** Transaktionstabelle */
#define NO_TATAB_MEMORY 	650	/* Speicherplatzanforderung TATAB
					   fehlgeschlagen		   */
#define NO_USER_AIM_MEMORY	651	/* Speicherplatzanforderung USER_AIM
					   fehlgeschlagen		   */
#define TATAB_VOLL		652	/* Transaktionstabelle voll */
#define CORRUPT_TAENTRY         654     /* unstimmiger TATAB-Eintrag       */
#define UNKNOWN_READDIR         655     /* unbekannte Leserichtung auf TATAB*/
#define TA_RELEASE_ERROR        656     /* bereits freigegebener Speicher- */
					/* block soll freigegeben werden   */
/*** LOG-Datei */
#define TRAN_NOT_SET		660	/* Umgebungsvariable TRAN ist nicht
					   gesetzt			     */
#define LOGFILE_OPN_ERROR	661	/* Fehler beim Eroeffnen LOG-Datei   */
#define LOGFILE_CLOSE_ERROR	662	/* Fehler beim Schliessen LOG-Datei  */
#define LOGFILE_TRUNC_ERROR	663	/* Fehler beim Positionieren in LOG-
					   datei			     */
#define LOGFILE_REQPOS_ERROR	664	/* Fehler beim Erfragen der Datei-   */
					/* zeigerposition der LOG-datei      */
#define NO_MORE_LOGENTRY	665	/* kein weiterer Eintrag in LOG-Datei*/
#define LOGFILE_WRITE_ERROR	666	/* Fehler beim Schreiben in LOG-Datei*/
#define LOGFILE_ERROR		667	/* Fehler bei Arbeiten mit LOG-Datei */
#define LOGFILE_SETPOS_ERROR    668     /* Fehler bei Positionieren in LOG   */
#define LOGFILE_DEL_ERROR       669     /* Fehler bei Loeschen LOG-Datei     */
#define LOGFILE_READ_ERROR      670     /* Fehler bei Lesen LOG-Datei        */
#define NO_LOGFILE		671     /* keine LOG-Datei gefunden          */
#define CORRUPT_LOG		672	/* inkonsistente LOG-Datei bei wieder*/
#define LOGNO_TOO_LARGE		673     /* Nummer neuer LOG-Datei > SHRT_MAX */
#define LOGFILE_ORDER		674	/* Sortierung LOG-Dateien fehlerhaft */


#define REBUILD_ERROR           680     /* Fehler bei Rebuild Datenbank      */
#define ZENTRALOPEN_ERROR       681     /* Fehler bei Zentralopen            */

/*** CLTR-Phase */
#define COMMIT_REWR_NO_MEMORY   682	/* kein Hauptspeicher fuer REWR Commit*/
#define TA_LOGIC_ERROR		683	/* logischer Fehler in TA-Verarbeitung*/
#define CLTR_MOD_ERROR		684	/* Falscher Modus bei CLTR	      */
#define CLTR_ERROR		685     /* CLTR gescheitert		      */


			/* Laden des Kataloges in SHM */
#define KEIN_KATPFAD        	690
#define KEIN_KATNAME        	691
#define KAT_OPN_ERROR      	692
#define READ_KEY_ERROR      	693
#define KAT_READ_ERROR      	694
#define TOO_MUCH_INDICES        695
#define KAT_TAB_VOLL        	696
#define KAT_ANZ_ERROR       	697
#define SI_NAME_ZU_LANG     	698
#define KATNAM_TOO_LONG		699
#define TOO_MUCH_SEGMENTS	701
#define KAT_CLOSE_ERROR		702
#define KATALOG_KORRUPT         703
#define IPC_ERROR		704
#define CTFIL_ANZ_ERROR         705

			/* SHM-Katalog-Zugriffe  */
#define SHMKAT_ERROR            710     /* Dateinummer,Schluesselindex */
#define SHMFIL_NOTFOUND         711     /* Dateiname im Katalog nvor       */
#define SHMSI_NOTFOUND          712     /* SI-name nicht gefunden           */
#define KEYBUF_ERROR            713     /* Buffer fuer Schluesselaufbereitung*/
			                /*  kleiner als Schluessel-Laenge   */
#define SI_NOT_INCLUDED		714	/* Datensatz zu klein fuer SI       */

/*                     Benutzer-Funktionen                       	     */
#define USER_NOT_FOUND		800	/* Benutzer nicht gefunden  	     */
#define USER_NO_TOO_LARGE	801 	/* Benutzernummer zu lang   	     */
#define USER_KILL_ERR           802     /* Fehler beim killen eines Benutzers */
#define NO_MORE_USER   		803	/* Ende der Benutzertabelle          */
#define NO_USRFKT		804     /* keine Benutzerfumktion            */


/******************** Fehlercodes in IPC *******************************/
/*
#define	IPC1_ERROR		900
#define IPC2_ERROR		901
#define MSQ1_ERROR		902
#define MSQ2_ERROR		903
#define MSQ3_ERROR		904
#define MSQ4_ERROR		905
#define SHM1_ERROR		906
#define SHM2_ERROR		907
#define SHM3_ERROR		908
#define SHM4_ERROR		909
#define SEM1_ERROR		910
#define SEM2_ERROR		911
#define SEM3_ERROR		912
#define WSRV_ERROR		913
#define FSRV_ERROR		914
#define RQST_ERROR		915
#define RSPN_ERROR		916
#define WEBO_ERROR		917
#define FEBO_ERROR		918
#define DEMON_ERROR		919
*/

#define DBS_IPCINIT_ERROR	900
#define DBS_IPCSND_ERROR	901
#define DBS_IPCRCV_ERROR	902
#define DBS_EBODBS_PORT_NOTSET	903
#define DBS_EBODBS_HOST_NOTSET	904
/* Clientnummer,Verbindungsnummer und PID Client inkonsistent in bei Senden
   Der Serverantwort an den Client 
*/
#define DBS_RSPN_ERROR		905

/* Bei getmids() wurde ein falscher Wert fuer Parameter connect_id angegeben */
#define DBS_WRONG_CONNECT_ID    906  
#define DBS_DEMON_ERROR		907
#define DBS_NOIPCBUF		908
/* Client mittlerweile verstorben					*/
#define CLIENT_IS_DEAD		909 

/************************ Fehlermakros Verzeichnisfunktion LOG-Datei ***/
#define OPNDIR_ERROR		930
#define READDIR_ERROR		931
#define CLSDIR_ERROR		932
#define FILSTAT_ERROR		933


#define DBERROR_INCL

#endif /* DBERROR_INCL */
