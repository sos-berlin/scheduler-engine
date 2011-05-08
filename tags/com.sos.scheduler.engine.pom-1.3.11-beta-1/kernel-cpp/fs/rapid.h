/* rapid.h                                              (c) Joacim Zschimmer
   20. 2.91
   Datenstrukturen aus Rapid
*/

#ifndef __RAPID_H
#define __RAPID_H

#if !defined __SOSSTREA_H
#   include "../kram/sosstrea.h"
#endif

#if !defined __INTEGER8_h
#   include "../kram/integer8.h"          // Nach sosstrea.h !
#endif

#if !defined __ABSFILE_H
#   include "../file/absfile.h"
#endif

#if !defined __IBM370_H
#   include "../fs/ibm370.h"
#endif

#if !defined __EBCDIC_H
#   include "../kram/ebcdic.h"
#endif

#include "../kram/sosobj.h"

namespace sos {


struct Stream;
extern Stream* host_connection;    // Für Rapid::Task
extern Stream* bs2ipk_stream;      // Für Rapid::Task


const int  rapid_fnamelen             = 200;
const int  rapid_membername_size      = 160;
const int  rapid_max_file_name_size   = 64;   // (BS2000 PLAM)   ?
const int  rapid_max_filename_size    = 64;   // (BS2000 PLAM)   ?
const int  rapid_param_offset         = 72;   // A(CALLS-Parameter) - A(LOCAL)
const int  rapid_errcode_size         = 16;   // Länge eines Rapid-Fehlercodes
const int  rapid_errtext_size         = 300;  // Max. Länge eines Fehlertextes
const uint rapid_max_task_buffer_size = 256u + 2u + 32767u + 256u + 100u;
const uint rapid_fd_binary_size       = 40;
const uint rapid_openpar_binary_size  = 0x118;
const uint rapid_parflget_binary_size = 0x2C;
const uint rapid_parflput_binary_size = 28;
const uint rapid_parflinf_binary_size = 24;
const uint rapid_fileinfo_binary_size = 0x294;
const uint rapid_renampar_binary_size = 0x1A4;
const uint rapid_erasepar_binary_size = 0xDC; // + 8 für eracontx? jz 20.3.95

//?jz 13.11.94 Widerspruch zu sos.h: typedef enum {close_normal, close_roll_back, close_emergency} Close_mode;

//----------------------------------------------------Rapid_task_connection_cmd

enum Rapid_task_connection_cmd { //taxcreq = 1,
		   //taxcrel = 2,
			 taxccal = 3,
			 taxcxcp = 4,
			 taxcret = 5
};


struct Rapid
{

struct Fd                         // FILE DESCRIPTOR
{

    struct Fdattr {                       // ATTRIBUTE IM FILE DESCRIPTOR
                                Fdattr                  ();

        DECLARE_BIT( fdin   , _fdattr1 , 0x01 )   // in       Nur Lesen erlaubt
        DECLARE_BIT( fdout  , _fdattr1 , 0x02 )   // out      Nur streng sequentielles Schr.
        DECLARE_BIT( fdinout, _fdattr1 , 0x04 )   // inout    Alles erlaubt
        DECLARE_BIT( fdshare, _fdattr1 , 0x08 )   // share    Gemeinsamer Zugriff (SHARUPD=YES)
        DECLARE_BIT( fddupky, _fdattr1 , 0x10 )   // dupkey   Schlüssel nicht eindeutig -jz 18.5.97
        DECLARE_BIT( fdcomns, _fdattr1 , 0x20 )   // comnoseq Mehrfaches Oeffnen ohne sequen-
                                                  //          tielle Bearbeitung. Es wird nur ein
                                                  //          FD aufgebaut.
        DECLARE_BIT( fderase, _fdattr1 , 0x40 )   // erase    Bei @CLOSE Datei entfernen
        DECLARE_BIT( fdflgmv, _fdattr1 , 0x80 )   //          GET-Routinen koennen den Satz in einen angege-
                                                  //          benen Puffer uebertragen. (s.a. $FDFLGLC).

        // FDATTR2  Attribute zum Zugriff
        DECLARE_BIT( fdseq  , _fdattr2 , 0x01 )   // seq      Streng sequentielle Verarbeitung (in/out
        DECLARE_BIT( fdrecno, _fdattr2 , 0x02 )   // rec#     Zugriff ueber Satznummern
        DECLARE_BIT( fdkey  , _fdattr2 , 0x04 )   // key      Zugriff ueber Schluessel (vor @OPEN)
                                                  //          Saetze enthalten Schluessel (nach @OPEN)
        DECLARE_BIT( fdignky, _fdattr2 , 0x08 )   // ignkey   Schluessel am Satzanfang ignorieren (in)
        DECLARE_BIT( fdwrout, _fdattr2 , 0x10 )   // wrout    Saetze sofort auf die Platte schreiben
        DECLARE_BIT( fdkyext, _fdattr2 , 0x20 )   //          Zu jedem Satz gibt es einen Schluessel,
                                                  //          der aber nicht im Satz enthalten ist.
        DECLARE_BIT( fdbfuse, _fdattr2 , 0x40 )   //       Puffer (FDBUFADR) ist in Gebrauch, nicht freigeb.
        DECLARE_BIT( fdclslb, _fdattr2 , 0x80 )   // ??       Close-Treiber muss Bibliothek schliessen.
                                                  // LibID    muss in eigener Variable gemerkt werden.

        // FDATTR3  Attribute zur Datei
        DECLARE_BIT( fddir  , _fdattr3 , 0x01 )   // dir      Directory
        DECLARE_BIT( fdstrea, _fdattr3 , 0x02 )   // stream   Keine Satzstruktur
        DECLARE_BIT( fdfixrc, _fdattr3 , 0x04 )   // f        Saetze fester Laenge
        DECLARE_BIT( fdvarrc, _fdattr3 , 0x08 )   // v        Saetze variabler Laenge
        DECLARE_BIT( fdprint, _fdattr3 , 0x10 )   // print    Erste Spalte mit Vorschubsteuerzeichen
        DECLARE_BIT( fddestr, _fdattr3 , 0x20 )   // destroy  Bei @ERASE Inhalt loeschen
        DECLARE_BIT( fdgpush, _fdattr3 , 0x40 )   // getpush  Schubbetrieb bei @GET
        DECLARE_BIT( fdppush, _fdattr3 , 0x80 )   // putpush  Schubbetrieb bei @PUT

        // FDATTR4  Indicators
        DECLARE_BIT( fdeof  , _fdattr4 , 0x01 )   //          EOF  (get)
        DECLARE_BIT( fdpteof, _fdattr4 , 0x02 )   //          @PUT ist nicht mehr erlaubt
        DECLARE_BIT( fdinfo , _fdattr4 , 0x04 )   // info     Zus^atzliche Informationen zu "dir".
                                                  //          Der Dateiname kommt nach X'00', das in-
                                                  //          nerhalb der ersten 256 Byte steht.
                                                  //          Wenn kein X'00' vorhanden ist, steht nur
                                                  //          der Dateiname im Satz.
        DECLARE_BIT( fdlib  , _fdattr4 , 0x08 )   // lib      Bibliothek
        DECLARE_BIT( fdbakup, _fdattr4 , 0x10 )   // backup   Nur wenn mit "out n o" geoeffnet wird:
                                                  // Wenn im FTD BACKUP angegeben ist, dann
                                                  // wird eine neue Datei unter einem Phantasienamen ge-
                                                  // oeffnet und bei @CLOS die Originaldatei entfernt
                 	                              // (falls vorhanden) und der Phantasienamen gegen den
                                                  // Originalnamen getauscht. Der Phanatasiename ist
                                                  // acht Zeichen lang.
                                                  //   Wenn im FTD nicht BACKUP angegeben wird (z.B. DMS,
                                                  // weil hier die Passwords sonst verschwaenden), dann
                                                  // wird vor dem OPEN die Originaldatei auf eine Datei
                                                  // mit einem Phantasienamen kopiert, die dann bei
                                                  // @CLOS entfernt wird. @CLOS A=(RESTORE) wird die
                                                  // Datei wieder zurueck kopiert.
        DECLARE_BIT( fd_put , _fdattr4 , 0x20 )   // Mit @PUT/@INS ist eine Zeile begonnen.
                                                  // Wird TRUE , wenn eine Zeile begonnen, aber nicht
                                                  // abgeschlossen wird (@PUT, @INS statt @PUTL, @INSL).
                                                  // Setzt, liest: MUFLPUT
        DECLARE_BIT( fdinitu, _fdattr4 , 0x40 )   // Wird von einer Datei-INIT/EXIT-Routine
                                                  // benutzt. Ein @CLOS setzt dieses Flag nur
                                                  // zurueck und das naechste @CLOS schliesst
                                                  // dann die Datei (fuer MUSTART2).
        DECLARE_BIT( fdflglc, _fdattr4 , 0x80 )   // Die GET-Routinen lassen sich ohne Puffer aufrufen
                                                  // (-$GETBUFF). Sie geben Adresse und Laenge des
                                                  // gelesenen Satzes zurueck (eigener Puffer).

        friend void             read_fdattr             ( Sos_binary_istream*, Fdattr* );
        friend void             write_fdattr            ( Sos_binary_ostream*, const Fdattr& );

      private:
        Byte                   _fdattr1;
        Byte                   _fdattr2;
        Byte                   _fdattr3;
        Byte                   _fdattr4;
    };


                                Fd                      ();

    Fdattr                     _fdattr;                 // Attribute
    DECLARE_PUBLIC_MEMBER( Addr   , fdftdad  )          // A(File Type Descriptor)
    DECLARE_PUBLIC_MEMBER( uint2  , fdobfln  )          // Für MUFLPUT (@PUT)
    DECLARE_PUBLIC_MEMBER( Byte   , fdobfof  )          // Offset im Ausgabepuffer
    DECLARE_PUBLIC_MEMBER( uint2  , fdkp     )          // Key position (ab 0), oder < 0
    DECLARE_PUBLIC_MEMBER( uint2  , fdkl     )          // Key length, oder <= 0
    DECLARE_PUBLIC_MEMBER( Addr   , fdgetloc )          // A(GETLOC-Routine)
    DECLARE_PUBLIC_MEMBER( Addr   , fdbufadr )          // A(Puffer)
    DECLARE_PUBLIC_MEMBER( Addr   , fdibfptr )          // A(nächstes Zeichen fr MUFLGET)
#   define                         fdibfemp 0xFF       //   Eingabepuffer ist leer
    DECLARE_PUBLIC_MEMBER( uint2  , fdibflen )          // len (Eingabe)
    DECLARE_PUBLIC_MEMBER( uint2  , fdbufsiz )          // Puffergr”áe

    uint                        binary_size             () { return rapid_fd_binary_size; }
    friend void                 read_fd                 ( Sos_binary_istream*, Fd* );
    friend void                 write_fd                ( Sos_binary_ostream*, const Fd& );

  private:
    Byte                       _link [2] [4];           // Linked list
    Byte                       _reserve_1;

  //void                        object_store            ( ostream& );
  //Fd&                         object_load             ( istream& );
};

struct Openpar : Sos_self_deleting     // Parameterbereich fuer OPEN             OPENPAR
{
    struct Opflags
    {
                                Opflags                 ();

        DECLARE_BIT( opnew  , _opflag1, 0x01 )  // n/new   Neue Datei beschreiben (ueberschreiben)
        DECLARE_BIT( opcreat, _opflag1, 0x02 )  // create  Datei wenn noetig einrichten (/FILE)
        DECLARE_BIT( opow   , _opflag1, 0x04 )  // o/ow    Ueberschreiben erlaubt
        DECLARE_BIT( opexist, _opflag1, 0x08 )  // exist   Datei muss logisch existieren

        DECLARE_BIT( opnuc  , _opflag2, 0x01 )  // nuc      Fileserver verwenden
        DECLARE_BIT( opcom  , _opflag2, 0x02 )  // com      Gemeinsamer Zugriff auf eine Datei
        DECLARE_BIT( oplibop, _opflag2, 0x04 )  // Bibliothek OPLIBID ist geoeffnet worden. ???
                                                // Der Dateitreiber muss sie bei CLOSE schliessen!

        friend void             read_opflags            ( Sos_binary_istream*, Opflags* );
        friend void             write_opflags           ( Sos_binary_ostream*, const Opflags& );

      private:
        Byte                   _opflag1;
        Byte                   _opflag2;
        Byte                   _opflag3;
        Byte                   _opflag4;
    };

                                Openpar                 ();



    char                       _optyp_string0  [ 8 + 1 ];  // Typname, Grossbuchstaben
    char                       _opname_string0 [ rapid_fnamelen + 1 ]; // Dateiname, ohne Dateityp
    Byte                       _oppass [ 8 ];   // (EBCDIC bzw. binär ) Password oder XL8'00' pass=
    DECLARE_PUBLIC_MEMBER( int4   , opfirec )   // Nummer des ersten Satzes	 rec#=
    DECLARE_PUBLIC_MEMBER( uint2  , oprl )      // Satzlaenge              rl= / f= / v=
    DECLARE_PUBLIC_MEMBER( Addr   , opdatadr )  // @OPEN DATA=
    DECLARE_PUBLIC_MEMBER( uint4  , oplibid  )  // File-ID der Bibliothek oder 0
    char                       _opnucnam_string0 [ 8 + 1 ];  // nuc=  Name des Fileservers fuer FLNUC
    Opflags                    _opflags;
    DECLARE_PUBLIC_MEMBER( uint4  , opcmdaid )
    DECLARE_PUBLIC_MEMBER( uint4  , opfddaid )  // DAID des FD

    Integer8                   _opcrdate;       // Creation date (STCK)       oder 0
    Integer8                   _opmddate;       // Modification date (STCK)   oder 0

    char                       _opcontxt_string0 [ 8+1 ]; // Names des Kontextes oder Blank (=Benutzer)

    uint                        binary_size             () { return rapid_openpar_binary_size; }
    friend void                 read_openpar            ( Sos_binary_istream*, Openpar* );
    friend void                 write_openpar           ( Sos_binary_ostream*, const Openpar& );

  private:
    uint2                       ___opkl;
    uint2                       ___opkp;
    Byte                       _alignment_1 [ 6 ];

  //void        object_store( ostream& );
  //Openpar&    object_load( istream& );
};

struct Parflget                 // CALLS-Parameterbereich fuer GET-Routinen
{
    struct Flags
    {
                                Flags                   ();

        DECLARE_BIT( getkyna, _byte[ 2 ], 0x01 )  // KEYNAME= angegeben: Name des Sekund„rschluessel
        DECLARE_BIT( getkend, _byte[ 2 ], 0x02 )  // KEYEND= angegeben: Letzter Schluessel
        DECLARE_BIT( getrcnt, _byte[ 2 ], 0x04 )  // COUNT= angegeben: Geschätzte Anzahl Sätze

        DECLARE_BIT( getneof, _byte[ 3 ], 0x01 )  // Nur fuer MUFLGET: Nicht Exception EOF ausloes.
        DECLARE_BIT( getlock, _byte[ 3 ], 0x02 )  // Zu lesenden Satz sperren
        DECLARE_BIT( getrcno, _byte[ 3 ], 0x04 )  // Satz ueber eine Satznummer lesen  (->FTDGETR#)
        DECLARE_BIT( getkey , _byte[ 3 ], 0x08 )  // Satz ueber einen Schluessel lesen (->FTDGETKY)
        DECLARE_BIT( getbuff, _byte[ 3 ], 0x10 )  // Satz in den angegebenen Puffer lesen
        DECLARE_BIT( getline, _byte[ 3 ], 0x20 )  // Nur fuer MUFLGET: @GETL (nicht @GET)
        DECLARE_BIT( getrec , _byte[ 3 ], 0x40 )  // Nur fuer MUFLGET: Aufruf von @GETR
        DECLARE_BIT( getrev , _byte[ 3 ], 0x80 )  // Rückwärts lesen

        friend void             read_parflget_flags     ( Sos_binary_istream*, Flags* );
        friend void             write_parflget_flags    ( Sos_binary_ostream*, const Flags& );

      private:
        Byte                   _byte [ 4 ];
    };

                                Parflget                ();

    Flags                      _flags;

    Addr                       _getfdadr;       // A(File Descriptor)

    Addr                       _getbfadr;       // if $GETBUFF: Pufferadresse
    int4                       _getbfsiz;       // if $GETBUFF: Puffergroesse

    Addr                       _getkyadr;       // if $GETKEY: A(Schluessel)
    int4                       _getkylen;       // if $GETKEY: len (Schluessel) bei var. Laenge
#   define                    _getrecno _getkylen // if $GETRC#: Satznummer, beginnend bei 0

    Addr                       _getnamad;       // if $GETKYNA: A(Schluesselname)
    int4                       _getnamln;       // if $GETKYNA: len (Schluesselname)

    Addr                       _getendad;       // if $GETKEND: A(letzter Schluessel)

    int4                       _getrccnt;       // if $GETRCNT: Gesch„tzte Anzahl der S„tze

    friend void                 read_parflget   ( Sos_binary_istream*, Parflget* );
    friend void                 write_parflget  ( Sos_binary_ostream*, const Parflget& );
};


struct Parflput                 // CALLS-Parameterbereich fuer PUT-Routinen
{
                                Parflput                ();

    struct Flags
    {
                                Flags                   ();

        DECLARE_BIT( putupd , _byte[ 2 ], 0x01 )
        DECLARE_BIT( putasyn, _byte[ 2 ], 0x02 )

        DECLARE_BIT( putout , _byte[ 3 ], 0x01 )    // Satz ausgeben ohne Zeilenende (fuers Terminal)
      //DECLARE_BIT(        , _byte[ 3 ], 0x02 )    // (bei GET: Satz sperren)
        DECLARE_BIT( putrcno, _byte[ 3 ], 0x04 )    // Satz ueber eine Satznummer schreiben
        DECLARE_BIT( putkey , _byte[ 3 ], 0x08 )    // Satz ueber einen Schluessel schreiben
        DECLARE_BIT( putins , _byte[ 3 ], 0x10 )    // Satz einfuegen (nicht ueberschreiben) @INS
        DECLARE_BIT( putline, _byte[ 3 ], 0x20 )    // Nur f. MUFLPUT: @PUTL statt @PUT (Zeilenende)
        DECLARE_BIT( putrec , _byte[ 3 ], 0x40 )    // (Nur fuer MUFLPUT: Aufruf von @PUTR)
        DECLARE_BIT( putkeyx, _byte[ 3 ], 0x80 )    // Schluessel ist extra  KEY=...

        friend void             read_parflput_flags     ( Sos_binary_istream*, Flags* );
        friend void             write_parflput_flags    ( Sos_binary_ostream*, const Flags& );

      private:
        Byte                   _byte [ 4 ];
    };

    uint4                      _putfilid;
    Flags                      _flags;

    Addr                       _putadr;         // if $PUTBUFF: Pufferadresse
    int4                       _putlen;         // if $PUTBUFF: Puffergroesse

    Addr                       _putkyadr;       // if $PUTKEY: A(Schluessel) bei seperatem Schl.
    int4                       _putkylen;       // if $PUTKEY: len (Schluessel) bei var. Laenge
#   define                    _putrecno _putkylen // if $PUTREC#: Satznummer, beginnend bei 0
    uint4                      _putcalid;       // if $PUTASYN:

    friend void                 read_parflput           ( Sos_binary_istream*, Parflput* );
    friend void                 write_parflput          ( Sos_binary_ostream*, const Parflput& );
};

//typedef Parflput::Flags Parflput_flags;         // gcc 2.5.7

struct Erasepar
{
                                Erasepar                ();

    Byte                       _reserve[3];
    Byte                       _eraflags;

    uint4                      _eralibid;       //  DAID (Library-FD)  oder 0
    uint4                      _eracomid;       //  DAID (COMMON) oder 0
    char                       _eraname_string0  [ rapid_fnamelen + 1 ];
    char                       _eracontx_string0 [ 8 + 1 ];

    static uint                 binary_size             () { return 4 + 4 + rapid_fnamelen + 8; }

    friend void                 read_erasepar           ( Sos_binary_istream*, Erasepar* );
    friend void                 write_erasepar          ( Sos_binary_ostream*, const Erasepar& );
};

struct Renampar                 // CALLS-Parameterbereich fuer RENAME-Routinen'
{
                                Renampar                ();

    Byte                       _reserve [ 3 ];
    Byte                       _renflags;

    uint4                      _renlibid;       // DAID (Library-FD)  oder 0
    uint4                      _rencomid;       // DAID (COMMON) oder 0
    char                       _renoldna_string0 [ rapid_fnamelen + 1 ];
    char                       _rennewna_string0 [ rapid_fnamelen + 1 ];
    char                       _rencontx_string0 [ 8 + 1 ];

    friend void                 read_renampar           ( Sos_binary_istream*, Renampar* );
    friend void                 write_renampar          ( Sos_binary_ostream*, const Renampar& );
};


struct Fileinfo                 // Dateiinformationen:
{
                                Fileinfo                ();

    char                       _fityp_string0  [ 8 + 1  ];              // Dateityp
    char                       _finame_string0 [ rapid_fnamelen + 1 ];  // Dateiname (normiert, aber kein UCASE)
					                                                    // Mit Bibliotheksnamen
    char                       _ficlass [ 8 + 1];                       // Dateiklasse (Name des Programms fuer die Datei)
    char                       _ficommen_string0 [ 80 ];        // Kommentar

    Integer8                   _ficrstck;       // Date of Creation                STCK-Format  or 0
    Integer8                   _fimdstck;       // Date of Last Modification       STCK-Format  or 0
    Integer8                   _filastck;       // Date of Last Access             STCK-Format  or 0
    Integer8                   _fiexstck;       // Date of Expriration             STCK-Format  or 0

    int4                       _fisize;         // Genutzte Groesse                      Bytes  or 0
    int4                       _fireccnt;       // Anzahl der Saetze                     Bytes  or 0
    int4                       _fispace;        // Insgesamt reservierter Platz          Bytes  or 0
    int4                       _fispace2;       // Second Allocation                     Bytes  or 0

    Byte                       _access_flags [ 4 ];
    Byte                       _x_flags      [ 4 ];
    Byte                       _status_flags [ 4 ];  // Augenblicklicher Zustand:

    Fd::Fdattr                 _fiattr;

    int4                       _fiblksiz;       // Block Size                                   or 0
    uint2                      _firl;           // Record Length (Maximum)                      or 0
    uint2                      _fikl;           // Key Length
    uint2                      _fikp;           // Key Position
    Byte                       _reserve4 [8];

    char                       _fimbname_string0 [ rapid_membername_size + 1 ]; // Dateiname ohne Bibliotheksname und Klammern
    uint4                      _filibid;        // Library FileID                               or 0

    int2                       _finameln;       // len (FINAME)
    int2                       _fimbnaln;       // len (FIMBNAME)

    friend void                 read_fileinfo           ( Sos_binary_istream*, Fileinfo* );
    friend void                 write_fileinfo          ( Sos_binary_ostream*, const Fileinfo& );

};


struct Parflinf                // CALLS-Parameterbereich fuer FLINF-Routinen
{
                                Parflinf                ();

    Byte                       _flags [ 4 ];

    DECLARE_BIT( inffidx, _flags[ 3 ], 0x01 )  // INFFID ist gueltig
    DECLARE_BIT( infaccs, _flags[ 3 ], 0x02 )  // INFO=ACCESS
    DECLARE_BIT( inffile, _flags[ 3 ], 0x04 )  // INFO=FILE
    DECLARE_BIT( infname, _flags[ 3 ], 0x08 )  // INFO=NAME

    uint4                      _inffid;         // FileID
    Addr                       _infnamad;       // A(Dateiname)
    int4                       _infnamln;       // len (Dateiname)
    Addr                       _infaradr;       // A(Area)
    int4                       _infarsiz;       // size (Area)

    friend void                 read_parflinf           ( Sos_binary_istream*, Parflinf* );
    friend void                 write_parflinf          ( Sos_binary_ostream*, const Parflinf& );
};

//--------------------------------------------------------Rapid::Ipk_connection

// Implementiert in rapidipk.cxx

#if defined __BS2IPK_H
struct Ipk_connection : Abs_connection
{
    Ipk_connection( Bs2ipk_connection* );

    void open  ( const char*, Open_mode = in|out, const File_spec& = File_spec() );
    void close ( Close_mode );

    void flush ();
    virtual uint skip_record();

  protected:
    void write_area ( const Const_area& );
    void read_area  ( Area& );

  private:
    void _read_header();

    Ptr<Bs2ipk_connection>   _ipk_ptr;
    Ebcdic_array< 0, 7 >     _partner_name;
    Bool                     _record_begin;

};
#endif

//-------------------------------------------------------Rapid::Task_connection

struct Task_connection : Sos_self_deleting// : Abs_connection
{
    typedef enum Rapid_task_connection_cmd Cmd;

                                Task_connection         ( const Sos_object_ptr& );
                               ~Task_connection         ();

    void                        call                    ( const char* entry_name );
    void                        write_record            ();
    void                        read_answer             ();
    Sos_binary_ostream*         ostream_ptr             ()  { return &_output_stream; }
    Sos_binary_istream*         istream_ptr             ()  { return &_input_stream;  }

    Bool                        connection_lost         () const { return _connection_lost; }

  protected:
    void                        write_cmd               ( Cmd );    // Nur am Anfang eines Satzes.


  private:
    void                       _read_header();          // Die Antwort besteht nur aus dem Kopf

    Fill_zero                  _zero_;
    Sos_object_ptr             _conn_ptr;
    Sos_binary_ostream         _output_stream;
    Sos_binary_istream         _input_stream;
    uint4                      _server_word;
    uint4                      _my_word;
    uint4                      _task_id;
    Byte*                      _buffer_ptr;
    Bool                       _connection_lost;
    Bool                       _active;
};


}; // Rapid

//typedef Rapid::Parflput_flags Rapid_parflput_flags;     // gcc 2.5.8

#if defined SYSTEM_MICROSOFT
    struct Rapid_openpar : Rapid::Openpar {};                // MSVC++ 4.0

    struct Rapid_task_connection : Rapid::Task_connection   // MSVC++ 4.0
    {
                                    Rapid_task_connection   ( const Sos_object_ptr& o ) : Task_connection ( o ) {}
    };
#elif defined SYSTEM_SOLARIS
#   define Rapid_openpar            Rapid::Openpar
#   define Rapid_task_connection    Rapid::Task_connection
#else
    typedef Rapid::Openpar          Rapid_openpar;
    typedef Rapid::Task_connection  Rapid_task_connection;
#endif

} //namespace sos

#include "rapid.inl"
#endif

