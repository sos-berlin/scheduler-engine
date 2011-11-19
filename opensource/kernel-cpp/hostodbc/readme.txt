hostODBC
Der ODBC-Treiber für BS2000 und anderes


SOS Software- und Organisations-Service GmbH
Badensche Straße 29
D-10715 Berlin

Fax     (030) 861 33 35
e-mail  sosberlin@aol.com

-------------------------------------------------------------------------------

INSTALLATION
============

Sie installieren den Treiber durch Aufruf des Windows-Programms SETUP.EXE aus
dem Installationsverzeichnis.

Um Daten Ihres BS2000-Rechners zu erreichen, muss dort das Produkt Fileserver
der SOS GmbH installiert sein.

Die 32-Bit-Version ist NICHT auf win32s (d.h. Windows 3.1 oder 3.11) benutzbar.

Für die 32-Bit-Version wird die Cursor-Bibliothek ODBCCR32.DLL nicht installiert.
In der Regel ist sie aber bereits vorhanden.

-------------------------------------------------------------------------------

LIZENZ
======

Für den Betrieb benötigen Sie einen Lizenzschlüssel. Sie geben ihn in der Datei
SOS.INI in Ihrem Windows-Verzeichnis ein. Führen Sie hierzu "notepad sos.ini"
aus und suchen Sie den Abschnitt [licence]. Wenn er noch nicht eingerichtet ist,
tippen Sie folgendes ein, andernfalls ergänzen Sie den Abschnitt entsprechend:

[licence]
key1=SOS-kunde-nr-xxxx-yyyyyyy

Ohne Lizenzschlüssel wird der Fehler "SOS-1000" gemeldet.

Wenn Sie einen Lizenzschlüssel zu bereits vorhandenen hinzufügen möchten, dann
verwenden Sie den Eintrag mit der nächsten freien Nummer, in diesem Fall key2=.
Die Numerierung muss lückenlos sein!

Die ersten drei Teile des Lizenzschlüssel bilden die Lizenznummer SOS-kunde-nr.
Eine Lizenznummer darf an einem Tag nur an einem Computer verwendet werden. Die
Verwendung einer Lizenznummer an zwei Computern am selben Tag ist nicht zuläs-
sig.




