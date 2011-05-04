/*----UEBERSETZUNGSEINHEIT---------------------------------------------------*/
/*									     */
/* Name: katfuncdef.h         						     */
/*									     */
/* Beschreibung: Deklaration der Datensatzstruktur der EBO Katalogdatei      */
/*									     */
/* Autor: R. Cramer							     */
/*									     */
/* Historie: <projektspezifische Beschreibung von Erstell- und Aenderungs-   */
/*            daten, Version u.s.w.>					     */
/*									     */
/*---------------------------------------------------------------------------*/
#ifndef KATFUNCDEF_INCL

typedef struct {
	COUNT   soffset;        /* segment offset       */
	COUNT   slength;        /* segment length       */
	} KAT_SEGM; /* == 4 Bytes */

typedef struct {
	TEXT        fipath[MAX_EBO_DB_PFAD+MAX_EBO_DATNAM+2];           /* Dateipfad */
	TEXT        finam[MAX_EBO_DATNAM+1];                            /* Dateiname */
	TEXT        keyname[MAX_EBO_INDEX][MAX_EBO_IDXNAME+1];          /* Tabelle der SI-Namen */
	NINT        reclen;                                             /* Satzlaenge */
	NINT        indexes;                                            /* Anzahl Indexe incl. Primaer */
	NINT        keylen[MAX_EBO_INDEX];                              /* Schluessellaenge */
	NINT        anzseg[MAX_EBO_INDEX];                              /* Anzahl Einzelsegmente je Schluessel */
	KAT_SEGM    segs[MAX_EBO_INDEX*MAX_EBO_SEGMENT];                /* Tabelle aller Schluesselsegmente */
} KAT_SATZ;        /* == 492 Bytes */


/* Uebergabe zwischen Katalogverwaltung und den aufrufenden Funktionen */
typedef  struct {          
    TEXT pfilnam[MAX_EBO_DB_PFAD+MAX_EBO_DATNAM+2];     /* Dateiname inkl. Pfad */
    TEXT kurzname[MAX_EBO_DATNAM+1];                    /* Name der Datei ohne Pfad */
    IFIL entry_ifil;                                    /* IFIL-Struktur der Datei */
    IIDX entry_iidx[MAX_EBO_INDEX];                     /* Alle Indexstrukturen der Datei */   
    ISEG entry_iseg[MAX_EBO_INDEX*MAX_EBO_SEGMENT];     /* Alle Schluesselsegmente der Datei */
    TEXT sinamen[MAX_EBO_INDEX][MAX_EBO_IDXNAME+1];      /* Alle Schluesselnamen der Datei */
} KATENTRY_IFIL_S;

#define KATFUNCDEF_INCL

#endif /* KATFUNCDEF_INCL */
