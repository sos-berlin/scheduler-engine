#ifndef ECBENV_INCL

#define MAX_ENV_VARIABLEN  64

typedef struct {
      pTEXT  mem_ecbenv;  /* Speicheradresse Environmentvariablen */
      COUNT  anz_ecbenv;            /* Anzahl Environment-Variablen  */
      COUNT  lang_ecbenv;           /* Laenge Environment-Variablen  */
      LONG   off_envname[MAX_ENV_VARIABLEN];    /* offset Namen im Speicher */
      LONG   off_envwert[MAX_ENV_VARIABLEN];    /* offset Werte  "    "     */
} ENV_HANDLE_S; 


/* Environment-Tabelle erstellen 
   IN	pTEXT 	dateiname inklusive Pfad der Environmentdefinitionen 
   liefert ECB_ERROR im Fehlerfall zurueck, sonst NO_ERROR
*/
EXT DLL COUNT init_ecbenv(ENV_HANDLE_S*,pTEXT);

/* Environment-Tabelle wieder freigeben 
*/
EXT DLL VOID free_ecbenv(ENV_HANDLE_S*);

/* Alle Environment-Variablen ausgeben 
*/
EXT DLL VOID all_ecbenv(VOID);

/* Wert der Environment-Variable  ermitteln
   IN 	pTEXT  	Name der Environmentvariablen 
   liefert Null-Pointer zurueck, falls environment-Variable nvor 
*/	
EXT DLL pTEXT get_ecbenv(ENV_HANDLE_S*,pTEXT);

/* Wert einer Environment-Variablen abfragen, Ersatz fuer
   getenv() */
EXT DLL pTEXT ecb_getenv(pTEXT);

/* Wert einer Environment-Variablen setzten.
   Ersatz fuer putenv()
*/
EXT DLL NINT ecb_putenv(pTEXT);

/* Wert einer Envorinment-Variablen setzen */
EXT DLL pTEXT ecb_setenv (pTEXT var,pTEXT value);


#define ECBENV_INCL

#endif /* ECBENV_INCL */
