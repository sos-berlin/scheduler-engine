/* PUTLIB                                        (c) Joacim Zschimmer
    6. 9.89

   Die Eingabedatei wird in einzelne Ausgabedateien zerlegt.
   Sie enth„lt Trenns„tze "*** FILENAME IS : ext.name"

   Mit moddate=yymmddhhmmss hinter dem Dateinamen kann das Dateidatum
   angegeben werden.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <dos.h>

void main (void) {

FILE *source, *datei;
char zeile [256+1];
char filename [80];
int  n;
int  Anzahl = 0;
char Kennung [19];
int  Laenge_der_Kennung = 18;
int     f;
int     jahr;
int     monat;
int     tag;
int     stunde;
int     minute;
int     sekunde;
int     rc = 0;

strcpy (Kennung, "*** FILENAME IS : ");

fprintf (stderr, "PUTLIB\n");

source = stdin;


setmode (fileno (stdin ), O_BINARY);
setvbuf (stdin , NULL, _IOFBF, 0x7000);

if feof (source) {
   fprintf (stderr, "Die Eingabedatei ist leer\n");
   exit (1);
}
if ((fgets (zeile, sizeof zeile, source)) == NULL) {
    perror ("Fehler bei fgets: ");
    exit (1);
}
if (strncmp (& zeile [3], Kennung, Laenge_der_Kennung)) {
    fprintf (stderr, "Die Eingabedatei ist nicht mit GETLIB erstellt worden\n");
    fprintf (stderr, zeile);
    exit (2);
}
do {
    filename [0] = 0;
    jahr    = 0;
    monat   = 0;
    tag     = 0;
    stunde  = 0;
    minute  = 0;
    sekunde = 0;

    n = sscanf (zeile + 3 + Laenge_der_Kennung,
                       " %s moddate=%4d%2d%2d%2d%2d%2d", 
                       filename, 
                       &jahr, &monat, &tag, &stunde, &minute, &sekunde); 
        
    printf ("%3d. %-12s", ++Anzahl, filename);

    if ((datei = fopen (filename, "wb")) == NULL) {
        fprintf (stderr, "Fehler beim Einrichten der Datei %s:\n", filename);
        perror ("");
        rc  = 4;
        for (;;) {
    	    zeile [3] = '\0';
	    if (fgets (zeile, sizeof zeile, source) == NULL)  break;

	    if (! strncmp (& zeile [3], Kennung, Laenge_der_Kennung))  break;
        }
        continue;
    }

    setvbuf (datei, NULL, _IOFBF, 0x7000);

    for (;;) {
	zeile [3] = '\0';
	if (fgets (zeile, sizeof zeile, source) == NULL)  break;

	if (! strncmp (& zeile [3], Kennung, Laenge_der_Kennung))  break;
	if (fputs (zeile, datei) == EOF) {
            fprintf (stderr, "Fehler beim Beschreiben der Datei %s:\n",
                             filename);
            perror ("");
            exit (5);
        }
    }

    fclose (datei);

    if (jahr > 0) {
        printf ("  %2d.%2d.%4d ", tag, monat, jahr);
        if (stunde > 0  &&  minute > 0  &&  sekunde > 0) {
            printf ("%2d:%02d:%02d", stunde, minute, sekunde);
        }
    }

/*
    f = open (filename, O_RDWR);
    _dos_setftime (f, 
                   tag | (monat << 5) | ((jahr > 0 ? jahr - 1980 : 0) << 9),
                   (sekunde >> 1) | (minute << 5) | (stunde << 11));

    close (f);
*/
    printf ("\n");
} while (! feof (stdin));

if (source != stdin) {
    fclose (source);
}

exit (rc);
}
