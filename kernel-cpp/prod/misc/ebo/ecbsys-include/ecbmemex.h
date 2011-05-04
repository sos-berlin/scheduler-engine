#ifndef ECBMEM_INCL

/* Allokieren eines Speicher >= 10 Byte vom Heap */
EXT DLL VOID *Salloc(NINT no_bytes);
                                                   
/* Allokieren eines Speichers >= 100 Byte vom Heap */
EXT DLL VOID *Balloc(NINT no_bytes);
                                                     
/* Vergroessern eines zuvor allokierten Speichers im Heap */
EXT DLL VOID *Brealloc(VOID * ptr, NINT no_bytes);
                                                     
/* Freigabe eines zuvor vom Heap allokierten Speichers */
EXT DLL VOID Bfree(VOID *memaddr);

/* Initialisieren Speicherbereich mit angegebenen Wert */
EXT DLL VOID ecb_memset(VOID *mem,NINT value,NINT size);

/* Vergleich zweier Speicherbereiche */
EXT DLL NINT ecb_memcmp(VOID *mem1,VOID *mem2,NINT size);

/* Kopieren eines Speicherbereiches */
EXT DLL VOID ecb_memcpy(VOID *ziel,VOID *quelle,NINT laenge);

/* suche Zeichen im Speicherbereiches */
EXT DLL pTEXT ecb_memchr(VOID *mem1,NINT zeichen,NINT laenge);

/* Kopieren eines Speicherbereiches mit Tauschen */
EXT DLL VOID ecb_swab(VOID *quelle,VOID *ziel,NINT anzahl);

/* Kopieren eines Speicherbereichs rueckwaerts */
EXT DLL VOID ecb_memcpyr(VOID *ziel,VOID *quelle,NINT anzahl);

#define ECBMEM_INCL

#endif /* ECBMEM_INCL */
