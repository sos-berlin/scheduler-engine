// $Id$

#include "spooler.h"
#include "../zschimmer/directory_lister.h"

namespace sos {
namespace scheduler {
namespace folder {
namespace replication

using namespace zschimmer::file;

//-------------------------------------------------------------------------------------------------

/*
    Mit Supervisor verbinden

    <configuration.fetch_updated_files signal_next_change_at_udp_port="...">
        <configuration.files>
            <configuration.file file="path" timestamp=".." md5="..."/>
            <configuration.directory directory="directory/" timestamp="..">
                <configuration.my_file file="path" timestamp=".."/>
            </configuration.file>
        <configuration.files>
    </configuration.fetch_updated_files>


    Antwort:

    <configuration.files>
        <configuration.file file="path" timestamp=".." md5="..." encoding="base64">...</configuration.file>
        <configuration.file file="path" timestamp=".." md5="..."><xml>...</xml></configuration.file>    Funktioniert nicht mit MD5! 
        <configuration.file file="path" timestamp=".." md5="...">text...</configuration.file>           Funktioniert nicht mit MD5! 
        <configuration.file file="path" removed="yes"/>
        <configuration.directory directory="directory/" timestamp="..">
            <configuration.my_file file="path" timestamp=".."/>
        </configuration.file>
    </configuration.files>


    UDP:

    <configuration.changed/>




    
    START

    Verbindung zum Supervisor aufbauen
    geht nicht?
        Normal starten
        Weiter versuchen, die Verbindung zum Supervisor aufzubauen

    Verbindung aufgebaut?
        Wenn noch nicht eigene Konfigurationsdateien geladen (state<active)
            Eigene Konfigurationsverzeichnisse lesen, Timestamp und MD5 einsammeln
        Sonst bereits geladene Informationen benutzen

        an Supervisor senden und dabei UDP-Signal für nächste Änderung bestellen


        Supervisor registriert Scheduler
        merkt sich am Scheduler gespeicherte Dateien (Zeitstempel, MD5)
        Startet Verzeichnisüberwachung (sowieso durch folder.cxx)
        vergleicht Dateien und liefert Änderungen



    Verzeichnisänderung am Supervisor:
        Für jeden registrierten Scheduler mit bestelltem UDP-Signal:
            Verzeichnisse durchgehen und Änderung feststellen
            Falls geändert, UDP-Signal
            UDP-Signal wiederholen, wenn Scheduler Dateien nicht abholt, aber seine TCP-Verbindung noch besteht


    ZEITSTEMPEL
        in UTC



    QUOTED-PRINTABLE, verändert für Scheduler "scheduler-printable"
        09, 0A, 20 bis 7C -> durchlassen
        00 bis 1F -> U+7E '~', U+7E '~', U+40 '@' bis U+5F      U+00 -> ~~@   U+0D -> ~~M
        80 bis 9E -> U+7E '~', U+7E '~', U+60 '`' bis U+7E      U+80 -> ~~`
        9F bis FC -> U+7E '~', U+20 bis U+7D                    U+9F -> ~~    U+FD -> ~~}
        FD bis FF -> U+7E '~', U+7E '~', U+30 '0' bis U+32 '2'  U+FD -> ~~0   U+FF -> ~~3
        XML: Blanks am Anfang und am Ende beachten


    ODER VERÄNDERTES UTF-8 (nein!)
        09, 0A, 20 bis 7E -> durchlassen
        00 bis 1F -> wie 100 bis 11F in UTF-8
        80 bis 9F -> wie 120 bis 140 in UTF-8
        A0 bis FF -> in UTF-8
            

*/


//-------------------------------------------------------------------------------------------------

} //namespace replication
} //namespace folder
} //namespace scheduler
} //namespace sos
