/* Es wird die Anzahl der benoetigten Shutdownauftraege an den
   Server festgelegt, mit der ein bestimmter Serverzustand
   erreicht werden soll.
*/

/* Beim ersten Versuch wird der Server beendet, falls keine 
   Transaktion aktiv ist 
   Ist eine Transaktion aktiv, so wird der Shutdown Zaehler um eins
   erhoeht und der Fehler TA_ACTIVE zurueckgegeben
   Ebenso bei den Versuchen NO_MORE_TA und NO_MORE_CLIENTS
   Beim Versuch SHUTDOWN werden evtl. offene Transaktionen
   zurueckgerollt und evtl. offene dateien geschlossen
*/

#ifndef SHUTDOWN_INCL

#define FIRST_SHUTDOWN  0
#define SECOND_SHUTDOWN 1
#define NO_MORE_CLIENTS 2	/* keine neue Transaktion mehr */
#define NO_MORE_TA     	3	/* keine neuer Benutzer mehr */
#define SHUTDOWN	4	/* Server wird sofort beendet */

#define SHUTDOWN_INCL

#endif /* SHUTDOWN_INCL */
