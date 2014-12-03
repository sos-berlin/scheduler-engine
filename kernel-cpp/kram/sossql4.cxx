// $Id: sossql4.cxx 13579 2008-06-09 08:09:33Z jz $

#if defined _MSC_VER
#   pragma warning( disable:4018 )   // warning C4018: '>': Konflikt zwischen 'signed' und 'unsigned'
#endif

//#if defined __BORLANDC__
//#  define CHECK_STACK_OVERFLOW
//#endif

#include "precomp.h"

//#define MODULE_NAME "sossql4"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

/*
    Implementierte Klassen:

    Sossql_profile_catalog_file
    Sql_key_intervall
    Sql_table
    Sql_system_record


    Mögliche Erweiterungen:

    Nach einem vollständigen table scan:
        Für jede Spalte feststellen, ob sie aufsteigend oder absteigend geordnet,
        und ob sie eindeutig ist.


    Nicht bei gemeinschaftlichem Zugriff:

    Geordnete Spalten können als eine Art Index verwendet werden (wenn der Schlüssel
    nicht anwendbar ist): Intervalle über die geordnete Spalte werden gebildet;
    rewind() wird nur ausgeführt, wenn der Wert kleiner ist als der Wert an der
    aktuellen Dateiposition.

    Wenn eine Spalte eindeutig ist und auf Gleichheit geprüft wird, dann kann nach
    dem ersten Treffer abgebrochen werden.

    Wenn eine Spalte oft in der Where-Klasuel verwendet wird, dann kann ein temporärer
    Index aufgebaut werden (eingeschränkt bei gemeinschaftlichen Zugriff).
*/

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/sosarray.h"
#include "../kram/stdfield.h"
#include "../kram/soslimtx.h"
#include "../file/anyfile.h"
#include "../kram/sqlutil.h"
#include "../kram/sosprof.h"
#include "../kram/sosopt.h"
#include "../kram/sossql2.h"

using namespace std;
namespace sos {


const int default_key_display_size = 256;

//------------------------------------------------------------------Sossql_profile_catalog_file

struct Sossql_profile_catalog_file : Abs_file
{
    void                        prepare_open            ( const char*, Open_mode, const File_spec& );

  private:
    Any_file                   _file;
};

//------------------------------------------------------------------Sossql_profile_catalog_file

struct Sossql_profile_catalog_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "sossql_profile_catalog"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Sossql_profile_catalog_file> f = SOS_NEW( Sossql_profile_catalog_file );
        return +f;
    }
};

const Sossql_profile_catalog_file_type  _sossql_profile_catalog_file_type;
const Abs_file_type&                     sossql_profile_catalog_file_type = _sossql_profile_catalog_file_type;

//---------------------------------------------------------------------------------------------
// Minimum und Maximum für die Intervalle

const Byte low_value[ 256 ] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                              "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                              "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                              "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                              "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                              "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                              "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                              "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

const Byte high_value[ 256+1 ] = "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";

const Const_area sql_low_value  ( low_value , 256 );
const Const_area sql_high_value ( high_value, 256 );

//------------------------------------------------------------------Sossql_profile_catalog_file

void Sossql_profile_catalog_file::prepare_open( const char* param, Open_mode open_mode, const File_spec& )
{
    Sos_string          filename;
    Sos_string          section;
    Sos_option_iterator opt( param );

    for( ; !opt.end(); opt.next() )
    {
        if( opt.param( 1 ) )                   section = opt.value();
        else
        if( opt.param( 2 ) )                   filename = opt.value();
        else throw_sos_option_error( opt );
    }

    _file.prepare_open( "-key=TABLE_NAME "     // -key, damit Access einen Primärschlüssel hat
                        "select null TABLE_QUALIFIER, "
                               "null TABLE_OWNER,"
                               "entry TABLE_NAME,"
                               "'TABLE' TABLE_TYPE,"
                               "value REMARKS,"
                               "value FILE"
                        " | profile -section=" + quoted_string( section ) + " " + filename,
                        open_mode );


    _any_file_ptr->new_file( &_file );
}

//---------------------------------------------------------------------------------print_ebcdic

void print_ebcdic( ostream* s, const void* p, int len )
{
    if( !len )  return;
    const Byte* b = (const Byte*)p;

    if( memcmp( b, sql_low_value.ptr(), len ) == 0 )  *s << "<low value>";
    else
    if( memcmp( b, sql_high_value.ptr(), len ) == 0 )  *s << "<high value>";
    else
    {
        const Byte* b_end = b + len;
        while( b < b_end )  s->put( (char) ebc2iso[ *b++ ] );
    }
}

//---------------------------------------------------------------------------------print_ebcdic

inline void print_ebcdic( ostream* s, const Const_area& a )
{
    print_ebcdic( s, a.ptr(), a.length() );
}

//-----------------------------------------------------------------------------------is_join_op

Bool is_join_op( Sql_operator op )
{
    return op == op_lt || op == op_le || op == op_eq || op == op_ne || op == op_ge || op == op_gt
        || op == op_like || op_between;
}

//-------------------------------------------------------------------cmp_operator(Sql_operator)
/*
static Bool cmp_operator( Sql_operator op )
{
    return op == op_lt || op == op_le || op == op_eq || op == op_ne || op == op_ge || op == op_gt;
}
*/
//---------------------------------------------------------Sql_key_intervall::Sql_key_intervall

Sql_key_intervall::Sql_key_intervall()
:
    _offset      ( 0 ),
    _length      ( 0 ),
    _same        ( false )
{
}

//---------------------------------------------------------Sql_key_intervall::Sql_key_intervall

Sql_key_intervall::Sql_key_intervall( const Sql_key_intervall& o )
:
    _offset      ( o._offset ),
    _length      ( o._length ),
    _lowest_key  ( o._lowest_key ),
    _highest_key ( o._highest_key ),
    _same        ( o._same )
{
    //obj_const_name( "Sql_key_intervall" );
}

//---------------------------------------------------------Sql_key_intervall::Sql_key_intervall

Sql_key_intervall::Sql_key_intervall( const Const_area& low, Bool low_incl,
                                      const Const_area& high, Bool high_incl,
                                      uint len, uint offset )
{
    //obj_const_name( "Sql_key_intervall" );

    assign( low, low_incl, high, high_incl, len, offset );
}

//---------------------------------------------------------Sql_key_intervall::Sql_key_intervall

Sql_key_intervall::Sql_key_intervall( const Const_area& key, uint len, uint offset )
{
    assign( key, len, offset );
}

//--------------------------------------------------------------------Sql_key_intervall::assign

void Sql_key_intervall::assign( const Const_area& low, Bool low_incl,
                                const Const_area& high, Bool high_incl,
                                uint len, uint offset )
{
    //obj_const_name( "Sql_key_intervall" );

    // wird bei <, <=, ==, >=,> und LIKE gerufen

    assert( low.length() == high.length() );

    _offset = offset;
    _length = min( offset + low.length(), len );       
    _same = false;

    _lowest_key .allocate_length( len );
    _highest_key.allocate_length( len );

    _lowest_key .fill( '\x00' );
    _highest_key.fill( '\xFF' );

    memcpy( _lowest_key .byte_ptr() + offset, low .ptr(), _length - offset );
    memcpy( _highest_key.byte_ptr() + offset, high.ptr(), _length - offset );

    if( !low_incl )  {
        _lowest_key.length( min( offset + low.length(), len ) );
        incr( &_lowest_key );
        _lowest_key .length( len );
    }

    if( !high_incl ) {
        _highest_key.length( min( offset + high.length(), len ) );
        decr( &_highest_key );
        _highest_key.length( len );
    }
}

//--------------------------------------------------------------------Sql_key_intervall::assign

void Sql_key_intervall::assign( const Const_area& key, uint len, uint offset )
{
    //obj_const_name( "Sql_key_intervall" );

    // wird bei <, <=, ==, >=,> und LIKE gerufen

    _offset = offset;
    _length = min( offset + key.length(), len );

    _lowest_key.allocate_length( len );
    _lowest_key.fill( '\x00' );
    memcpy( _lowest_key.byte_ptr() + offset, key.ptr(), _length - offset );

    if( key.length() >= len ) 
    {
        _same = true;
    } 
    else 
    {
        _same = false;
        _highest_key.allocate_length( len );
        _highest_key.fill( '\xFF' );
        memcpy( _highest_key.byte_ptr() + offset, key.ptr(), _length - offset );
    }
}

//--------------------------------------------------------Sql_key_intervall::~Sql_key_intervall

Sql_key_intervall::~Sql_key_intervall()
{
}

//-----------------------------------------------------------------Sql_key_intervall::operator=

Sql_key_intervall& Sql_key_intervall::operator= ( const Sql_key_intervall& o )
{
    _offset = o._offset;
    _length = o._length;
    _same   = o._same;
    _lowest_key._assign( o._lowest_key );
    _highest_key._assign( o._highest_key );
    return *this;
}

//----------------------------------------------------------------operator<<(Sql_key_intervall)

ostream& operator<< ( ostream& s, const Sql_key_intervall& ki )
{
    int i;

    s.put( '[' );
    for( i = 0; i < ki._offset; i++ )  s << '_';
    print_ebcdic( &s, ki.lowest_key().byte_ptr() + ki._offset, ki.lowest_key().length() - ki._offset );
    s << ',';
    for( i = 0; i < ki._offset; i++ )  s << '_';
    print_ebcdic( &s, ki.highest_key().byte_ptr() + ki._offset, ki.highest_key().length() - ki._offset );
    s.put( ']' );

    return s;
}

//---------------------------------------------------------------operator<<(Sql_key_intervalle)

ostream& operator<< ( ostream& s, const Sql_key_intervalle& ki )
{
    for( int i = ki.first_index(); i <= ki.last_index(); i++ ) {
        s << ki[ i ] << "\n";
    }

    return s;
}

//---------------------------------------------------------------------------cut(Sql_key_intervall)
// Schnitt
/*
void cut( Sql_key_intervall* a, const Sql_key_intervall& b )
{
    if( a._same ) {
        a._highest_key = a._lowest_key;
        a._same = false;
    }

    if( a._offset < b._offset )
    {
        if( memcmp( a._lowest_key.byte_ptr() + b._offset, b._lowest_key.byte_ptr() + b._offset, b._length - b._offset ) < 0 )  
            memcpy( a._lowest_key.byte_ptr() + b._offset, b._lowest_key.byte_ptr() + b._offset, b._length - b._offset );

        if( memcmp( a._highest_key.byte_ptr() + b._offset, b._highest_key.byte_ptr() + b._offset, b._length - b._offset  ) > 0 )  
            memcpy( a._highest_key.byte_ptr() + b._offset, b._highest_key.byte_ptr() + b._offset, b._length - b._offset  );

        if( a._length < b._length )  a._length = b._length;
    }
    else
    if( a._offset > b._offset )
    {
        if( memcmp( a._lowest_key.byte_ptr() + a._offset, b._lowest_key.byte_ptr() + a._offset, a._length - a._offset ) < 0 )  
            memcpy( a._lowest_key.byte_ptr() + a._offset, b._lowest_key.byte_ptr() + a._offset, a._length - a._offset );

        if( memcmp( a._highest_key.byte_ptr() + a._offset, b._highest_key.byte_ptr() + a._offset, a._length - a._offset ) > 0 )  
            memcpy( a._highest_key.byte_ptr() + a._offset, b._highest_key.byte_ptr() + a._offset, a._length - a._offset );

        if( a._length < b._length )  a._length = b._length;
    }
    else
    {
        if( a._lowest_key  < b._lowest_key  )  a._lowest_key = b.lowest_key();
        if( a._highest_key > b._highest_key )  a._highest_key = b.highest_key();
    }

    if( a._lowest_key > a._highest_key )  a._highest_key = a._lowest_key;  // 16.1.2002: Solange Zugriffsplan nicht korrekt erstellt wird (Reihenfolge der Tabellenzugriffe), kann das passieren.

    return a;
}
*/
//----------------------------------------------------------------operator&=(Sql_key_intervall)
// Schnitt

Sql_key_intervall& operator&= ( Sql_key_intervall& a, const Sql_key_intervall& b )
{
    if( a._lowest_key.length()  != b._lowest_key.length()  )  throw_xc( "SOS-SQL-94", a._lowest_key.length() , b._lowest_key.length()  );
    if( a._highest_key.length() != b._highest_key.length() )  throw_xc( "SOS-SQL-94", a._highest_key.length(), b._highest_key.length() );

    if( a._same ) {
        a._highest_key = a._lowest_key;
        a._same = false;
    }


    if( a._offset < b._offset )
    {
        if( memcmp( a._lowest_key.byte_ptr() + b._offset, b._lowest_key.byte_ptr() + b._offset, b._length - b._offset ) < 0 )  
            memcpy( a._lowest_key.byte_ptr() + b._offset, b._lowest_key.byte_ptr() + b._offset, b._length - b._offset );

        if( memcmp( a._highest_key.byte_ptr() + b._offset, b._highest_key.byte_ptr() + b._offset, b._length - b._offset  ) > 0 )  
            memcpy( a._highest_key.byte_ptr() + b._offset, b._highest_key.byte_ptr() + b._offset, b._length - b._offset  );

    }
    else
    if( a._offset > b._offset )
    {
        memcpy( a._lowest_key .byte_ptr() + b._offset, b._lowest_key .byte_ptr() + b._offset, a._offset - b._offset );
        memcpy( a._highest_key.byte_ptr() + b._offset, b._highest_key.byte_ptr() + b._offset, a._offset - b._offset );

        if( memcmp( a._lowest_key.byte_ptr() + a._offset, b._lowest_key.byte_ptr() + a._offset, a._length - a._offset ) < 0 )  
            memcpy( a._lowest_key.byte_ptr() + a._offset, b._lowest_key.byte_ptr() + a._offset, a._length - a._offset );

        if( memcmp( a._highest_key.byte_ptr() + a._offset, b._highest_key.byte_ptr() + a._offset, a._length - a._offset ) > 0 )  
            memcpy( a._highest_key.byte_ptr() + a._offset, b._highest_key.byte_ptr() + a._offset, a._length - a._offset );

        a._offset = b._offset;
    }
    else
    {
        if( a._lowest_key  < b._lowest_key  )  a._lowest_key = b.lowest_key();
        if( a._highest_key > b._highest_key )  a._highest_key = b.highest_key();
    }

    
    if( a._length < b._length )  a._length = b._length;

    if( a._lowest_key > a._highest_key )  a._highest_key = a._lowest_key;  // 16.1.2002: Solange Zugriffsplan nicht korrekt erstellt wird (Reihenfolge der Tabellenzugriffe), kann das passieren.

    return a;
}

//-----------------------------------------------------------------------exchange_key_intervall

void exchange_key_intervall( Sql_key_intervall* a, Sql_key_intervall* b )
{
    exchange( &a->_offset     , &b->_offset     );
    exchange( &a->_same       , &b->_same       );
    exchange( &a->_lowest_key , &b->_lowest_key );
    exchange( &a->_highest_key, &b->_highest_key );
}

//----------------------------------------------------------normalize_ored(Sql_key_intervalle*)
// Vereinigung aller Intervalle vereinfachen

void normalize_ored( Sql_key_intervalle* ki_ptr )
{
    //int QUADRATISCHER_AUFWAND_BEI_INTERVALLEN;

    Sql_key_intervalle& ki = *ki_ptr;
  //Sql_key_intervall   empty_intervall;

    // Sortieren
    // Folgende Schleife hat QUADRATISCHEN AUFWAND, was bei mehreren Hundert Intervallen bemerkbar wird:   qsort!
    int i;
    for( i = 0; i < ki.count() - 1; i++ ) {
        Sql_key_intervall* mini = &ki[ i ];
        for( int j = i + 1; j < ki.count(); j++ ) {
            Sql_key_intervall* k = &ki[ j ];
            int len = min( k->lowest_key().length(), mini->lowest_key().length() );
            if( len ) {
                if( memcmp( k->lowest_key().ptr(), mini->lowest_key().ptr(), len ) < 0
                 || k->lowest_key().length() < mini->lowest_key().length()             )  mini = k;
            }
            //if( k->lowest_key() < mini->lowest_key() )  mini = k;
        }
        if( mini != &ki[ i ] )  exchange( mini, &ki[ i ] );
    }

    // Folgende Schleife hat QUADRATISCHEN AUFWAND, was bei mehreren Hundert Intervallen bemerkbar wird:
    // Lösung: Löcher markieren und in einem zweiten Durchlauf entfernen.
    // Und last_index() nur einmal rufen, damit Speicher nur einmal freigegegeben wird.

    // Überlappungen zusammenfassen:
    for( i = ki.count() - 2; i >= 0; i-- ) {
        if( ki[ i ].highest_key() >= ki[ i+1 ].lowest_key() ) {
            ki[ i ].highest_key( ki[ i + 1 ].highest_key() );
            for( int j = i+1; j < ki.count() - 1; j++ ) {
                ki[ j ] = ki[ j+1 ];
            }
            ki.last_index( ki.last_index() - 1 );
        }
    }
}

//-------------------------------------------------------------------------Sql_table::Sql_table

Sql_table::Sql_table()
:
    _zero_(this+1)
  //_index(-1),
  //_open_mode ( Any_file::Open_mode( Any_file::in | Any_file::seq ) )
  //_open_mode ( Any_file::Open_mode( Any_file::inout | Any_file::nocreate ) )
                                // damit comfile die Datei für getkey nutzen läßt
{
    obj_const_name( "Sql_table" );

    _key_intervalle.obj_const_name( "Sql_table::_key_intervalle" );
    _key_intervalle.increment( 100 );
    _selected_type = Record_type::create();
}

//------------------------------------------------------------------------Sql_table::~Sql_table

Sql_table::~Sql_table()
{
/*
    if( _index >= 0 ) {
        int i = _index;  _index = -1;
        Sossql_static* st = sos_static_ptr()->_sossql;      // Falls falsche Reihenfolge beim shut down
      //if( st )  st->_tables[ i ] = 0;
    }
*/
}

//--------------------------------------------------------------Sql_table::is_primary_key_field

Bool Sql_table::is_primary_key_field( const Field_descr* field_descr )
{
    if( !_key_in_record )  return false;
    if( !_key_len       )  return false;
    if( uint( field_descr->offset() - _key_pos ) >= (uint)_key_len ) return false; // Kein Schlüsselfeld?

    return true;
}

//--------------------------------------------------------------Sql_table::is_primary_key_field

Bool Sql_table::is_primary_key_field( const Sql_field_expr* expr )
{
    if( +expr->_table != this )         return false;
    if( expr->_index_array.count() )    return false;    // Indiziertes Feld?
    return is_primary_key_field( expr->_field_descr );
}

//-------------------------------------------------------------Sql_table::is_secondary_key_field

Bool Sql_table::is_secondary_key_field( const Sql_field_expr* )
{
    return false;
/*
    if( !_key_in_record )               return false;
    if( !_key_len       )               return false;
    if( +expr->_table != this )         return false;
    if( expr->_index_array.count() )    return false;    // Indiziertes Feld?

    int n = _file.no_of_secondary_indices();
    for( int i = 1; i <= n; i++ ) {
        if( uint( expr->_field_descr->offset() - _file.index( i )._key_pos ) >= (uint)_file.index( i )._key_len ) return false; // Kein Schlüsselfeld?
    }

    return true;
*/
}

//--------------------------------------------------------------------------Sql_table::set_join

void Sql_table::set_join( Sql_expr* expr )
{
    if( !is_join_op( expr->_operator ) )  throw_xc( "Sql_table::set_join", "Kein Join-Operator!" );

    Sql_expr_with_operands* e = (Sql_expr_with_operands*)expr;
    Sql_field_expr*         f = (Sql_field_expr*) +e->_operand_array[ 0 ];

    if( f->_operator != op_field )  throw_xc( "Sql_table::set_join", "Interner Fehler: Links ist kein Feld!" );

    if( _build_expr_ptr )  _build_expr_ptr->_operand_array.add( expr );
                     else  _join_expr = expr;
    //else throw_xc( "SOS-SQL-78", expr, c_str( _name ) );  // Doppelt?

    for( int i = 1; i <= e->_operand_array.last_index(); i++ ) {
        _master_tables |= e->_operand_array[ i ]->_used_tables;
    }

    if( is_primary_key_field( f )  &&  f->_field_descr->offset() == f->_table->_key_pos ) {
        _key_join = true;
        LOG( "Join erkannt: " << *expr << "; Direkter Zugriff auf Tabelle " << f->_table->simple_name() << '\n' );
        if( expr->_operator == op_eq )  _equijoin = true;
    }
}

//-------------------------------------------------------------------Sql_table::begin_join_expr
// Baut die Struktur des Join-Ausdrucks auf.
// Wird von Sql_stmt::prepare_expr() gerufen.

void Sql_table::begin_join_expr( Sql_operator op )
{
    // Wird in der Reihenfolge der Polnischen Notation aufgerufen.
    if( op != op_or  &&  op != op_and  &&  op != op_not )  throw_xc( "Sql_table::extend_join_expr", sql_op_text[ op ] );
/*
    Sql_expr_with_operands* e = _join_expr;

    // Das äußerste rechte Blatt des bisherigen Ausdrucks suchen mit expr->_operator vor op
    if( e ) {
        if( op == op_or )  throw_xc( "Sql_table::extend_join_expr", "2 mal OR!" );
        while( e->_operand_array.count() ) {
            Sql_expr_with_operands* child = (Sql_expr_with_operands*)e->_operand_array[ e->_operand_array.last_index() ];
            if( child->priority() >= Sql_expr::priority( op ) )  break;
            e = child;
        }
    }
*/
    Sos_ptr<Sql_expr_with_operands> expr = SOS_NEW( Sql_expr_with_operands( op ) );

    if( _build_expr_ptr )  _build_expr_ptr->_operand_array.add( +expr );
                     else  _join_expr = expr;

    _build_expr_ptr = expr;

    //LOG( "Sql_table[" << _name << "]::begin_join_expr(" << op << ")  ==>  " << *_join_expr << '\n' );
}

//---------------------------------------------------------------------Sql_table::end_join_expr

void Sql_table::end_join_expr()
{
    // Einen Konten im Ast steigen

    Sql_expr_with_operands* parent = NULL;
    Sql_expr_with_operands* e      = (Sql_expr_with_operands*)+_join_expr;

    while( e != _build_expr_ptr ) {
        parent = e;
        e = (Sql_expr_with_operands*)+e->_operand_array[ e->_operand_array.last_index() ];
    }

    if( e->_operand_array.count() == 0 ) {
        // Leeren Knoten löschen:
        //LOG( "Sql_table::end_join_expr: deleting empty node\n" );
        if( parent )  parent->_operand_array.last_index( parent->_operand_array.last_index() - 1 );
                else  _join_expr = NULL;
    }
    else
    if( e->_operand_array.count() == 1  &&  e->_operator != op_not )
    {
        // Überflüssigen Knoten entfernen:
        //LOG( "Sql_table::end_join_expr: deleting useless node\n" );
        if( parent )  parent->_operand_array[ parent->_operand_array.last_index() ] = e->_operand_array[ 0 ];
                else  _join_expr = +e->_operand_array[ 0 ];
    }

    _build_expr_ptr = parent;

    //LOG( "Sql_table[" << _name << "]::end_join_expr()  ==>  " );
    //if( _join_expr )  { LOG( *_join_expr << '\n' ); }
    //            else  LOG( "---\n" );
    if( _join_expr )  LOG( "Join-Ausdruck für Tabelle " << simple_name() << ": "  << *_join_expr << '\n' );
}

//--------------------------------------------------------------Sql_table::single_key_intervall
// Speichert in expr->_value true, wenn ein Intervall erzeugt wird!

void Sql_table::add_key_intervalls_cmp( Sql_key_intervalle* iv, Sql_expr* expr, Bool single_only )
{
    uint key_offset = 0;
    int  last_index = iv->last_index();

    Sql_operator op = expr->_operator;

    if( !_key_in_record )  goto NIX;
    if( !_key_len       )  goto NIX;


    if( op==op_lt || op==op_le || op==op_eq || op==op_ne || op==op_ge || op==op_gt
     || op==op_is_null || op==op_like || op==op_between )
    {
        Dynamic_area                  value_as_text;
        const Sql_expr_with_operands* e   = (Sql_expr_with_operands*)expr;
        const Sql_expr*               op0 = e->_operand_array[ 0 ];

        if( op0->_operator != op_field )  goto NIX;

        const Sql_field_expr* f = (Sql_field_expr*)+op0;
        if( !is_primary_key_field( f ) )  goto NIX;

        key_offset = f->_field_descr->offset() - _key_pos;

        const Field_type* type = f->_field_descr->type_ptr();

        if( type->obj_is_type( tc_Record_type )  &&  ((Record_type*)type)->_group_type ) {      //jz 2.10.2000
            type = ((Record_type*)type)->_group_type;  
        }

        if( op == op_is_null )
        {
            if( !type->nullable() )  goto EMPTY;
            Dynamic_area value ( type->field_size() );
            type->set_null( value.byte_ptr() );
            //iv->add( Sql_key_intervall( value, true , value, true, _key_len, key_offset ) );
            Sql_key_intervall* v = iv->add_empty();
            v->assign( value, true , value, true, _key_len, key_offset );
            return;
        }

        Sql_expr*       op1     = e->_operand_array[ 1 ];
        Dynamic_area    value   ( type->field_size() + 1 );    // Endgültige binäre Darstellung des Schlüsselwertes

        if( op == op_like  ||  op == op_between  )
        {
            Dyn_obj val = _stmt->eval_expr( op1 );        // Der Ausdruck wird doppelt ausgerechnet ...

            if( val.null() )  goto EMPTY;
            value_as_text.allocate_min( default_key_display_size+1 );
            val.write_text( &value_as_text );
            value_as_text.resize_min( value_as_text.length() + 1 );
            value_as_text.char_ptr()[ value_as_text.length() ] = '\0';

            if( op == op_like )
            {
                // Typ muß eine Zeichendarstellung sein, etwa Cobol PIC X(n) oder PIC 9(n);
                // gepackte Zahl z.B. ist nicht möglich.
                if( !type->info()->_exact_char_repr )  goto NIX;

                int like_len = min( position( value_as_text.char_ptr(), '%' ),
                                    position( value_as_text.char_ptr(), '_' ) );
                value.length( like_len <= type->field_size()? like_len : type->field_size() );
                Text_format format = raw_text_format;
                format.text( true );  // Eine Zahl wird RECHTS mit Nullen aufgefüllt: "010" in fünfstelliges Feld ergibt 1000.

                if( like_len >= value_as_text.length() )  // LIKE ohne Wildcard?
                {
                    value.length( type->field_size() + 1 );      // Mit Blanks auffüllen ( + '\0' )
                    memset( value.char_ptr() + like_len, ' ', type->field_size() - like_len );
                    like_len = type->field_size();
                }
                value_as_text.char_ptr()[ like_len ] = '\0';

                type->read_text( value.byte_ptr(), value_as_text.char_ptr(), format );

                //LOG( "single_key_intervall like: offset=" << key_offset << " len=" << v.length() << " value=\"" << Const_area( value_as_text.ptr(), v.length() ) << "\"\n" );

                Sql_key_intervall* v = iv->add_empty();
                v->assign( value, true, value, true, _key_len, key_offset );
            }
            else
            if( op == op_between )
            {
                Dynamic_area value2 ( type->field_size() );
                Dyn_obj      val2   = _stmt->eval_expr( e->_operand_array[ 2 ] );        // Der Ausdruck wird doppelt ausgerechnet ...

                type->read_text( value.byte_ptr(), value_as_text.char_ptr() );
                value.length( type->field_size() );

                if( val2.null() ) goto EMPTY;
                val2.write_text( &value_as_text );
                value_as_text += '\0';
                type->read_text( value2.byte_ptr(), value_as_text.char_ptr() );
                value2.length( type->field_size() );

                Sql_key_intervall* v = iv->add_empty();
                v->assign( value, true, value2, true, _key_len, key_offset );
            }
        }
        else
        {
            Dyn_obj          b;                                     // Ein rechter Operand, also ein Schlüsselwert
            Sql_select_expr* select_expr = NULL;
            int              i           = 1;   // if !select_expr
            Dynamic_area     buffer;            // if select_expr
            Sql_expr*        op1         = e->_operand_array[ 1 ];

            if( op1->_operator == op_select ) {
                select_expr = (Sql_select_expr*)op1;
                select_expr->_select.execute();
                iv->increment( 1000 );      // Bei select werden's wohl ein paar Intervalle mehr
            }

            value_as_text.allocate_min( default_key_display_size+1 );


            while(1) {
                if( select_expr ) {
                    try { select_expr->_select.fetch( &buffer ); }
                    catch( const Eof_error& ) { break; }
                    b.assign( select_expr->_select._result_record_type->field_descr_ptr( 0 ), buffer.ptr() );
                } else {
                    if( i > e->_operand_array.last_index() )  break;
                    _stmt->eval_expr( &b, e->_operand_array[ i++ ] );
                }

                if( !b.null() )
                {
                    Sql_key_intervall* v;
                    b.write_text( &value_as_text );  value_as_text += '\0';
                    type->read_text( value.byte_ptr(), value_as_text.char_ptr() );
                    int l = type->field_size();
                    value.length( l );


                    switch( op )
                    {
                        case op_lt: v = iv->add_empty();  v->assign( Const_area( sql_low_value.ptr(), l ), true , value                                , false , _key_len, key_offset );  break;
                        case op_le: v = iv->add_empty();  v->assign( Const_area( sql_low_value.ptr(), l ), true , value                                , true  , _key_len, key_offset );  break;
                        case op_ne: v = iv->add_empty();  v->assign( Const_area( sql_low_value.ptr(), l ), true , value                                , false , _key_len, key_offset );
                                    v = iv->add_empty();  v->assign( value                               , false, Const_area( sql_high_value.ptr(), l ), true  , _key_len, key_offset );  break;
                        case op_eq: v = iv->add_empty();  v->assign( value                               ,                                                       _key_len, key_offset );  break;
                        case op_ge: v = iv->add_empty();  v->assign( value                               , true , Const_area( sql_high_value.ptr(), l ), true  , _key_len, key_offset );  break;
                        case op_gt: v = iv->add_empty();  v->assign( value                               , false, Const_area( sql_high_value.ptr(), l ), true  , _key_len, key_offset );  break;
                        default   : throw_xc( "Sql_table::add_key_intervalls_cmp" );
                    }
                }
            }
        }

        if( single_only  &&  iv->last_index() > last_index + 1 ) {    // Nur ein Intervall erlaubt, aber mehrere erzeugt?
            // WHERE expr AND expr IN (list) kann derzeit nicht optimiert werden, denn das ist zu komplex ( entspricht: expr AND ( expr OR expr ) )
            iv->last_index( last_index );                                 // Wieder löschen.
            //LOG( "last_index=" << last_index << ", iv->last_index=" << iv->last_index() << "\n" );
            LOG( *this << ".add_key_intervall_cmp zu komplex: " << *expr << '\n' );
            return;
        }

/*16.1.2002: Zur Zeit werden die Tabellen nicht richtig geordnet, also der Zugriffsplan nicht korrekt erstellt.
             Bei select * from b,a where a.a=b.a and a.a=2 wird falsch zuerst Tabelle B, dann A gelesen.
             Geliefert werden wilde Kombinationen aus A und B.
             Deshalb soll wenigstens nicht noch die Where-Klausel geprüft werden. Deshalb auskommentiert.

        if( key_offset == 0 ) {
            const Bool t = true;
            expr->_value.assign( &bool_type, &t );
            expr->_text     = "1";              // ist das nötig?
            expr->_null     = false;
            expr->_computed = true;
        } else {
            // Alle nachrangigen Schlüsselsegmente über _where-Klausel auswerten.
            // (das kann optimieren wer will).
        }
*/
    }

    return;

  EMPTY: ;  // where KEY = NULL => leeres Intervall.
    {
        Sql_key_intervall* v = iv->add_empty();
        v->assign( sql_low_value, false, sql_low_value, false, _key_len, key_offset );  // Leer
        expr->_value    = null_dyn_obj;
        expr->_null     = true;
        expr->_computed = true;
        return;
    }

  NIX: ;    // Ausdruck ist nicht für direkten Zugriff verwertbar (unabhängig von den Daten)
    return; // Sql_key_intervall( sql_low_value, true, sql_high_value, true, _key_len, 0 );
}

//----------------------------------------------------------------Sql_table::add_key_intervalls

void Sql_table::add_key_intervalls( Sql_key_intervalle* intervall_array, Sql_expr* expr )
{
    if( !_key_in_record )  return;

    if( expr )  add_key_intervalls2( intervall_array, expr );

    if( intervall_array->count() == 0 ) 
    {
        //jz 5.12.97 Wenn kein Intervall da ist, dann heißt das: alle Sätze lesen
        intervall_array->add( Sql_key_intervall( Const_area( low_value, _key_len ), true, 
                                                 Const_area( high_value, _key_len ), true, 
                                                 _key_len, 0 ) );
    }
}


//---------------------------------------------------------------Sql_table::add_key_intervalls2

void Sql_table::add_key_intervalls2( Sql_key_intervalle* intervall_array, Sql_expr* expr )
{
    try {
        if( expr->_operator == op_and )
        {
            const Sql_expr_with_operands* e   = (Sql_expr_with_operands*)+expr;
            uint                          len = _key_len;
            Sql_key_intervalle            iv;
            Sql_key_intervall             intervall;
            Bool                          intervall_valid = false;

            iv.increment( 2 );

            if( len == 0  ||  !_key_in_record )  len = sql_low_value.length();

            //Sql_key_intervall intervall = single_key_intervall( e->_operand_array[ 0 ] );

            for( int i = 0; i <= e->_operand_array.last_index(); i++ )
            {
                int previous_last_index = iv.last_index();

                add_key_intervalls_cmp( &iv, e->_operand_array[ i ], true );   // Nebenwirkung: In e->_operand_array[ i ] wird errechnetes Ergebnis (meistens true) gespeichert

                if( iv.last_index() > previous_last_index )
                {
                    if( !intervall_valid ) {
                        // Überflüssige &= vermeiden, denn jedes &= lässt lowest_key und highest_key getrennt speichern, auch wenn die Werte gleich sind.
                        intervall = iv[ iv.last_index() ];
                        intervall_valid = true;
                    } else {
                        intervall &= iv[ iv.last_index() ];
                    }

                    iv.last_index( 0 );     // iv[0] bleibt stehen, damit nicht immer wieder Speicher freigegeben und angefordert wird.
                }
            }


            if( intervall_valid )  intervall_array->add( intervall );

/*jz 13.4.97 Baustelle:
            Sos_key_intervall   iv;      // Einfache Intervalle werden sofort verundet.
            Sos_key_intervalle  ored;    // Ein geodertes Intervalle ist möglich; ( a and (b or c) => a and b or a and c )

            for( int i = 0; i <= e->_operand_array.last_index(); i++ ) {
                add_key_intervall_cmp( k );
                if( k->count() <= 0 ) {
                    if( k->count() <= 1 )  iv &= (*k)[ 0 ];
                    ored.last_index( ored.last_index() - 1 );
                }
            }

            if( ored.count() ) {     // Ausmultiplizieren:
                ausmultiplizieren( intervall_array, ored.count() );
            }
            else
            {
                intervall_array->add( intervall );
            }
*/
        }
        else
        if( expr->_operator == op_or ) {
            const Sql_expr_with_operands* e = (Sql_expr_with_operands*)+expr;

            for( int i = e->_operand_array.first_index(); i <= e->_operand_array.last_index(); i++ ) {
                add_key_intervalls( intervall_array, e->_operand_array[ i ] );
            }
        }
        else
        {
            add_key_intervalls_cmp( intervall_array, expr );
        }
    }
    catch( const Overflow_error& )  //  < low_value oder > high_value  ==> leeres Intervall
    {
        // nix
    }

//LOG( "Sql_table::vor normalized_ored  _param_table->_table_type=" << (void*)+_stmt->_param_table->_table_type << '\n' );
    normalize_ored( intervall_array );
}

//-------------------------------------------------------------------------------collect_fields
/*
void collect_fields( Sos_array<Field_descr*>* field_array, int key_pos, int key-len, Field_descr* key_descr )
{
    for( int i =
}
*/
//-------------------------------------------------------------------Sql_table::print_intervall

void Sql_table::print_intervall( ostream* s, const Sql_key_intervall& intervall )
{
    // Felder mit offset() >= _key_pos + intervall._offset und offset() + type().field_size() <= _key_pos + _key_len ) ermitteln.
    // Felder die vorne oder hinten überhängen, werden nicht ausgegeben, weil deren Werte nicht vollständig bekannt ist.
    // Die vollständigen, evtl. überhängenden Werte müssten im Sql_key_intervall mitgeführt werden
    // (aber wie solche Intervalle vereinigen?.

    //Sos_simple_array<Field_descr*> field_array;
    //collect_fields( &field_array, _key_pos, _key_len, _table_key_descr );

    if( !_table_key_descr ) {
        // _key_len > 0 aber keine Schlüsselfelder? Sollte eigentlich nicht passieren.
        *s << '[' << hex << intervall.lowest_key() << ',' << intervall.highest_key() << ']';
        return;
    }

    Record_type* key_type = SOS_CAST( Record_type, _table_key_descr->type_ptr() );

    for( int i = 0; i < key_type->field_count(); i++ )
    {
        if( i > 0 )  *s << " and ";
        Field_descr* f = key_type->field_descr_ptr( i );
        int pos = f->offset();
        int len = f->type().field_size();
        *s << f->name();
        int cmp =  memcmp( intervall.lowest_key().byte_ptr() + pos,
                           intervall.highest_key().byte_ptr() + pos, len );
        *s << ( cmp == 0? "=" : " between " );
        try {
            f->print( intervall.lowest_key().byte_ptr(), s, std_text_format, '\'', '\'' );
        }
        catch( const Xc& x ) {
            if( s != log_ptr )  *s << x;
        }

        if( cmp != 0 ) {
            *s << " and ";
            try {
                f->print( intervall.highest_key().byte_ptr(), s, std_text_format, '\'', '\'' );
            }
            catch( const Xc& x ) {
                if( s != log_ptr )  *s << x;
            }
            if( cmp > 0 )  throw_xc( "Sql_key_intervall ungültig", "siehe log" );
        }
    }
}

//------------------------------------------------------------------Sql_table::print_intervalls

void Sql_table::print_intervalls( ostream* s, const Sql_key_intervalle& intervalls )
{
    for( int i = 0; i <= intervalls.last_index(); i++ ) {
        if( i > 0 )  *s << " or ";
        if( i % 10 == 0  &&  intervalls.last_index() > 0 )  *s << '\n';
        print_intervall( s, intervalls[ i ] );
    }
}

//------------------------------------------------------------------Sql_table::print_intervalls

void Sql_table::print_intervalls( ostream* s )
{
    print_intervalls( s, _key_intervalle );
}

//---------------------------------------------------------------------Sql_table::prepare_where

void Sql_table::prepare_where( Sql_expr* where_clause )
{
    if( _system )  return;

    _key_intervalle.clear();

    add_key_intervalls( &_key_intervalle, where_clause );

    {
        Log_ptr log;
        if( log ) {
            *log << "Key_intervall für " << *this << ": ";
            print_intervalls( log );  //LOG( _key_intervalle );
            *log << '\n';
        }
    }

    _ki_idx = -1;
    to_next_key_intervall();
}

//---------------------------------------------------------------------------Sql_table::prepare

void Sql_table::prepare()
{
    Sos_string filename;

    if( _full_file_name )            // Tabellenname ist ein vollständiger Dateiname?
    {
        _open_mode = Any_file::Open_mode( Any_file::in );
        // | Any_file::seq );  //jz 7.5.97  nur, wenn kein get_key() und kein rewind() ausgeführt wird. (Nur, wenn 1. Tabelle seq. gelesen wird; Auf nachrangige Tabellen wird rewind() ausgeführt).
        filename = _name;
    } 
    else 
    {
        _open_mode = Any_file::Open_mode( Any_file::inout | Any_file::nocreate );   //jz Damit UPDATE, DELETE, INSERT möglich sind.

        try {
            //filename = as_string( get_single_row( "select file where table_name = ? | "
            //                                      + _session->_catalog_name,
            //                                      _name ) );
            Any_file        f;
            Dynamic_area    record;
            Sos_string      type_kind;
            Sos_string      type_fname;
            Sos_string      select = "select FILE,"
                                            "field( 'LOCAL' ) LOCAL,"
                                            "field( 'TYPE' ) TYPE,"
                                            "field( 'TYPE-KIND' ) TYPE_KIND,"
                                            "field( 'TYPE-LOCAL' ) TYPE_LOCAL "
                                            "where ucase( table_name ) = ucase( ";
            select += quoted_string( _name, '\'', '\'' );
            select += " ) | ";
            select += _session->_catalog_name;

            f.open( select, Any_file::in );
            f.get( &record );

            // -cobol-type='[filenameprefix]typefile'
            type_fname = as_string( f.spec().field_type_ptr()->field_descr_ptr( "TYPE" ), record.byte_ptr() );
            type_kind  = as_string( f.spec().field_type_ptr()->field_descr_ptr( "TYPE-KIND" ), record.byte_ptr() );

            if( !empty( type_fname ) ) {
                // Umstand wegen Gnu-Compiler (Internal compiler error)
                Bool ok = type_kind == "" || type_kind == "COBOL" || type_kind == "FRAME";
                if( ok  )
                {
                    Sos_string fname;
                    if( as_bool( f.spec().field_type_ptr()->field_descr_ptr( "TYPE-LOCAL" ), record.byte_ptr(), false ) ) {
                        fname = _session->_filename_prefix;
                    }
                    fname += type_fname;
                    append_option( &filename,
                                   type_kind != "FRAME"? "-cobol-type=" : "-frame-type=",
                                   fname );
                }
            }

            filename += ' ';

            // [filenameprefix]file
            if( as_bool( f.spec().field_type_ptr()->field_descr_ptr( "LOCAL" ), record.byte_ptr(), false ) ) {
                filename += _session->_filename_prefix;
            }

            filename += as_string( f.spec().field_type_ptr()->field_descr_ptr( "FILE" ), record.byte_ptr() );;

            f.close();

            //LOG( "Sql_table::prepare2 filename=" << filename << "\n" );
        }
        catch( const Eof_error& ) {
            Not_exist_error x ( "SOS-1263" );
            x.insert( _name );
            x.insert( length( _session->_db_name )? c_str( _session->_db_name ) : "[alias]" );
            throw x;
        }

        filename = "com | " + filename;
    }

    //jz 2.12.96 _file.prepare_open( filename, Any_file::Open_mode( _open_mode | Any_file::binary ) );
    _file.prepare_open( filename, _open_mode );

    if( !_table_type )  _table_type = (Record_type*)+_file.spec().field_type_ptr();
    if( !_table_type )  throw_xc( "SOS-SQL-55", c_str( _name ) );

    // Der ganze Datensatz als Feld für select table.* :
    _table_field_descr = Field_descr::create();
    _table_field_descr->_type_ptr = _table_type;
    _table_field_descr->name( "*" );
    _table_field_descr->_offset = 0;

    _table_key_descr = _file.spec()._key_specs._key_spec._field_descr_ptr;
    _unique_key      = _file.key_length()  &&  !_file.spec()._key_specs._key_spec._duplicate;


    //jz 7.12.97if( _table_key_descr ) {
    if( _file.key_in_record() ) {
        _key_len       = _file.key_length();
        _key_pos       = _file.key_position();
        _key_in_record = _file.key_in_record();

        if( !_table_key_descr )  LOG( "??? Datei mit kl=" << _key_len << " und kp=" << _key_pos << ", aber ohne Schlüsselfelder: " << filename << '\n' );
        if( _key_pos >= 0  &&  _key_pos + _key_len > _table_type->field_size() )  throw_xc( "SOS-1369", _key_pos, _key_len, _table_type->field_size() );
    }

    prepare_record_buffer();
}

//-------------------------------------------------------------Sql_table::prepare_record_buffer

void Sql_table::prepare_record_buffer()
{
    // Nullflags hinzufügen für Outer Join:
/*
    int last_null_flag_offset   = -1;
    _appended_null_flags_offset = -1;

    for( int i = 0; i < _table_type->field_count(); i++ ) {
        Field_descr* f = _table_type->field_descr_ptr( i );
        if( f  &&  !f->has_null_flag()  &&  f->type_ptr()  &&  !f->type_ptr()->nullable() ) {
            f->add_null_flag_to( _table_type );
            if( _appended_null_flags_offset == -1 )  _appended_null_flags_offset = f->_null_flag_offset;
        }
    }

    if( _appended_null_flags_offset == -1 )  _appended_null_flags_offset = _table_type->field_size();
*/

    _record.allocate_length( _table_type->field_size() );
}

//--------------------------------------------------------------Sql_table::update_selected_type
// wird von Sql_stmt::prepare_field_expr aufgerufen

void Sql_table::update_selected_type( Field_descr* f )
{
    if( !f )  return;  // kann das sein?

    for( int i = 0; i < _selected_type->field_count(); i++ )  {
        if( _selected_type->field_descr_ptr( i ) == f )  return;
    }

    _selected_type->add_field( f );
}

//------------------------------------------------------------------------------Sql_table::open

void Sql_table::open()
{
    if( _system )  return;

    if( _opened ) {
        rewind();
        return;
    }

    _file.open();
    _opened = true;
    _rewound = true;
    
    if( !_stmt->_using_func_field     // field()-Funktion wird nicht verwendet?
     //&& !_stmt->_loop             )   // Keine LOOP-Klausel?
    )
    {
        // SELECT key FROM file : Nur die Satzposition lesen
        // Provisorisch: Der Satzschlüssel muß das erste Feld der Satzbeschreibung sein!

        if( _selected_type->field_size() == _key_len
       //&&  kleinster offset von _selected_type == _key_pos   //?
         //? ja ode nein:  && _key_pos == 0
         && _selected_type->field_count() >= 1  )
        {
            // Das erste Feld des Schlüssel muß am Satzanfang stehen.
            // Andernfalls müssen die offsets der
            // Field_descrs verschoben werden. Die Field_descrs bei Sql_stmt::prepare_field_expr
            // müssen dann Kopien sein!

            Dynamic_area set ( _key_len );
            memset( set.byte_ptr(), 0, _key_len );

            for( int i = 0; i < _selected_type->field_count(); i++ )
            {
                Field_descr* f = _selected_type->field_descr_ptr( i );
                if( f ) {
                    //LOG( *this << ": " << *f << "\n" );
                    int o = f->offset();
                    int l = f->type_ptr()->field_size();
                    if( o < _key_pos  ||  o + l > _key_len )  goto KEY_ONLY_OK;
               }
            }
            _read_position = true;  // _file.get_pos() nutzen
            LOG( *this << ": Nur Schlüssel werden gelesen\n" );
            KEY_ONLY_OK: ;
        }
    }

    _record.allocate_min( _selected_type->field_size() );

    if( _key_len )  _key.allocate_length( _key_len );

    _ki_idx = -1;           // intervall_index
    to_next_key_intervall();
}

//-----------------------------------------------------------------------------Sql_table::close

void Sql_table::close()
{
    if( !_system )  {
        _opened = false;
        _file.close();
    }
}

//----------------------------------------------------------------------------Sql_table::rewind

void Sql_table::rewind()
{
    if( !_rewound )  _rewind = true;
    _ki_idx = 0;
    _get_count = 0;
}

//---------------------------------------------------------------------------Sql_table::get_seq

void Sql_table::get_seq( const Const_area* until_key )
{
    if( _rewind ) {
        LOGI( "Sql_table(" << simple_name() << ")::get_seq: rewind()\n" );
        _file.rewind();
        _rewound = true;
        _rewind = false;
    }

    _rewound = false;

    _record.allocate_min( _table_type->field_size() );

    if( !_record.length() )  construct_record();
                     //else  memset( _base, 0, _record.length() );

    if( _read_position ) {
        //if( _record.length() < _key_pos + _key_len )  throw_xc( "Sql_table::get_seq" );
        Area pos_area ( _record.byte_ptr() + _key_pos, _key_len );
        _file.get_position( &pos_area, until_key );
        if( pos_area.length() < _key_len )  memset( pos_area.byte_ptr() + pos_area.length(), 0, _key_len - pos_area.length() );
    } else {
        if( until_key )  _file.get_until( &_record, *until_key );
                   else  _file.get( &_record );
        if( _record.length() < _record.size() )  memset( _record.byte_ptr() + _record.length(), 0, _record.size() - _record.length() );
    }

    _get_count++;
    _base = _record.byte_ptr();
    //set_appended_null_flags( false );
}

//-----------------------------------------------------------Sql_table::get_single_key_interval

Bool Sql_table::get_single_key_interval()
{
    // Hält den try-Block aus Sql_table::get heraus
    
    //assert( _unique_key );

    _rewind = false;    //jz 18.3.97
    _rewound = false;

    LOGI2( "sossql.get_key", Z_FUNCTION << "() " << hex << _key_intervalle[ _ki_idx ].lowest_key() << dec << '\n' );

    try {
        _get_key_count++;
        _file.get_key( &_record, _key_intervalle[ _ki_idx ].lowest_key() );
        LOGI2( "sossql.get_key", Z_FUNCTION << "() ==> " << hex << _record << dec << '\n' );
    }
    catch( const Not_found_error& )  { return false; }

    _base = _record.byte_ptr();

    return true;
}

//-------------------------------------------------------------------------------Sql_table::get

void Sql_table::get()
{
    if( _stmt->_read_count > _stmt->_max_reads )  throw_xc("SOS-SQL-91", _stmt->_max_reads );

    if( !_key_in_record ) 
    {
        get_seq( NULL );
        goto FERTIG;
    }


    while( _ki_idx <= _key_intervalle.last_index() )
    {
        Bool eof = false;

        Sql_key_intervall* ki = &_key_intervalle[ _ki_idx ];

        if( _set_key ) 
        {
            if( _unique_key  &&  ki->lowest_key() == ki->highest_key() )
            {
                Bool ok = get_single_key_interval();
                _ki_idx++;
                if( ok )  goto FERTIG;
                continue;
            }

          //if( ki->lowest_key() != sql_low_value  ||  !_rewound ) {
            if( _rewound  &&  memcmp( _key.ptr(), sql_low_value.ptr(), _key.length() ) == 0 )
            {
                // set() überflüssig
                _set_key = false;  //jz 7.6.01, Fehlermeldung von Fehrmann vom 30.5.01
            }
            else
            {
                _set_key = false;
                _set_count++;
                
                LOGI( "Sql_table(" << simple_name() << ")::get set_key " << hex << _key << dec << '\n' );

                _rewind = false;
                _rewound = false;
              //_file.set( ki->lowest_key() );  //jz 23.12.00
                _file.set( _key );
            }
        }


        try {
            if( memcmp( ki->highest_key().ptr(), sql_high_value.byte_ptr(), _key_len ) == 0 ) {
                // Kein get_until, das verträgt nicht jeder Dateityp.
                get_seq( NULL );  
            } else {
                get_seq( &ki->highest_key() );
            }
        }
        catch( const Eof_error& ) { eof = true; }       // Bei Verwendung von get_until() gilt EOf nur für ein Intervall.
                                                        // Bei vielen Intervallen ist das vielleicht nicht effzient. jz 7.12.97


        if( !eof ) 
        {
            if( memcmp( _base + _key_pos,
                        ki->highest_key().ptr(),
                        ki->highest_key().length() ) <= 0 ) 
            {
                if( ki->_offset ) {      // Anfang des Key_intervalls ist unbestimmt?
                    if( !key_in_offset_intervall( _base + _key_pos ) )  continue;
                }
        
                goto FERTIG;
            }
        }

        to_next_key_intervall();  // Beim nächsten Zugriff auf nächsten Intervall-Anfang positionieren
    }

    throw_eof_error();
    return;

  FERTIG:
    _stmt->_read_count++;
}

//---------------------------------------------------------------------------Sql_table::set_key

void Sql_table::set_key()
{
    set_key_intervall();

  //if( key_in_offset_intervall( _key.byte_ptr() ) ) 
  //{

        if( _ki_idx <= _key_intervalle.last_index() )   // 13.5.01 neu: Feinsteuerung, wenn where-Klausel hinteren Teil des Schlüssel einschränkt
        {
            Sql_key_intervall* ki = &_key_intervalle[ _ki_idx ];

            if( memcmp( _key.byte_ptr() + ki->_offset,
                        ki->lowest_key().byte_ptr() + ki->_offset,
                        ki->lowest_key().length() - ki->_offset ) < 0 ) 
            {
                memcpy( _key.byte_ptr() + ki->_offset,
                        ki->lowest_key().byte_ptr() + ki->_offset,
                        ki->lowest_key().length() - ki->_offset );
            }
        }


        LOGI( "Sql_table(" << simple_name() << ")::set_key " << hex << _key << dec << '\n' );

        _rewound = false;
        _rewind = false;
        _set_key = true;    // jz 23.12.00
      //_file.set( _key );
  //} 
  //else 
  //{
  //    LOG( "Sql_table(" << simple_name() << ")::set_key " << hex << _key << dec << " nicht im Schlüsselintervall\n" );
  //    _set_key = true;
  //} 
}

//-----------------------------------------------------------------Sql_table::set_key_intervall
// Positioniert anhand des Schlüssels _key den Index _ki_idx
// auf das richtige Schlüssel-Intervall

void Sql_table::set_key_intervall()
{
    int i;

    for( i = 0; i < _key_intervalle.count(); i++ ) {
        if( memcmp( _key_intervalle[ i ].highest_key().ptr(), _key.ptr(), _key_len ) >= 0 )  break;  // Intervall-Ende >= _key ?
    }

    _ki_idx = i;
}

//-------------------------------------------------------------Sql_table::to_next_key_intervall

void Sql_table::to_next_key_intervall()     // jz 22.12.00
{
    _ki_idx++;
    
    if( _ki_idx <= _key_intervalle.last_index() ) 
    {
        _key = _key_intervalle[ _ki_idx ].lowest_key();
    }
  //else eof
    
    _set_key = true;
}

//-----------------------------------------------------------Sql_table::key_in_offset_intervall

Bool Sql_table::key_in_offset_intervall( const Byte* key )
{
    // Nur ein hinterer Teil des Schlüssels wird in der WHERE-Klausel abgefragt (z.B. Satzart).
    // Die Abfrage in der Where-Klausel ist totgelegt (_computed=true; _value=true),
    // weshalb hier abgefragt werden muss. Das ist auch effizienter.

    // Das hier ist nur die halbe Wahrheit. Eine Schlüsselklausel kann aus mehreren Segmenten
    // bestehen: where seg between 1 and 7 and seg2 between 3 and 9
    // seg2 muss außerdem mit eval_bool_expr geprüft werden. jz 8.12.97
    
    // Man könnte, statt sequentiell weiter zu lesen, auf den nächsten Schlüssel positionieren.
    // Aber dann sollte Fs_file::set_key im Burst_buffer positionieren, sonst wird's ineffizient.

    if( _ki_idx > _key_intervalle.last_index() )  return false;

    Sql_key_intervall* ki = &_key_intervalle[ _ki_idx ];

    if( memcmp( key + ki->_offset,
                ki->lowest_key().byte_ptr() + ki->_offset,
                ki->lowest_key().length() - ki->_offset ) < 0 )  return false;

    if( memcmp( key + ki->_offset,
                ki->highest_key().byte_ptr() + ki->_offset,
                ki->highest_key().length() - ki->_offset ) > 0 )  return false;

    return true;
}

//---------------------------------------------------------------------------Sql_table::get_key

void Sql_table::get_key()
{
    if( _stmt->_read_count > _stmt->_max_reads )  throw_xc("SOS-SQL-91", _stmt->_max_reads );

    set_key_intervall();
    if( !key_in_offset_intervall( _key.byte_ptr() ) )  throw_not_found_error( "SOS-SQL-68" );

    LOGI2( "sossql.get_key", Z_FUNCTION << "() " << hex << _key << dec << '\n' );

    _rewound = false;
    _rewind = false;

    _file.get_key( &_record, _key );

    _base = _record.byte_ptr();
    //set_appended_null_flags( false );

    _stmt->_read_count++;
}

//----------------------------------------------------------------------------Sql_table::update

void Sql_table::update()
{
    _rewound = false;
    _file.update( _record );
}

//----------------------------------------------------------------------------Sql_table::insert

void Sql_table::insert()
{
    _rewound = false;
    _file.insert( _record );
}


//-------------------------------------------------------------------------------Sql_table::del

void Sql_table::del()
{
    _rewound = false;
    _file.del();
}

//------------------------------------------------------------------Sql_table::construct_record

void Sql_table::construct_record()
{
    _record.allocate_length( _table_type->field_size() ); //_selected_type->field_size() );

    _base = _record.byte_ptr();
    //_table_type->construct( _base );
    memset( _base, 0, _record.length() );

    //set_appended_null_flags( false );
}

//-----------------------------------------------------------Sql_table::set_appended_null_flags
/*
void Sql_table::set_appended_null_flags( Bool b )
{
    Bool* p     = (Bool*)( _base + _appended_null_flags_offset );
    Bool* p_end = (Bool*)( _base + _table_type->field_size() );

    while( p < p_end )  *p++ = b;
}
*/
//--------------------------------------------------------------------------Sql_table::set_null
/*
void Sql_table::set_null()
{
    _table_type->set_null( _base );
    //set_appended_null_flags( false );
}
*/

//-----------------------------------------------------------------------Sql_table::simple_name

Sos_string Sql_table::simple_name()
{
    if( !empty( _alias ) )  return as_string( _alias );
    return _name;
}

//------------------------------------------------------------------------Sql_table::_obj_print

void Sql_table::_obj_print( ostream* s ) const
{
    *s << "Tabelle ";
    if( length( _alias ) )  *s << _alias;
                      else  *s << _name;
}

//-----------------------------------------------------------------Sql_system_record::make_type

Sos_ptr<Record_type> Sql_system_record::make_type()
{
    ZERO_RETURN_VALUE( Sos_ptr<Record_type> );

    Sos_ptr<Record_type> t = SOS_NEW( Record_type );
    Sql_system_record*       o = 0;
    t->name( "Sql_system_record" );

    RECORD_TYPE_ADD_FIELD( sys_loop_index, 0 );   _sys_loop_index_field = t->field_descr_ptr( t->field_count() - 1 );
    RECORD_TYPE_ADD_FIELD( sys_date      , 0 );
    RECORD_TYPE_ADD_FIELD( sys_record_no , 0 );
    RECORD_TYPE_ADD_FIELD( user          , 0 );
    RECORD_TYPE_ADD_FIELD( sys_newline   , 0 );
    RECORD_TYPE_ADD_FIELD( sys_cr        , 0 );

    return +t;
}


} //namespace sos
