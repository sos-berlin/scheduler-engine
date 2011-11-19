
/*----UEBERSETZUNGSEINHEIT---------------------------------------------------*/
/*									     */
/* Name: katfuncex.h      					     */
/*									     */
/* Beschreibung: <Erlaeuterung des Inhalts>				     */
/*	EXTERN-Deklarationen Verarbeitung Katalogdatei			     */
/*									     */
/* Funktionen: <Aufzaehlung aller enthaltenen Funktionen>		     */
/*									     */
/* Autor: R.Cramer							     */
/*									     */
/* Historie: <projektspezifische Beschreibung von Erstell- und Aenderungs-   */
/*            daten, Version u.s.w.>					     */
/*									     */
/*---------------------------------------------------------------------------*/
#ifndef KATFUNCEX_INCL

#include "katfuncdef.h"

EXT DLL COUNT create_catalog(TEXT *,COUNT);    /* angegebenen Katalog erzeugen */
EXT DLL COUNT open_catalog(TEXT * ,LONG *);    /* Katalogname inkl. Pfad, Anzahl Dateien im Katalog */
					    
EXT DLL COUNT close_catalog(VOID);              /* schliesst aktuell eroeffneten Katalog */

EXT DLL COUNT rebuild_catalog(TEXT *);          /* Wiederaufbau Katalog(Pfad/Name) */

EXT DLL COUNT read_catentry(TEXT *, KATENTRY_IFIL_S *); /* EBO Dateiname ohne Pfad !  */
EXT DLL COUNT read_first_catentry(KATENTRY_IFIL_S *); /* EBO Dateiname ohne Pfad !  */
EXT DLL COUNT read_next_catentry(TEXT *, KATENTRY_IFIL_S *); /* EBO Dateiname ohne Pfad ! */

/* EXT DLL POINTER read_first_catprimkey(TEXT *);   1.  Dateiname im Katalog */
/* EXT DLL POINTER read_next_catprimkey(TEXT *,TEXT *); folgender Dateiname im Katalog */

EXT DLL COUNT insert_catentry(KAT_SATZ *); /* einfuegen Kateintrag aus IFIL */
EXT DLL COUNT delete_catentry(TEXT *);   /* loeschen Åber EBO Dateiname */

EXT DLL COUNT bestimme_katalogname(TEXT *,TEXT *,TEXT *); /*bestimmt Katalogname (Pfad/Name) */
EXT DLL COUNT zerlege_katalogname(TEXT *,TEXT *, TEXT *); /* Pfad/Name,Pfad,Name*/
EXT DLL COUNT lies_katalog(TEXT *liste, LONG anz_dateien);  

#define KATFUNCEX_INCL

#endif /* KATFUNCEX_INCL */
