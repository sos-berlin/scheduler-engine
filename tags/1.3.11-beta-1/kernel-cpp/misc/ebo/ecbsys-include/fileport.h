#ifndef FILEPORT_INCL

#ifndef ECBPORT_INCL
#include "ecbport.h"
#endif

#define NEW_CREATE	0
#define ALWAYS_CREATE   1
#define EXISTS_OPEN     2
#define ALWAYS_OPEN     3
#define READ_ONLY       4
#define WRITE_ONLY      5
#define READ_WRITE      6
#define SYNC_FILE       7
#define NOSYNC_FILE     8
#define READ_USRSHARE   0x01       /* 0000 0001 */
#define WRITE_USRSHARE  0x02       /* 0000 0010 */
#define READ_GRPSHARE   0x04       /* 0000 0100 */
#define WRITE_GRPSHARE  0x08       /* 0000 1000 */
#define READ_OTHSHARE   0x10       /* 0001 0000 */
#define WRITE_OTHSHARE  0x20       /* 0010 0000 */
#define READ_ALLSHARE   0x15       /* 0001 0101 */
#define WRITE_ALLSHARE  0x2A       /* 0010 1010 */
#define NO_SHARE        0x40       /* 0100 0000 */

#define INVALID_HANDLE  -1        /* ungueltiger Wert fuer Dateizeiger */


#if defined(WIN32) || defined(MSDOS)

#define PATH_SEPERATOR '\\'
#define PATH_SEPERATOR_STR "\\"

#define SEEK_FSTART      FILE_BEGIN
#define SEEK_FPOS	 FILE_CURRENT
#define SEEK_FEND 	 FILE_END

typedef		HANDLE		RNDFILE; /* Dateihandle Standardio */
typedef     RNDFILE *	pRNDFILE;
typedef		DWORD		FMODE;   /* Dateirechte bei Erzeugen */
typedef 	DWORD		FSEEK;	 /* Psoitionierungsmodus */
typedef  	DWORD		FSIZE;   /* Dateilaenge in Bytes und
					    Dateizeigerposition       */
typedef         DWORD           FBUFSIZ; /* Anz. zu verarbeitender Bytes bei
					   read()/write() */
typedef         DWORD           FNOBYTES;/* Anz. real verarbeiteter Bytes bei
					   read()/write() */
#else  /* defined(WIN32) || defined(MSDOS) */

#define PATH_SEPERATOR '/'
#define PATH_SEPERATOR_STR "/"

/* SEEK_SET, SEEK_CUR, SEEK_END werden aus unistd.h uebernommen */
#define SEEK_FSTART	SEEK_SET
#define SEEK_FPOS	SEEK_CUR
#define SEEK_FEND	SEEK_END

typedef		int		RNDFILE; /* Dateihandle Standardio */
typedef         int *	        pRNDFILE;
typedef		mode_t		FMODE;   /* Dateirechte bei Erzeugen */
typedef         int		FSEEK;   /* Positionierungsmodus */
typedef  	off_t		FSIZE;   /* Dateilaenge in Bytes und
					    Dateizeigerposition       */
typedef         size_t          FBUFSIZ; /* Anz. zu verarbeitender Bytes bei
					   read()/write() */
typedef         ssize_t         FNOBYTES;/* Anz. real verarbeiteter Bytes bei
					   read()/write() */
#endif

#define FILEPORT_INCL

#endif /* FILEPORT_INCL */
