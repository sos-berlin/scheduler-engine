#ifndef FILEIOEX_INCL

#ifndef FILEPORT_INCL
#include "fileport.h"
#endif

/* Eroeffnen einer Datei
   IN	TEXT *	dateiname
   OUT	pRNDFILE erzeugter Dateideskriptor/-handle
   IN	COUNT  	openmode
   IN	COUNT	accmode
   IN	COUNT 	sharemode
   IN	COUNT	syncmode
   liefert ECB_ERROR im Fehlerfall zurueck, sonst NO_ERROR
*/
EXT DLL COUNT ecb_open_file(TEXT *,pRNDFILE,COUNT,COUNT,COUNT,COUNT);

/* Schliessen einer Datei
   IN 	RNDFILE	Dateideskriptor
*/	
EXT DLL COUNT ecb_close_file(RNDFILE fd);

/* Existiert Datei ?    
   IN 	TEXT *	dateiname
*/
EXT DLL BOOL ecb_file_exists(TEXT *filename);

/* Loeschen einer Datei
   IN 	TEXT *	dateiname
*/
EXT DLL COUNT ecb_delete_file(TEXT *filename);

/* Umbenennen einer Datei
   IN 	TEXT *	aktueller Name der Datei
   IN	TEXT *	neuer Name der Datei
*/
EXT DLL COUNT ecb_rename_file(TEXT *source, TEXT *target);

/* Positionieren in einer Datei
   IN 	RNDFILE	Dateideskriptor
   IN	TEXT *	Dateiname
   IN 	FSIZE	relativer Offset in Bytes
   IN	FSEEK	seekmode in dem positioniert werden soll
*/
EXT DLL FSIZE ecb_seek_file(RNDFILE,TEXT *,FSIZE,FSEEK);

/* Lesen aus einer Datei
   IN 	RNDFILE	Dateideskriptor
   IN 	VOID *	Puffer in den gelesen wird
   IN   FBUFSIZ Anzahl der zu lesenden Bytes 
   liefert ECB_ERROR im Fehlerfall zurueck, ansonsten Anzahl gelesener Bytes 
*/
EXT DLL FNOBYTES ecb_read_file(RNDFILE,VOID *,FBUFSIZ);

/* Schreiben in eine Datei
   IN 	RNDFILE	Dateideskriptor
   IN 	VOID *	Puffer aus dem geschrieben wird
   IN   FBUFSIZ Anzahl der zu schreibenden Bytes
   liefert ECB_ERROR im Fehlerfall zurueck, ansonsten Anzahl geschriebener Bytes
*/
EXT DLL FNOBYTES ecb_write_file(RNDFILE,VOID *,FBUFSIZ);

/* Schreiben der Dateipuffer auf Platte 
   IN 	RNDFILE	Dateideskriptor
*/
EXT DLL COUNT ecb_sync_file(RNDFILE fd);
  

/* String aus einer Datei lesen */
EXT DLL COUNT ecb_fgets (TEXT *string,COUNT n,RNDFILE fd);


/*
//access: 0 = shared, 1 = exclusive
// wait:   0 = nowait, 1 = wait

EXT DLL COUNT ecb_lock_file (RNDFILE fd,COUNT access,COUNT wait);
*/

#define FILEIOEX_INCL

#endif /* FILEIOEX_INCL */
