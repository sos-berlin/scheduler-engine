/*----UEBERSETZUNGSEINHEIT---------------------------------------------------*/
/*									     */
/* Name: ecbipcex.h       						     */
/*									     */
/*									     */
/* Autor: R. Cramer 					 		     */
/*									     */
/* Historie: <projektspezifische Beschreibung von Erstell- und Aenderungs-   */
/*            daten, Version u.s.w.>					     */
/*									     */
/*---------------------------------------------------------------------------*/
#ifndef ECBIPCEX_INCL

#include "ecbipdef.h"

/* Nachrichtenpuffer */
                           
/* Schliessen der Verbindung zu einem Client */
EXT DLL VOID close_srvclient(COUNT connid);

/* allen Clients ein Endesignal senden */
EXT DLL VOID killipc_all_clients(VOID);

/* Initialisieren der IPC-Datenstrukturen bei Programmstart */  
/* maxusr	max. Anzahl paralleler Verbindungen
   maxmsg	max. Laenge Datenteil einer IPC-Nachricht
   portno	Nummer des Serverports bei Socketkommunikation,
		sonst 0
*/
EXT DLL COUNT init_srvipc(COUNT maxusr, IPCMSGLEN maxmsg, COUNT srvport);

/* Schliessen der IPC-Datenstrukturen bei Programmende */
EXT DLL VOID deinit_srvipc(VOID);                        
                                                              
/* Warten auf eine Nachricht
   connid	Nummer der Verbindung, auf der die Nachricht eingetroffen ist
		0< *connid <= bei init_ipc() angegebener Wert fuer maxusr
   msgbuf	Puffer in den die Nachricht uebergeben wird
		Laenge des Puffers wird nicht ueberprueft, sollte immer dem
		Wert fuer maxmsg+sizeof(NACHRICHT) bei Aufruf von init_ipc()
		 entsprechen
*/
EXT DLL COUNT empfange_clntnachricht(COUNT *connid, TEXT *msgbuf,IPCMSGLEN buflen,COUNT warten);

                                                         
/* Senden einer Nachricht
   connid	Nummer der Verbindung, auf der die Nachricht eingetroffen ist
		0< *connid <= bei init_ipc() angegebener Wert fuer maxusr
   msgbuf	Puffer in dem die zu sendende Nachricht uebergeben wird
		Laenge des Puffers wird nicht ueberprueft,die Nachrichtenlaenge
		wird dem Nachrichtenkopf entnommen
*/
EXT DLL COUNT sende_srvnachricht(COUNT connid, TEXT *msgbuf,IPCMSGLEN buflen, COUNT warten); 



EXT DLL COUNT init_clntipc(ECBIPC_CLNTHANDLE_S **pclnt_handle,IPCMSGLEN maxmsg,COUNT srvport,IPCHOST srvhost);

EXT DLL COUNT strt_clntipc(ECBIPC_CLNTHANDLE_S **pclnt_handle,IPCMSGLEN maxmsg,COUNT srvport,IPCHOST srvhost);

EXT DLL COUNT tst_clntconnect(ECBIPC_CLNTHANDLE_S *clnt_handle,COUNT warten);

EXT DLL COUNT tst_clntipc(ECBIPC_CLNTHANDLE_S **pclnt_handle,COUNT srvport,IPCHOST srvhost);

EXT DLL VOID  deinit_clntipc(ECBIPC_CLNTHANDLE_S *clnt_handle);

EXT DLL COUNT empfange_srvnachricht(ECBIPC_CLNTHANDLE_S *clnt_handle,TEXT *msgbuf,IPCMSGLEN buflen, COUNT warten);

EXT DLL COUNT sende_clntnachricht(ECBIPC_CLNTHANDLE_S *,TEXT *msgbuf,IPCMSGLEN buflen,COUNT warten);

EXT DLL COUNT ecb_same_machine(TEXT *hostname);

EXT DLL VOID  print_msghead(NACHRICHT *);
EXT DLL VOID  print_msgdata(NACHRICHT *);

#define ECBIPCEX_INCL

#endif /* ECBIPCEX_INCL */
