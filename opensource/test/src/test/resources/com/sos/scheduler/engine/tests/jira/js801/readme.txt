In diesem Ordner ist eine Beispielkonfiguration enthalten, die unabhï¿½ngig vom exit code eine Jobkettenknotens gewï¿½hrleistet, dass jeder Schritt der Jobkette zur
Ausfï¿½hrung kommt. Im Fehlerfall wird eine zentrale Fehlerbehandlung ("error_handler") aufgerufen, der zweierlei macht:
1. Start der Jobkette "windows_event" (hier kï¿½nnte auch eine andere Form der Fehlerbehandlung stattfinden)
2. Fortsetzen der Jobkette am nï¿½chsten folgenden Knoten (realisert durch die ï¿½bergabe des Parameters "next_step" aus dem Postprocessing eines jeden jobs)

Damit ist gewï¿½hrleistet:
- das jede Jobkette mit allen Schritten ausgefï¿½hrt wird (die Kette immer mit "success" endet
- Fehler ï¿½ber eine zentrale Fehlerbehandlung protokolliert werden und auch dort (ï¿½ber die Jobkette "windows_event") gemonitored werden kï¿½nnen.

Die Jobkette endet immer mit dem Job "last_step", der die Order auf fehlerhaft setzt, wenn wenigstens ein Job der Kette mit einem Fehler beendet worden ist.
Sind mehrere Jobs der Kette mit einem Fehler beendet worden, wird der hï¿½chste der exit_codes als Fehler der Order gesetzt.

SS, 18.11.11
