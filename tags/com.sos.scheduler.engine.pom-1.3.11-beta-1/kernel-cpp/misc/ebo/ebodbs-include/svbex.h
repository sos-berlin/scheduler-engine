/*----HEADER-----------------------------------------------------------------*/
/*									     */
/* Name: svbex.h       		          				     */
/*									     */
/* Inhalt: externe Funktionen des Moduls shmkat.c  			     */
/*									     */
/* Autor: B.Sommerfeld							     */
/*									     */
/* Historie:                                                                 */
/* V00.0 xx.01.95-so Erstellung					             */
/*---------------------------------------------------------------------------*/

#ifndef SVBEX_INCL

#include "svbdef.h"

EXT DLL LONG compute_svblen(USER_NO maxusr,IPCMSGLEN maxmsg,LONG anz_dateien,LONG memkatlen);

EXT DLL LONG get_svblen(VOID);

EXT DLL COUNT setze_svb_srv(TEXT *svb_start,USER_NO maxusr,IPCMSGLEN maxmsg,NINT anz_ebofiles,LONG memkatlen);

EXT DLL COUNT setze_svb_clnt(TEXT *svb_start);

EXT DLL COUNT alloc_svb(LONG len, TEXT **start);
EXT DLL COUNT free_svb(VOID);

EXT DLL TEXT *get_svb_start(VOID);

/*     NO_ERROR       Datei-    key-    Zeigerablage 		*/
/*     ECB_ERROR		index    index 	             		*/
EXT DLL COUNT get_zeiger(COUNT , COUNT ,SVB_ZEIGER_S *); 

/*	NO_ERROR	(Dateinr im Katalog, Nummer Index bzgl. Datei,
			 Zeiger auf errechnete Schluessellaenge)
	ECB_ERROR					
*/
EXT DLL COUNT get_keylen(COUNT, COUNT, COUNT *);

/*	NO_ERROR	(Dateinr im Katalog, Nummer Index bzgl. Datei,
			 Zeiger auf errechnete ctree Dateinummer)
	ECB_ERROR
*/
EXT DLL COUNT get_ctree_filno(COUNT, COUNT, COUNT*);

/*
	COUNT	Nummer der Datei im Katalog
	COUNT * Datensatztyp (FIXED, VLENGTH)

	NO_ERROR, ECB_ERROR
*/
EXT DLL COUNT get_rectype(COUNT, COUNT*);

/*
	COUNT	Nummer der Datei im Katalog
	COUNT * Datensatztyp (FIXED, VLENGTH)
	COUNT * Recordlaenge

	NO_ERROR, ECB_ERROR
*/
EXT DLL COUNT get_rtype_rlen(COUNT, COUNT*, COUNT*);

/*
	COUNT	Nummer der Datei im Katalog
	COUNT * Minimale Satzlaenge           

	NO_ERROR, ECB_ERROR
*/
EXT DLL COUNT get_minlen(COUNT, COUNT*);

/*
	COUNT	Nummer der Datei im Katalog
	COUNT * Anzahl Indices                

	NO_ERROR, ECB_ERROR
*/
EXT DLL COUNT get_anz_idx(COUNT, COUNT*);

/*     NO_ERROR          Datei-    key-    Datenablage   		*/
/*     ECB_ERROR		   index    index 	             		*/
EXT DLL COUNT get_all_ctree(COUNT , COUNT ,ALL_CTREE_S *); 


/*      NO_ERROR       Dateiname Dateiindex            			*/
/*	ECB_ERROR          							*/
EXT DLL	COUNT get_shmkat(TEXT* , COUNT*);

/*      NO_ERROR       + Dateiindex, -Dateiname            			*/
/*	ECB_ERROR          							*/
EXT DLL COUNT get_ebofil_filename(COUNT, TEXT *);

/*      NO_ERROR       + Dateiindex, -Dateiname           			*/
/*	ECB_ERROR          							*/
EXT DLL COUNT get_ebofil_kurzname(COUNT, TEXT *);

EXT DLL COUNT set_ebofil_kurzname(COUNT, TEXT *);

/*      NO_ERROR       + Dateiindex, -IFIL-Zeiger            			*/
/*	ECB_ERROR          							*/
EXT DLL COUNT get_ebofil_ifilptr(COUNT, pIFIL *);

/*      NO_ERROR       + Dateiindex, -IFIL-Zeiger            			*/
/*	ECB_ERROR          							*/
EXT DLL COUNT set_ebofil_ifilptr(COUNT, IFIL *);

/* 	NO_ERROR     Datei-   SI-Name  key-Index		        */
/*      ECB_ERROR         index						*/
EXT DLL	COUNT get_si(COUNT, TEXT*, COUNT*);

/*     NO_ERROR        wohin  woher  Laenge Laenge  Datei-  key-    YES/NO 	*/
/*     ECB_ERROR		             wohin  woher   index         	*/
EXT DLL COUNT mk_bufkey(TEXT*, TEXT*, COUNT, COUNT , COUNT, COUNT, COUNT);

/*     NO_ERROR        wohin  woher Datei-  key-    Satzlaenge		*/
/*     ECB_ERROR		            index   index         		*/
EXT DLL COUNT mk_arkey(TEXT*, TEXT*, COUNT , COUNT, COUNT);    


EXT DLL COUNT set_ebofil_ifilptr(COUNT, IFIL *);

EXT DLL LONG get_katlen(VOID);

EXT DLL COUNT setze_katlen(LONG);

EXT DLL COUNT get_katname(TEXT *);

EXT DLL LONG get_ebo_anzdateien(VOID);

EXT DLL COUNT get_ebo_maxusr(VOID);

EXT DLL COUNT get_ebo_maxmsg(VOID);

EXT DLL COUNT lade_svbkat(TEXT *,TEXT *,USER_NO,COUNT,LONG,LONG *,LONG *);


#define SVBEX_INCL

#endif /* SVBEX_INCL */
