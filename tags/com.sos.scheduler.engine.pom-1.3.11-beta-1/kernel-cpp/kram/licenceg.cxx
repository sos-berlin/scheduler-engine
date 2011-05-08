#include "precomp.h"
//#define MODULE_NAME "licenceg"
//#define COPYRIGHT   "(c)1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "sosstrng.h"
#include "sos.h"
#include "sosarray.h"
#include "soslimtx.h"
#include "sosopt.h"
#include "../file/absfile.h"
#include "../file/anyfile.h"
#include "licence.h"
#include "log.h"

namespace sos { 

#ifndef SYSTEM_WIN
/*
static void strupr( char* s )
{
    while( *s )  *s = toupper( (uint)*s ),  s++;
}
*/
#endif

//---------------------------------------------------------------------------------------remark
/*
static void remark( Record_type* t, const char* text ) 
{
    t->field_descr_ptr( t->field_count() - 1 )->_remark = text;
}
*/
//---------------------------------------------------------------------------Licence_product_record

struct Licence_product_record
{
    Sos_limited_text<2>        _code;
    Sos_limited_text<50>       _name;
    Sos_limited_text<50>       _value;
};

//-----------------------------------------------------------------------------Licence_key_file

struct Licence_key_file : Abs_file
{
  public:
                                Licence_key_file        ();

    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        get_record              ( Area& area );

    void                        get_master_licence      ( Sos_licence*, const string& aussteller );
    void                        check_rights            ();

  private:
    Fill_zero                  _zero_;
    Sos_licence                _licence;
    
    bool                       _installed_keys;         // Installierte Lizenzschlüssel liefern
    std::list<string>::iterator _installed_keys_it;

    Sos_licence                _products_of_licence;
    Sos_licence*               _products_of;
    int                        _products_of_idx;

    int                        _count;
};

//------------------------------------------------------------------------Licence_key_file_type

struct Licence_key_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "licence"; }
    virtual const char*         alias_name              () const { return "licence_key"; }

    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Licence_key_file> f = SOS_NEW( Licence_key_file() );
        return +f;
    }
};

const Licence_key_file_type    _licence_key_file_type;
const Abs_file_type&            licence_key_file_type = _licence_key_file_type;

//---------------------------------------------------------------------------------licence_int_as_alnum

extern char licence_int_as_alnum( int i );   // licence.cxx

//-----------------------------------------------------------------------------Sos_licence::key

Sos_string Sos_licence::key() const
{
    int                     i;
    Sos_seriennr*           s   = &_seriennr[ 1 ];
    Sos_limited_text<100>   key = s->_ausstellerkuerzel;

    Sos_date( s->_date.year(), s->_date.month(), s->_date.day() );  // Datum prüfen

    key += '-';
    key += s->_kundenkuerzel;
    key += '-';
    key += as_string( s->_lfdnr );
    key += '-';
    key += licence_int_as_alnum( s->_date.year() - 1990 );
    key += licence_int_as_alnum( s->_date.month() );
    key += licence_int_as_alnum( s->_date.day() );
    key += '-';

    for( i = _par.first_index(); i <= _par.last_index(); i++ ) {
        const char* p = (*this)[ i ];
        if( p ) {
            if( i >= base  ||  *p )  key += licence_int_as_alnum( i / base );
            key += licence_int_as_alnum( i % base );
            key += p;
            key += '-';
        }
    }

    if( _zz ) key += "ZZ-";

    key += _salz;

    uint4 k = berechneter_sicherungscode();
    for( i = 6; i > 0; i-- ) {
        key += licence_int_as_alnum( k % base );
        k /= base;
    }

    return as_string( key );
}

//-----------------------------------------------------------Licence_key_file::Licence_key_file

Licence_key_file::Licence_key_file()
:
    _zero_(this+1)
{
    _count = 1;
}

//-------------------------------------------------------------Licence_key_file::get_master_licence

void Licence_key_file::get_master_licence( Sos_licence* result, const string& aussteller )
{
    Any_file lics ( "licence -installed-keys" );
    while( !lics.eof() )
    {
        Sos_licence lic ( lics.get_string() );
        if( strcmp( lic._seriennr[1]._ausstellerkuerzel, "SOS" ) != 0 )  continue;
        if( strcmp( lic._seriennr[1]._kundenkuerzel, aussteller.c_str() ) != 0 )  continue;
        if( !lic[ licence_licence_key ] )  continue;

        result->merge_licence( lic );
    }

    if( result->empty() )  throw_xc( "SOS-1447" );
}

//-------------------------------------------------------------------Licence_key_file::check_rights

void Licence_key_file::check_rights()
{
    // Nur Codes sind erlaubt, die in installierten Schlüsseln mit Code 10 (Lizenzgenerator) vorkommen.
    // Und diese Schlüssel müssen als Kunden den zu generierenden Aussteller führen,
    // und diese Schlüssel müssen als Austeller SOS führen.

    Sos_seriennr* s = &_licence._seriennr[ 1 ];

    if( strcmp( s->_ausstellerkuerzel, "SOS" ) != 0 )
    {
        if( _licence[licence_licence_key] )  throw_xc( "SOS-1447" );
        if( _licence._zz                  )  throw_xc( "SOS-1447" );
    }

    Sos_licence master_licence;
    get_master_licence( &master_licence, s->_ausstellerkuerzel );

    for( int i = 0; i <= _licence._par.last_index(); i++ )
    {
        if( _licence[i] )
        {
            switch( i )
            {
                case licence_verfallsdatum_1900: throw_xc( "SOS-1447" );
                case licence_verfallsdatum_2000: break;
                case licence_os: break;

                default: if( _licence[i]  &&  !master_licence[i] )  throw_xc( "SOS-1447" );
            }
        }
    }

    if( master_licence[ licence_verfallsdatum_2000 ] )
    {
        if( !_licence[ licence_verfallsdatum_2000 ] )  throw_xc( "SOS-1447" );
        if( strcmp( _licence[ licence_verfallsdatum_2000 ], master_licence[ licence_verfallsdatum_2000 ] ) > 0 )  throw_xc( "SOS-1447" );
    }

    if( master_licence[ licence_os ] )
    {
        if( !_licence[ licence_os ] )  throw_xc( "SOS-1447" );
        const char* p = _licence[ licence_os ];
        while( *p )  if( !strchr( master_licence[licence_os], *p++ ) )  throw_xc( "SOS-1447" );
    }
}

//-----------------------------------------------------------------------Licence_key_file::open

void Licence_key_file::open( const char* fn, Open_mode open_mode, const File_spec& )
{
    if( !SOS_LICENCE( licence_licence_key ) )  throw_xc( "SOS-1000", "Lizenzgenerator" );

    bool products = false;

    _licence._seriennr.last_index( 1 );
    Sos_seriennr* s = &_licence._seriennr[ 1 ];

    if( open_mode & out )  throw_xc( "SOS-LICENCE-READONLY" );

    s->_date  = Sos_date::today();
    strcpy( s->_ausstellerkuerzel, "SOS" );
    strcpy( s->_kundenkuerzel    , "DEMO" );
    s->_lfdnr = 1;

    for( Sos_option_iterator opt( fn ); !opt.end(); opt.next() )
    {
        if( opt.flag( "installed-keys" ) )  _installed_keys = true,  _installed_keys_it = sos_static_ptr()->_licence->_installed_keys.begin();
        else
        if( opt.with_value( "products-of" ) )   _products_of_licence.read_key( opt.value().c_str() ),  _products_of = &_products_of_licence;
        else
        if( opt.flag( "products" ) )   products = true;
        else
        if( opt.with_value( "aussteller" ) ) {
            strcpy( s->_ausstellerkuerzel, c_str( opt.value() ) );
            strupr( s->_ausstellerkuerzel );
        }
        else
        if( opt.with_value( "kunde" ) )  {
            strcpy( s->_kundenkuerzel, c_str( opt.value() ) );
            strupr( s->_kundenkuerzel );
        }
        else
        if( opt.with_value( "nr" ) )        s->_lfdnr = as_int( opt.value() );
        else
        if( opt.with_value( "datum" ) )     s->_date.assign( opt.value() );
        else
        if( opt.with_value( "salz" ) )      _licence._salz = opt.value()[ 0 ];
        else
        if( opt.with_value( "count" ) )     _count = as_int( opt.value() );
        else
        if( opt.with_value( "19" )          // Ob -19=xx oder -20=xx, ist egal!
         || opt.with_value( "20" ) )
        {
            Sos_date date;
            date.assign(opt.value(), 90 );  // bis vor 90: 20xx, sonst 19xx
            Sos_limited_text<100> text;
            date.write_text( &text, "yyyymmdd" );

          //const char* p = opt.option();
            int n = Sos_licence::alnum_as_int( text[0] ) * base + Sos_licence::alnum_as_int( text[1] );
            _licence.set_par( n, c_str( text ) + 2 );
        }
        else 
        if( opt.flag( "ZZ" ) )              _licence._zz = opt.set();
        else {
            const char* p = opt.option();
            if( (uint)strlen( p ) - 1 <= 2 - 1 ) {
                int n = Sos_licence::alnum_as_int( p[0] );
                if( p[1] )  n = base*n + Sos_licence::alnum_as_int( p[1] );

                if( opt.flag( p ) )  _licence.set_par( n, "" );
                else
                if( opt.with_value( p ) )  _licence.set_par( n, c_str( opt.value() ) );
                else throw_sos_option_error( opt );
            }
            else throw_sos_option_error( opt );
        }
    }

    if( products )
    {
        if( s->_ausstellerkuerzel[0] )
        {
            get_master_licence( &_products_of_licence, s->_ausstellerkuerzel );
            _products_of = &_products_of_licence;
        }
        else
        {
            _products_of = +sos_static_ptr()->_licence;
        }
    }

    if( _installed_keys )
    {
        // nix
    }
    else
    if( _products_of )
    {
        Sos_ptr<Record_type>    t = SOS_NEW( Record_type );
        Licence_product_record* o = NULL;

        t->name( "Column" );
        t->allocate_fields( 3 );

        RECORD_TYPE_ADD_LIMTEXT     ( code    , 0 );
        RECORD_TYPE_ADD_LIMTEXT     ( name    , 0 );
        RECORD_TYPE_ADD_LIMTEXT     ( value   , 0 );

        _any_file_ptr->_spec._field_type_ptr = +t;
    }
    else
    {
        check_rights();
    }
}

//-----------------------------------------------------------------Licence_key_file::get_record

void Licence_key_file::get_record( Area& area )
{
    if( _installed_keys )
    {
        if( _installed_keys_it == sos_static_ptr()->_licence->_installed_keys.end() )  throw_eof_error();
        area.assign( *_installed_keys_it++ );
    }
    else
    if( _products_of )     // -products-of= ?
    {
        area.allocate_length( sizeof (Licence_product_record) );
        Licence_product_record* r = new (area.ptr()) Licence_product_record;
        
        while( _products_of_idx <= _products_of->_par.last_index()  &&  _products_of->_par[_products_of_idx].empty() )  _products_of_idx++;
        if( _products_of_idx > _products_of->_par.last_index() )  throw_eof_error();
        
        r->_code = Sos_licence::string_from_licence_int( _products_of_idx );
        r->_name = Sos_licence::component_name( _products_of_idx );
        if( _products_of->_par[_products_of_idx] != _products_of->_empty_value_string )  r->_value = _products_of->_par[_products_of_idx];

        _products_of_idx++;
    }
    else
    {
        if( _count == 0 )  throw_eof_error();

        if( !_licence._salz )  _licence._salz = licence_int_as_alnum( ( rand() ^ (int)time( NULL ) ) % base );

        area.assign( _licence.key() );

        _count--;
        _licence._salz = licence_int_as_alnum( ( rand() ^ (int)time( NULL ) ^ _licence._salz ) % base );
        _licence._seriennr[ 1 ]._lfdnr++;
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace sos
