/*----UEBERSETZUNGSEINHEIT---------------------------------------------------*/
/*									     */
/* Name: leaport.h         						     */
/*									     */
/* Beschreibung: zu portierende Datentypen 				     */
/*									     */
/*		MUSS VOR CTPORT.H mit #include eingebunden werden !	     */
/*									     */
/* Funktionen: keine                                     		     */
/*									     */
/* Autor: <Name>							     */
/*									     */
/* Historie: <projektspezifische Beschreibung von Erstell- und Aenderungs-   */
/*            daten, Version u.s.w.>					     */
/*									     */
/*---------------------------------------------------------------------------*/

#ifndef LEAPORT_INCL

#ifndef EBOMAX_INCL
#include "ebomax.h"
#endif

#ifndef ECBPORT_INCL
#include "ecbport.h"
#endif

#ifndef FILEPORT_INCL
#include "fileport.h"
#endif

typedef short       COB_SL;         /* Satzlaengenfeld COBOL */
typedef short		USER_NO;	/* max. Anzahl Benutzer */

typedef TEXT EBO_SI_NAME[MAX_EBO_IDXNAME+2];  /* +1 '\0'  +1 alignment */

#define ctMEM
#define ctDECL

/* die Dateimodidefinitionen sind heilig !! Anfassen verboten !      */
#define EXCLUSIVE 	0x0000		/* file in locked mode	     */
#define SHARED		0x0001		/* file in shared mode	     */
#define	VIRTUAL		0x0000		/* file is virtually opened  */
#define	PERMANENT	0x0002		/* file is physically opened */
#define	FIXED		0x0000		/* fixed length data	     */
#define	VLENGTH		0x0004		/* variable length data	     */
#define READFIL		0x0008		/* read only file lock	     */
#define PREIMG		SHADOW		/* transactions w/o recovery */
#define TRNLOG		(LOGFIL | SHADOW) /* PREIMG + recovery	     */
#define WRITETHRU	0x0040		/* write thru buffering	     */
#define CHECKLOCK	0x0080		/* must own lock for update  */
#define DUPCHANEL	0x0100		/* two i/o channels	     */
#define SUPERFILE	0x0200		/* superfile shell	     */
#define CHECKREAD	0x0400		/* must own lock on read     */
#define DISABLERES	0x0800		/* disable resource on create*/
#define OPENCRPT	0x4000		/* open corrupt file	     */

#define NONEXCLUSIVE	(READFIL | SHARED)
#define COMPLETE	EXCLUSIVE
#define PARTIAL		SHARED
#define NOTREUSE	0x0010		/* key type modifier: !reuse nodes  */

#define LEAPORT_INCL

#endif /* LEAPORT_INCL */

/* end of leaport.h */
