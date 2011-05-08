      *----------------------------------------------------------------
      * Datensatzbeschreibung für ODBC 2000-Katalog
      *
      * Bei einer ISAM-Datei sollte TABLE_NAME der Schlüssel sein, z.B.
      * FCBTYPE=ISAM,RECFORM=F,RECSIZE=316,KEYPOS=16,KEYLEN=50 oder
      * FCBTYPE=ISAM,RECFORM=V,RECSIZE=320,KEYPOS=20,KEYLEN=50
      *
      * Die Feldlängen sind nur Beispiele.
      * TABLE_QUALIFIER, TABLE_OWNER, TABLE_NAME und TABLE_TYPE
      * können bis 128 Zeichen lang sein.
      * REMARKS kann bis 254 Zeichen lang sein.
      * FILE kann bis 200 Zeichen lang sein.
      *
      * Bei einer flachen Struktur wie hier wird die Feldauswahl
      * -fields=(table_qualifier,table_owner,table_name,table_type,
      * remarks,file) im Dateinamen nicht benötigt.
      *
      *01 CATALOG.
      *
      *   Bei RECFORM=V darf das Satzlängenfeld nicht aufgeführt sein!
      *   10 LEN                        PIC S9(4) COMP.
      *   10 FILLER                     PIX XX.
      *
      *      Mit LOW-VALUES füllen:
          10 TABLE_QUALIFIER            PIC X(8).
      *      Mit LOW-VALUES füllen:
          10 TABLE_OWNER                PIC X(8).
      *      Name der Tabelle:
          10 TABLE_NAME                 PIC X(50).
      *      Immer "TABLE":
          10 TABLE_TYPE                 PIC X(10).
          10 REMARKS                    PIC X(80).
      *      BS2000- oder Rapid-Dateiname:
          10 FILE                       PIC X(160).
      *----------------------------------------------------------------
