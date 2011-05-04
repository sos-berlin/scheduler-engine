#ifndef DBCLNTIPCEX_INCL

#ifndef ECBPORT_INCL
#include "ecbport.h"
#endif
#ifndef ECBIPCDEF_INCL
#include "ecbipdef.h"
#endif

/* Warten auf eine Antwort vom Server */
EXT DLL COUNT warte_auf_server(USER_NO connid,PIDNR clntpid,ECBIPC_CLNTHANDLE_S *clnt_handle);

/* Eine Nachricht an den Server senden */
EXT DLL COUNT freigeben_server(USER_NO connid,PIDNR clntpid,ECBIPC_CLNTHANDLE_S *clnt_handle);

/* der Client sendet ECB_LOGOFF und baut die IPC-Datenstrukturen ab */
EXT DLL COUNT ridmid_ipc(PIDNR clntpid,USER_NO connid,COUNT modus,ECBIPC_CLNTHANDLE_S *clnt_handle);

/* der Client initialisiert die IPC-Schnittstelle und meldet sich am Server an */
EXT DLL COUNT getmids_ipc(UCOUNT apsize,USER_NO connid,USER_NO *apxid,ECBIPC_CLNTHANDLE_S **clnt_handle);

/* der Nachrichtenpuffer des Clients wird auf die richtige Adresse gesetzt */
EXT DLL VOID setze_msgbuf_clnt(NACHRICHT **msg,USER_NO connid,UCOUNT apsize,ECBIPC_CLNTHANDLE_S *clnt_handle);

/* Ueberpruefung, ob der Server aktiv ist oder nicht                       */
EXT DLL COUNT srv_ipc_activ(VOID);

/* Ueberpruefung, ob der Server auf derselben Maschine laeuft              */
EXT DLL COUNT srv_same_machine(VOID);

#define DBCLNTIPCEX_INCL

#endif /* DBCLNTIPCEX_INCL */
