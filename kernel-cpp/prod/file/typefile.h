#ifndef __TYPEFILE_H
#define __TYPEFILE_H

#ifndef __ANYFILE_H
#   include <anyfile.h>
#endif

namespace sos {

//------------------------------------------------------------------Typed_file<OBJECT,OBJECT_FIELD>
/*
Dieser Typ beschreibt eine Datei von Objekten der Klasse OBJECT, auf die sequentiell
oder über Satznummern (noch nicht in Any_file implementiert) zugegriffen werden kann.

...
*/
//--------------------------------------Typed_indexed_file<OBJECT,OBJECT_FIELD,KEY,KEY_FIELD_DESCR>
/*
Dieser Typ beschreibt eine Datei von Objekten der Klasse OBJECT, auf die mit einem Schlüssel
KEY zugegriffen werden kann.

Template-Argumente:
    OBJECT:          Eine beliebige C++-Klasse, die den Satzaufbau der Datei widerspiegelt.
    OBJECT_FIELD:    Eine C++-Klasse mit Feldbeschreibungen (Field_descr_as<...>).
                     Beschreibt die tatsächliche Satzstruktur.
    KEY              Eine beliebige C++-Klasse, den den Satzschlüssel widerspiegelt.
    KEY_FIELD_DESCR: Field_descr_as<FELDTYP,OFFSET> Beschreibung des Schlüsselfeldes im Satz.

Beispiel:
    Typed_indexed_file< P01801, Ebcdic_p01801, Saacke_auftragsnr, Ebcdic_p01801::Key >

Siehe auch
    sosfield.h  für die abstrakten Feldbeschreibungen und
    ebcdifld.h  für vordefinierte Feldbeschreibungen im EBCDIC-Format.
*/



template< class OBJECT, class KEY >
struct Typed_indexed_file : Any_file
{
    BASE_CLASS( Any_file )

  // /*static const*/ File_spec    file_spec               ();

                                //Typed_indexed_file      ();
                                Typed_indexed_file      ( const Sos_ptr<Record_type>&, const Sos_ptr<Field_descr>& );
                               ~Typed_indexed_file      ();

    void                        open                    ( const char*, Open_mode, const File_spec& = std_file_spec );
    void                        open                    ( const Sos_string&, Open_mode, const File_spec& = std_file_spec );
    void                        get                     ( OBJECT* o )                           { Area area ( o, sizeof *o ); Base_class::get( &area ); }
    void                        get                     ( OBJECT* o, const KEY& k )             { get_key( o, k ); }
    void                        get_key                 ( OBJECT* o, const KEY& k )             { Area area ( o, sizeof *o ); Base_class::get_key( &area, Const_area( &k, sizeof k ) ); }
    void                        put                     ( const OBJECT& o )                     { Base_class::put( Const_area( &o, sizeof o ) ); }
  //void                        put                     ( const OBJECT&, const KEY& );
    void                        insert                  ( const OBJECT& o )                     { Base_class::insert( Const_area( &o, sizeof o ) ); }
    void                        store                   ( const OBJECT& o )                     { Base_class::store( Const_area( &o, sizeof o ) ); }
    //void                        insert                  ( const OBJECT& o, const KEY& k )       { Base_class::insert( Const_area( &o, sizeof o ), Const_area( &k, sizeof *k ) ); }
    void                        update                  ( const OBJECT& o )                     { Base_class::update( Const_area( &o, sizeof o ) ); }
    void                        set                     ( const KEY& k )                        { Base_class::set( Const_area( &k, sizeof k ) ); }
    void                        del                     ( const KEY& k )                        { Base_class::del( Const_area( &k, sizeof k ) ); }
    void                        del                     ()                                      { Base_class::del(); }
/*
    void                        insert                  ( const Const_area& r )             { Base_class::insert( r ); }
  //void                        insert                  ( const Const_area& r, const Key& k ) { Base_class::insert( r, k ); }
    void                        store                   ( const Const_area& r )             { Base_class::store( r ); }
    void                        update                  ( const Const_area& r )             { Base_class::update( r ); }
    void                        set                     ( const Key& k )                    { Base_class::set( k ); }
    void                        del                     ( const Key& k )                    { Base_class::del( k ); }
*/
  private:
  //void                        build_put_record        ( const OBJECT&, Area* );
  //void                        build_key_record        ( const KEY&,    Area* );

    //const OBJECT_FIELD&        _object_field_descr;
    //const KEY_FIELD_DESCR&     _key_field_descr;
    Sos_static_ptr<Record_type> _object_type;
    Sos_static_ptr<Field_descr> _key_type;
};


template< class OBJECT >
struct Typed_file : Any_file
{
    BASE_CLASS( Any_file )

  // /*static const*/ File_spec    file_spec               ();

                                //Typed_file              ();
                                Typed_file              ( const Sos_ptr<Record_type>& );
                               ~Typed_file              ();

    void                        open                    ( const char*, Open_mode, const File_spec& = std_file_spec );
    void                        open                    ( const Sos_string&, Open_mode, const File_spec& = std_file_spec );
    void                        get                     ( OBJECT* o )                           { Area area ( o, sizeof *o ); Base_class::get( &area ); }
    void                        put                     ( const OBJECT& o )                     { Base_class::put( Const_area( &o, sizeof o ) ); }
    void                        insert                  ( const OBJECT& o )                     { Base_class::insert( Const_area( &o, sizeof o ) ); }

    //void                        put                     ( const Const_area& area ) { Base_class::put( Const_area( &o, sizeof o ) );
  private:

    Sos_static_ptr<Record_type> _object_type;
};

} //namespace sos

//========================================================================================templates
#if defined SYSTEM_INCLUDE_TEMPLATES
    #include "typefile.tpl"
#endif

#endif
