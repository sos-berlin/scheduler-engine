#ifndef ECBERROR_INCL

#include "toolsdef.h"

/* Systemfehlercode gematcht auf unten folgenden ECB-Fehlercodes */
/* EXT DLL ECBERROR ecb_errno;		*/

/* entsprechender Fehlertext zu den Fehlercodes */
/* EXT DLL TEXT     ecb_errtxt[40]; */

/* belegt ecb_errno mit dem Systemfehlercode */
/* und    ecb_errtxt mit dem Fehlerkurztext */ 
EXT DLL VOID ecb_errmap(SYSERROR syserr);


/* ecb_errno setzen */
EXT DLL VOID ecb_set_errno(ECBERROR num);

/* ecb_errno abfragen */
EXT DLL ECBERROR ecb_get_errno (VOID);

/* ecb_errtxt setzen */
EXT DLL VOID ecb_set_errtxt (TEXT *text);

/* ecb_errtxt abfragen */
EXT DLL TEXT *ecb_get_errtxt (VOID);


/* Funktion liefert Fehler => ecb_errno auswerten */
#ifdef ECB_ERROR
#undef ECB_ERROR
#endif
#define ECB_ERROR				-1      
 
/* Funktion liefert keinen Fehler => alles ok */
#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR			0

#define ECB_NO_ERROR 0x20000000		 /* no error */

#define ECB_EPERM (ECB_NO_ERROR + 1) 	/* Operation not permitted */
#define ECB_ENOENT (ECB_NO_ERROR + 2) 	/* No such file or directory */
#define ECB_ESRCH (ECB_NO_ERROR + 3) 	/* No such process */
#define ECB_EINTR (ECB_NO_ERROR + 4) 	/* interrupted system call */
#define ECB_EIO (ECB_NO_ERROR + 5) 	/* I/O error */
#define ECB_ENXIO (ECB_NO_ERROR + 6) 	/* No such device or address */
#define ECB_E2BIG (ECB_NO_ERROR + 7) 	/* Arg list too long */
#define ECB_ENOEXEC (ECB_NO_ERROR + 8) 	/* Exec format error */
#define ECB_EBADF (ECB_NO_ERROR + 9) 	/* Bad file descriptor */
#define ECB_ECHILD (ECB_NO_ERROR + 10) /* No child processes */
#define ECB_EAGAIN (ECB_NO_ERROR + 11) /* Resource temporarily unavailable */
#define ECB_ENOMEM (ECB_NO_ERROR + 12) /* Not enough space */
#define ECB_EACCES (ECB_NO_ERROR + 13) /* Permission denied */
#define ECB_EFAULT (ECB_NO_ERROR + 14) /* Bad address */
#define ECB_ENOTBLK (ECB_NO_ERROR + 15)/* Block device required */
#define ECB_EBUSY (ECB_NO_ERROR + 16)  /* Resource busy */
#define ECB_EEXIST (ECB_NO_ERROR + 17) /* File exists */
#define ECB_EXDEV (ECB_NO_ERROR + 18) /* Improper link */
#define ECB_ENODEV (ECB_NO_ERROR + 19) /* No such device */
#define ECB_ENOTDIR (ECB_NO_ERROR + 20) /* Not a directory */
#define ECB_EISDIR (ECB_NO_ERROR + 21) /* Is a directory */
#define ECB_EINVAL (ECB_NO_ERROR + 22) /* Invalid argument */
#define ECB_ENFILE (ECB_NO_ERROR + 23) /* Too many open files in system */
#define ECB_EMFILE (ECB_NO_ERROR + 24) /* Too many open files */
#define ECB_ENOTTY (ECB_NO_ERROR + 25) /* Inappropriate I/O control operation */
#define ECB_ETXTBSY (ECB_NO_ERROR + 26) /* Text file busy */
#define ECB_EFBIG (ECB_NO_ERROR + 27) /* File too large */
#define ECB_ENOSPC (ECB_NO_ERROR + 28) /* No space left on device */
#define ECB_ESPIPE (ECB_NO_ERROR + 29) /* Invalid seek */
#define ECB_EROFS (ECB_NO_ERROR + 30) /* Read only file system */
#define ECB_EMLINK (ECB_NO_ERROR + 31) /* Too many links */
#define ECB_EPIPE (ECB_NO_ERROR + 32) /* Broken pipe */
#define ECB_EDOM (ECB_NO_ERROR + 33) /* Domain error within math function */
#define ECB_ERANGE (ECB_NO_ERROR + 34) /* Result too large */
#define ECB_ENOMSG (ECB_NO_ERROR + 35) /* No message of desired type */
#define ECB_EIDRM (ECB_NO_ERROR + 36) /* Identifier removed */
#define ECB_ECHRNG (ECB_NO_ERROR + 37) /* Channel number out of range */
#define ECB_EL2NSYNC (ECB_NO_ERROR + 38) /* Level 2 not synchronized */
#define ECB_EL3HLT (ECB_NO_ERROR + 39) /* Level 3 halted */
#define ECB_EL3RST (ECB_NO_ERROR + 40) /* Level 3 reset */
#define ECB_ELNRNG (ECB_NO_ERROR + 41) /* Link number out of range */
#define ECB_EUNATCH (ECB_NO_ERROR + 42) /* Protocol driver not attached */
#define ECB_ENOCSI (ECB_NO_ERROR + 43) /* No CSI structure available */
#define ECB_EL2HLT (ECB_NO_ERROR + 44) /* Level 2 halted */
#define ECB_EDEADLK (ECB_NO_ERROR + 45) /* Resource deadlock avoided */

#define ECB_ENOTREADY (ECB_NO_ERROR + 46) /* Device not ready */
#define ECB_EWRPROTECT (ECB_NO_ERROR + 47) /* Write-protected media */
#define ECB_EFORMAT (ECB_NO_ERROR + 48) /* Unformatted media */

#define ECB_ENOLCK (ECB_NO_ERROR + 49) /* No locks available */

#define ECB_ENOCONNECT (ECB_NO_ERROR + 50) /* no connection */
#define ECB_ESTALE (ECB_NO_ERROR + 52) /* no filesystem */
#define ECB_EDIST (ECB_NO_ERROR + 53) /* old, currently unused AIX errno*/

#define ECB_EWOULDBLOCK (ECB_NO_ERROR + 54) /* Operation would block */

#define ECB_EINPROGRESS (ECB_NO_ERROR + 55) /* Operation now in progress */
#define ECB_EALREADY (ECB_NO_ERROR + 56) /* Operation already in progress */

#define ECB_ENOTSOCK (ECB_NO_ERROR + 57) /* Socket operation on non-socket */
#define ECB_EDESTADDREQ (ECB_NO_ERROR + 58) /* Destination address required */
#define ECB_EMSGSIZE (ECB_NO_ERROR + 59) /* Message too long */
#define ECB_EPROTOTYPE (ECB_NO_ERROR + 60) /* Protocol wrong type for socket */
#define ECB_ENOPROTOOPT (ECB_NO_ERROR + 61) /* Protocol not available */
#define ECB_EPROTONOSUPPORT (ECB_NO_ERROR + 62) /* Protocol not supported */
#define ECB_ESOCKTNOSUPPORT (ECB_NO_ERROR + 63) /* Socket type not supported */
#define ECB_EOPNOTSUPP (ECB_NO_ERROR + 64) /* Operation not supported on socket */
#define ECB_EPFNOSUPPORT (ECB_NO_ERROR + 65) /* Protocol family not supported */
#define ECB_EAFNOSUPPORT (ECB_NO_ERROR + 66) /* Address family not supported by protocol family */
#define ECB_EADDRINUSE (ECB_NO_ERROR + 67) /* Address already in use */
#define ECB_EADDRNOTAVAIL (ECB_NO_ERROR + 68) /* Can't assign requested address */

#define ECB_ENETDOWN (ECB_NO_ERROR + 69) /* Network is down */
#define ECB_ENETUNREACH (ECB_NO_ERROR + 70) /* Network is unreachable */
#define ECB_ENETRESET (ECB_NO_ERROR + 71) /* Network dropped connection on reset */
#define ECB_ECONNABORTED (ECB_NO_ERROR + 72) /* Software caused connection abort */
#define ECB_ECONNRESET (ECB_NO_ERROR + 73) /* Connection reset by peer */
#define ECB_ENOBUFS (ECB_NO_ERROR + 74) /* No buffer space available */
#define ECB_EISCONN (ECB_NO_ERROR + 75) /* Socket is already connected */
#define ECB_ENOTCONN (ECB_NO_ERROR + 76) /* Socket is not connected */
#define ECB_ESHUTDOWN (ECB_NO_ERROR + 77) /* Can't send after socket shutdown */

#define ECB_ETIMEDOUT (ECB_NO_ERROR + 78) /* Connection timed out */
#define ECB_ECONNREFUSED (ECB_NO_ERROR + 79) /* Connection refused */

#define ECB_EHOSTDOWN (ECB_NO_ERROR + 80) /* Host is down */
#define ECB_EHOSTUNREACH (ECB_NO_ERROR + 81) /* No route to host */

#define ECB_ERESTART (ECB_NO_ERROR + 82) /* restart the system call */

#define ECB_EPROCLIM (ECB_NO_ERROR + 83) /* Too many processes */
#define ECB_EUSERS (ECB_NO_ERROR + 84) /* Too many users */
#define ECB_ELOOP (ECB_NO_ERROR + 85) /* Too many levels of symbolic links */
#define ECB_ENAMETOOLONG (ECB_NO_ERROR + 86) /* File name too long */
#define ECB_ENOTEMPTY (ECB_NO_ERROR + 87) /* Directory not empty */
#define ECB_EDQUOT (ECB_NO_ERROR + 88) /* Disc quota exceeded */
#define ECB_EREMOTE (ECB_NO_ERROR + 93) /* Item is not local to host */
#define ECB_ENOSYS (ECB_NO_ERROR + 109) /* Function not implemented POSIX */
#define ECB_EMEDIA (ECB_NO_ERROR + 110) /* media surface error */
#define ECB_ESOFT (ECB_NO_ERROR + 111) /* I/O completed, but needs relocation */
#define ECB_ENOATTR (ECB_NO_ERROR + 112) /* no attribute found */
#define ECB_ESAD (ECB_NO_ERROR + 113) /* security authentication denied */
#define ECB_ENOTRUST (ECB_NO_ERROR + 114) /* not a trusted program */
#define ECB_ETOOMANYREFS (ECB_NO_ERROR + 115) /* Too many references: can't splice */
#define ECB_EILSEQ (ECB_NO_ERROR + 116) /* Invalid wide character */
#define ECB_ECANCELED (ECB_NO_ERROR + 117) /* asynchronous i/o cancelled */
#define ECB_ENOSR (ECB_NO_ERROR + 118) /* temp out of streams resources */
#define ECB_ETIME (ECB_NO_ERROR + 119) /* I_STR ioctl timed out */
#define ECB_EBADMSG (ECB_NO_ERROR + 120) /* wrong message type at stream head */
#define ECB_EPROTO (ECB_NO_ERROR + 121) /* STREAMS protocol error */
#define ECB_ENODATA (ECB_NO_ERROR + 122) /* no message ready at stream head */
#define ECB_ENOSTR (ECB_NO_ERROR + 123) /* fd is not a stream */
#define ECB_ENOTSUP (ECB_NO_ERROR + 124) /* POSIX threads unsupported value */
#define ECB_EMULTIHOP (ECB_NO_ERROR + 125) /* multihop is not allowed */
#define ECB_ENOLINK (ECB_NO_ERROR + 126) /* the link has been severed */
#define ECB_EOVERFLOW (ECB_NO_ERROR + 127) /* value too large to be stored in data type */

/* angegebener Name (Zeichenkette) ist ungueltig */
#define ECB_INVALID_NAME (ECB_NO_ERROR + 128)      

/* angegebener Service wurde nicht gefunden */
#define ECB_NO_SERVICE (ECB_NO_ERROR + 129) 

/* angegebener Service laeuft bereits */
#define ECB_SRV_RUNS (ECB_NO_ERROR + 130)

/* angegebene Daten sind ungueltig (Struktur) */
#define ECB_INVALID_DATA (ECB_NO_ERROR) + 131
                 
        
/* bei ecb_open_file() wurde ein ungueltiger OPEN-Modus angegeben */
#define ECB_WRONG_OPNMOD 	(ECB_NO_ERROR + 900)
/* bei ecb_open_file() wurde ein ungueltiger ACCESS-Modus angegeben */
#define ECB_WRONG_ACCMOD 	(ECB_NO_ERROR + 901)
/* bei ecb_open_file() wurde ein ungueltiger SHARE-Modus angegeben */
#define ECB_WRONG_SHARMOD 	(ECB_NO_ERROR + 902)
/* bei ecb_open_file() wurde ein ungueltiger SYNC-Modus angegeben */
#define ECB_WRONG_SYNCMOD 	(ECB_NO_ERROR + 903)    

/* neuer Client hat IPC-Verbindung zu Server aufgebaut */
#define ECB_NEW_CLIENT 		(ECB_NO_ERROR + 904)
/* System kann geforderten Hauptspeicher nicht bereitstellen */
#define ECB_NO_MEMORY		(ECB_NO_ERROR + 905)

/* gelesene Nachrichtenlaenge entspricht nicht der Laenge im Kopf 
   entsprechendes fuer gesendete Nachricht
*/
#define ECB_MISMATCH_MSGLEN	(ECB_NO_ERROR + 906)

/* kein freier Socket fuer den Client gefunden */
#define ECB_NO_SOCKET		(ECB_NO_ERROR + 907)

/* Kommunikationspartner hat Verbindung abgebaut */
#define ECB_IPC_DISCONNECT      (ECB_NO_ERROR + 908)

/* timeout bei warten auf eine Nachricht */
#define ECB_IPC_TIMEOUT		(ECB_NO_ERROR + 909)

/* Fehler bei Aufbereiten des empfangenen Nachrichtenkopfes */
#define ECB_IPC_XDRRCV_ERROR	(ECB_NO_ERROR + 910)

/* Fehler bei Aufbereiten des empfangenen Nachrichtenkopfes */
#define ECB_IPC_XDRSND_ERROR	(ECB_NO_ERROR + 911)

/* Uebergebener Datenpuffer bei XDR zu klein */
#define ECB_BUFFER_TOO_SMALL	(ECB_NO_ERROR + 912)


#define ECBERROR_INCL

#endif /* ECBERROR_INCL */
