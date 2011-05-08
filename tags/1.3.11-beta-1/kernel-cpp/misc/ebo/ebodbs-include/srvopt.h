#ifndef SRVOPT_INCL

#define CTS_MAXUSR      128	/* maximum number of users              */
#define CTS_MAXFIL      2000    /* maximum number of files              */
#define CTS_NODSEC      32     /* number of node sectors               */
#define CTS_BUFFER      84     /* number of index buffers              */
#define CTS_KEYOPS      (2 * MAX_DAT_KEY)

#define DATEXTSIZ	4096    /* Erweiterung .dat Datei */
#define IDXEXTSIZ	4096    /* Erweiterung .idx Datei */

#define WIEDERANLAUF            /* Server mit Wiederanlauf */

#define SRVOPT_INCL

#endif /* SRVOPT_INCL */

