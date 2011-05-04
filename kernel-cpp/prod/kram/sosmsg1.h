// sosmsg1.h                            jz 15.4.95
// Deponie für unausgereifte Klassen


//--------------------------------------------------------------------------------------Sos_factory

struct Sos_factory : Has_msg_handler
/* Name des gewünschten Objekttyps und Parameter als Text: "typ:parameter".
   Als Data_msg.

   Liefert Object_msg mit einen Zeiger auf das neue Objekt.

   Bsp.:  send( &sos_factory, Data_msg( "std:testfile" ) )

   Die Dateitypen sind Objekttypen. "std" ist Default.
   D.h. Dateien des Betriebssystems brauchen nicht weiter spezifiziert zu werden.

   Zerstört wird das Objekt mit dem delete-Operator.

   Erste Realisierung mit Any_file.

   Später soll jede Objektklasse ihre eigene Fabrik (d.h. Konstruktur) haben.

   Ein Ort, an dem das Objekt entstehen soll, kann angegeben werden (etwa IP-Adresse und TCP-Port).
   Es wird dann lokal ein Stellvertreter-Objekt angeleget (s. Sos_proxy).
*/
{
};

/* Besitzer bei verteilten Objekten

   Jedes verteilte Objekt hat einen Besitzer (Besitzt-Relation). Besitzer kann jedes Objekt
   sein.
   Ein Objekt kann mehreren Besitzern gehören.
   Das Objekt kennt nur die Anzahl seiner Besitzer, der Besitzer aber seine Objekte.

   Besitzer sind: Benutzer, Client-Prozess, Verbindung, ...

   Wird ein Besitzer gelöscht, werden alle seine Objekte gelöscht, d.h. deren Referenzzähler
   erniedrigt. Nur die Objekte, deren Referenzzähler 0 wird, werden tatsächlich gelöscht.

   Besitzbare Objekte erben von Sos_owned.

   Besitzende Objekte erben von Sos_owner, der den Besitz verwaltet.
   Eine Besitz-Struktur ausgehend von einem obersten Besitzer ist damit allgemein beschrieben.
   Es sollte einen obersten Besitzer geben (ein statisches Objekt).
*/


struct Sos_possessor;

struct Sos_possession
{
                                Sos_possession          ()              : _possessor_count ( 1 )  {}
                               ~Sos_possession          ()              { if( _possessor_count )  /*referenzen löschen*/; }

    void                        add_possessor           ()              { _possessor_count++; }
    void                        remove_possessor        ()              { if( --_possessor_count == 0 )  delete this; }

  private:
    int                        _possessor_count;
};

struct Sos_possessor
{
                                Sos_possessor           ()              {}
                               ~Sos_possessor           ()              { remove_possession(); }

    void                        add_possession          ( Sos_possession* );
    void                        remove_possession       ( Sos_possession* p ) { remove_possession( possession_index( p )); }
    void                        remove_possession       ( int i )             { _possession_array[ i ]->remove_possessor(); _possession_array[ i ] = 0; }
    void                        remove_possession       ();
    int                         possession_count        () const;
    int                         possession_index        ( Sos_possession* );

  private:
    int                        _possession_count;
    Sos_simple_array<Sos_possession*> _possession_array;
};
//---------------------------------------------------------------------------------Sos_registerable
/*
struct Sos_registerable
{
                                Sos_registerable        ()       : _register_index( 0 ) {}

    DECLARE_PUBLIC_MEMBER( int, register_index )
};
*/

