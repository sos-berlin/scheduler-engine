#ifndef DBSRVIPCEX_INCL

/* Loescht IPC-Schnittstelle */
EXT DLL COUNT loesche_ipc(VOID);

/* Loescht Verbindung zu einem bestimmten Client */
EXT DLL COUNT loesche_client(COUNT connid,PIDNR pid);

/* Sendet Ende-Signal an einen Client,falls dieser auf derselben Maschine ist*/
EXT DLL COUNT kill_client(USER_NO connid, PIDNR pid, TEXT *host);


/* Sendet an alle Clients ein Signal zur Beendigung der Clients */
EXT DLL VOID kill_all_clients(VOID);


/* Setzet den Nachrichtenpuffer fuer den angegebenen Client */
EXT DLL VOID setze_msgbuffer(TEXT **msg,USER_NO conn_id);

/* Anzahl zu eroeffnender Verbindungen (einschliesslich Serviceverbindungen */
EXT DLL COUNT get_ipc_users(COUNT service);

/* Anzahl zu eroeffnender Verbindungen (einschliesslich Serviceverbindungen */
EXT DLL COUNT get_ebo_maxuidx(VOID);


/* Initialisiert die IPC-Schnittstelle */
EXT DLL COUNT make_ipc(COUNT maxusr, IPCMSGLEN maxmsg, LONG svb_len, TEXT **svb_start);

/* Sendet den SVB an den angegebenen Client */
EXT DLL COUNT sende_svb_toclnt(USER_NO connid,TEXT *svb_start,IPCMSGLEN svb_len);

/* Wartet auf eine Nachricht von irgendeinem Client */
EXT DLL COUNT warte_auf_auftrag(USER_NO *connid);

/* Sendet Antwort an den angegebenen Client */
EXT DLL COUNT sende_srvantwort(USER_NO connid,PIDNR locmid);

#define DBSRVIPCEX_INCL

#endif /* DBSRVIPCEX_INCL */

