#include "precomp.h"

#if defined __BORLANDC__
#   define CHECK_STACK_OVERFLOW
#endif

#undef  MODULE_NAME // wegen Templates in Solaris 4.0.1
#define MODULE_NAME "sosfld2"
#undef  COPYRIGHT
#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
#undef  AUTHOR
#define AUTHOR      "Joacim Zschimmer"


#include "tabucase.h"       // für As_numeric_type
#include <ctype.h>          // für As_numeric_type

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
//#include "../kram/log.h"
#include "../kram/sosopt.h"         // Sos_token_iterator
#include "../kram/sosarray.h"
#include "../kram/soslimtx.h"
#include "../kram/xlat.h"
#include "../kram/sosctype.h"
#include "../kram/sosfield.h"
#include "../kram/sosfld2.h"
#include "../kram/sosdate.h"        // für modify_fields
#include "../kram/log.h"

using namespace std;
namespace sos {

//-------------------------------------------------------------Field_converter::Field_converter

Field_converter::Field_converter()
:
    _record_size ( 0 ),
    _empty_is_null ( false )
{
    _table.obj_const_name( "Field_converter::_table" );
}

//-------------------------------------------------------------Field_converter::Field_converter

Field_converter::Field_converter( const Sos_ptr<Record_type>& object_type,
                                  const Sos_ptr<Record_type>& record_type )
:
    _record_size ( 0 ),
    _empty_is_null ( false )
{
    init( object_type, record_type );
}

//-----------------------------------------------------Field_converter::prepare_for_equal_names

void Field_converter::prepare_for_equal_names( const Sos_ptr<Record_type>& object_type,
                                               const Sos_ptr<Record_type>& record_type )
{
    // Nur Felder mit gleichen Namen werden berücksichtigt. Alle anderen werden ignoriert.
    // Das kann dazu führen, dass kein Feld konvertiert wird.

    _object_type = object_type;
    _record_type = record_type;

    int i;
    int j = 0;
    int n = object_type->field_count();
    int m = record_type->field_count();

    for( i = 0; i < n; i++ )
    {
        const Field_descr& dn = object_type->field_descr( i );
        const char* dname = c_str( dn.name() );

        int j_stop = j;      // Zur Optimierung, falls beide Records gleich geordnet sind

        while(1)
        {
            const Field_descr& sn = record_type->field_descr( j );
            const char* sname = c_str( sn.name() );

            if( field_names_are_equal( dname, sname ) )
            {
                _table.add( Field_pair( &dn, &sn ) );
                //jz 9.7.97 _record_size = max( (uint)_record_size, (uint)sn.offset() + sn.type().field_size() );
                break;
            }

            j++;
            if( j == m )  j = 0;
            if( j == j_stop )  break;
        }
    }

    _record_size = record_type->field_size();  //jz 9.7.97
    _set_null_first = true;
}

//----------------------------------------------------------Field_converter::prepare_for_tuples

void Field_converter::prepare_for_tuples( const Sos_ptr<Record_type>& object_type,
                                          const Sos_ptr<Record_type>& record_type )
{
    int n = object_type->field_count();
    int m = record_type->field_count();

    if( n != m )  {
        Xc x ( "SOS-1190" );
        x.insert( object_type->name() + " mit " + as_string( n ) + " Feldern" );
        x.insert( record_type->name() + " mit " + as_string( m ) + " Feldern" );
        throw x;
    };

    _object_type = object_type;
    _record_type = record_type;
    _table.last_index( _table.first_index() - 1 );      // evtl. löschen
    _table.size( n );

    for( int i = 0; i < n; i++ )
    {
        _table.add( Field_pair( &object_type->field_descr( i ),
                                &record_type->field_descr( i ) ) );
    }

    _record_size = record_type->field_size();
}

//------------------------------------------------------------Field_converter::~Field_converter

Field_converter::~Field_converter()
{
}

//------------------------------------------------------------------------Field_converter::copy

void Field_converter::copy( void* dest_ptr, const void* src_ptr, Direction direction )
{
    if( _set_null_first ) {
        //Nicht benutzte Felder nicht ändern. Dateityp convert setzt nichtig.
        //( direction == dir_record_to_object? _object_type : _record_type ) -> set_null( (Byte*)dest_ptr );
    }

    const Field_pair* p = 0;

    int last = _table.last_index();

    _hilfspuffer.allocate_min( max_display_field_size + 1 );

    //try {
        for( int i = _table.first_index(); i <= last; i++ )
        {
             p = &_table[ i ];

             if( direction == dir_record_to_object ) {
                 copy_field( p->_o, (Byte*)dest_ptr,
                             p->_r, (Byte*)src_ptr, &_hilfspuffer, _empty_is_null );
             } else {
                 copy_field( p->_r, (Byte*)dest_ptr,
                             p->_o, (Byte*)src_ptr, &_hilfspuffer, _empty_is_null );
             }
        }
/*
    }
    catch( Xc& x )
    {
        char text [ 100 ];
        ostrstream s ( text, sizeof text );
        s << p->_r->name()
          << " " << p->_r->type()
          << " at " << p->_r->offset();
        x.insert( text, s.pcount() );
        throw x;
    }
*/
}

//------------------------------------------------------------------------Field_converter::read

void Field_converter::read( void* dest_obj, const Const_area& src_rec )
{
    //int soll = _record_type->field_length( src_rec.byte_ptr(), src_rec.byte_ptr() + src_rec.length() );
    int soll = _record_size;
    if( (int)src_rec.length() < soll )  throw_too_long_error( "SOS-1114" , src_rec.length(), soll );

    copy( dest_obj, src_rec.ptr(), dir_record_to_object );
}

//------------------------------------------------------------------------Field_converter::write

void Field_converter::write( const void* src_obj, Area* dest_rec )
{
    dest_rec->allocate_min( _record_size );
    copy( dest_rec->ptr(), src_obj, dir_object_to_record );
    //dest_rec->length( _record_type->field_length( dest_rec->byte_ptr(), dest_rec->byte_ptr() + _record_size ) );
    if( (int)dest_rec->length() < _record_size )  dest_rec->length( _record_size );
}

//-----------------------------------------------------------------------------------skip_white

void skip_white( const char** pp, char separator = ',' )
{
    const char* p = *pp;
    if( separator == '\t' )  while( *p == ' ' )  p++;
                       else  while( sos_isspace( *p ) )  p++;
    *pp = p;
}

//-----------------------------------------------------------------------------------skip_white

inline void skip_white( char** pp, char separator = ',' )
{
    skip_white( (const char**)pp, separator );
}

//-----------------------------------------------------------read_field_name_and_interpretation

void read_field_name_and_interpretation( const char** pp,
                                         Sos_string* name, Sos_string* interpretation,
                                         char separator )
{
    const char* p = *pp;
    skip_white( &p, separator );

    const char* p1 = p;
    while( sos_isalnum( *p ) || *p == '_' || *p == '-' )  p++;
    *name = as_string( p1, p - p1 );

    skip_white( &p, separator );

    if( *p == ':' ) {
        p1 = p;
        skip_white( &p, separator );
        //*interpretation = ":";
        int klammern = 0;
        while( *p ) {
            if( klammern == 0 ) {
                if( *p == ',' )  break;   // == separator?   jz 29.12.99
                if( *p == ')' )  break;
            }

            if( *p == '(' )  klammern++;
            else 
            if( *p == ')' )  klammern--;

            p++; 
/*29.12.99 jz
            if( *p == '(' )  klammern++;
            if( *p == ')' ) {
                if( klammern == 0 )  break;
                klammern--;
            }
            if( !klammern ) {
                skip_white( &p, separator );
                if( !*p )  break;
            }
            p++;
*/
            //*interpretation += *p++;
        }
        
        const char* p2 = p;
        while( p2 > p1  &&  isspace( p2[-1] ) )  p2--;
        *interpretation = as_string( p1, p2 - p1 );
    } else {
        *interpretation = "";
    }

    skip_white( &p, separator );

    *pp = p;
}

//-----------------------------------------------------------------------------------get_string

void get_string( const char** pp, Area* string )
{
    const char* p = *pp;
    skip_white( &p );

    if( *p != '\''  &&  *p != '"' )  goto FEHLER;
    else {
        char quote = *p++;
        string->length( 0 );
        while( *p && *p != quote ) *string += *p++;
        if( *p != quote )  goto FEHLER;
        p++;
        skip_white( &p );
        *pp = p;
        return;
    }

  FEHLER: throw_xc( "SOS-1116", *pp );
}

//-----------------------------------------------------------------------------------get_string

void get_string( const char** pp, Sos_string* string )
{
    Dynamic_area buffer;
    get_string( pp, &buffer );
    *string = as_string( buffer );
}

//---------------------------------------------------------------------------create_record_type

void create_record_type( Sos_ptr<Record_type>* type_ptr, Record_type* source_type,
                         const Sos_string& new_type_name, const char* suffix, const char* function_name )
{
    if( !source_type )  throw_xc( "SOS-1193", function_name );

    *type_ptr = Record_type::create();

    (*type_ptr)->flat_scope( source_type->flat_scope() );  //jz 21.4.97 Damit bei -field=(gruppe) die Gruppenelemente ansprechbar bleiben (Südpack) 

    if( new_type_name != "" )  (*type_ptr)->name( new_type_name );
    else
    if( length( source_type->name() ) < 100 )  (*type_ptr)->name( source_type->name() + suffix );
}

//---------------------------------------------------------------record_type_of_selected_fields

Sos_ptr<Record_type> record_type_of_selected_fields( const Sos_ptr<Record_type>& record_type,
                                                     const Sos_string& fields,
                                                     const Sos_string& new_type_name )
{
    ZERO_RETURN_VALUE( Sos_ptr<Record_type> );

    Bool                    geklammert = false;
    Sos_ptr<Record_type>    t;
    const char*             p0 = c_str( fields );
    const char*             p  = p0;
    Sos_string              name;
    Sos_string              interpretation;
    Sos_ptr<Field_descr>    f2;

    create_record_type( &t, record_type, new_type_name, "/selected", "record_type_of_selected_fields" );

    if( *p == '(' )  { p++; geklammert = true; }

    while(1) {
        read_field_name_and_interpretation( &p, &name, &interpretation, ',' );
        Field_descr* f = record_type->field_descr_ptr( c_str( name ) );
        //LOG( "record_type_of_selected_fields f=" << *f << "\n" );

/*jz 22.9.97
        if( f->type_ptr()
         && f->type_ptr()->obj_is_type( tc_Record_type )  // Gruppe von Felder in Cobol?
         && ((Record_type*)f->type_ptr())->_group_type )  // Den Gruppentyp nehmen, wenn vorhanden
        {
            //jz 22.4.97 f2 = Field_descr::create( *f );
            f2 = obj_copy( *f );
            f2->_type_ptr = ((Record_type*)f->type_ptr())->_group_type;
            LOG( "record_type_of_selected_fields f=" << *f << ", f2="<< *f2 << "\n" );
            f = f2;
        }
*/
        //if( t->flat_scope() ) {
        //    t->_field_size = max( t->_field_size, (int)f->offset() + f->type().field_size() );
        //    if( f->has_null_flag() )  t->_field_size = max( t->_field_size, (int)f->_null_flag_offset + sizeof (Bool) );
        //}

        if( interpretation == empty_string ) {
            t->add_field( f );
        } else {
            t->add_field( modified_field( f, interpretation ) );
        }

        if( *p != ',' )  break;
        p++;
    }

    skip_white( &p );

    if( geklammert )  {
        if( *p != ')' )  throw_syntax_error( "SOS-1184", ")", p - p0 );
        p++;
        skip_white( &p );
    }

    if( *p )  throw_syntax_error( "SOS-1184", "Parameterende", p - p0 );

    return t;
}

//------------------------------------------------------------------record_type_without_fields

Sos_ptr<Record_type> record_type_without_fields( const Sos_ptr<Record_type>& record_type,
                                                 const Sos_string& fields,
                                                 const Sos_string& new_type_name  )
{
    ZERO_RETURN_VALUE( Sos_ptr<Record_type> );

    if( !record_type )  throw_xc( "SOS-1193", "record_type_without_fields" );

    Sos_string selected_fields;
    Sos_simple_array<Sos_string> fields_list;

    fields_list.obj_const_name( "record_type_without_fields" );

    {
        Sos_token_iterator token ( fields, ',' );
        for( ; !token.end(); token.next() ) {
            fields_list.add( token.value() );
        }
    }

    int count = 0;
    for( int i = 0; i < record_type->field_count(); i++ ) {
        Field_descr* f = record_type->field_descr_ptr( i );
        for ( int j = fields_list.first_index(); j <= fields_list.last_index(); j++ ) {
           if ( fields_list[j] == f->name() ) goto loop; // weiter
        }
        if ( count++ > 0 ) selected_fields += ",";
        selected_fields += f->name();                    // den nehmen wir
    loop: ;
    }

    //LOG( "record_type_without_fields: selected_fields=" << selected_fields << '\n' );
    return record_type_of_selected_fields( record_type, selected_fields, new_type_name );
}



//-----------------------------------------------------------------record_type_where_flags_true

Sos_ptr<Record_type> record_type_where_flags_true( const Sos_ptr<Record_type>& record_type,
                                                   Field_descr_flags flags,
                                                   const Sos_string& new_type_name )
{
    ZERO_RETURN_VALUE( Sos_ptr<Record_type> );

    if( !record_type )  throw_xc( "SOS-1193", "record_type_where_flags_true" );

    Sos_ptr<Record_type> t = Record_type::create();
    int                  n = record_type->field_count();

    if( new_type_name != "" ) t->name( new_type_name );
    else
    if( length( record_type->name() ) < 100 ) {
         Sos_string new_name ( record_type->name() + "/" + as_string( flags ) );
         t->name( new_name );
    }


    for( int i = 0; i < n; i++ ) {
        Sos_ptr<Field_descr> f = record_type->field_descr_ptr( i );

        if( ( f->_flags & flags ) == flags ) {
            t->add_field( f );
        }
    }

    return t;
}

//-----------------------------------------------------------------fields_where_flags_true

Sos_string fields_where_flags_true( const Sos_ptr<Record_type>& record_type,
                                    Field_descr_flags flags )
{
    ZERO_RETURN_VALUE( Sos_string );

    Sos_string fields;

    if( !record_type )  throw_xc( "SOS-1193", "fields_where_flags_true" );
    int n = record_type->field_count();

    for( int i = 0; i < n; i++ ) {
        Sos_ptr<Field_descr> f = record_type->field_descr_ptr( i );

        if( ( f->_flags & flags ) == flags ) {
            if ( i > 0 ) fields += ",";
            fields += f->name();
        }
    }

    return fields;
}

//----------------------------------------------------------------------------------copy_fields

Sos_ptr<Record_type> copy_fields( Sos_ptr<Record_type> record_type,
                                  const Sos_string& new_type_name )
{
    ZERO_RETURN_VALUE( Sos_ptr<Record_type> );

    int                     n = record_type->field_count();
    Sos_ptr<Record_type>    t;

    create_record_type( &t, record_type, new_type_name, "", "copy_fields" );
    t->allocate_fields( n );

    for( int i = 0; i < n; i++ )
    {
        //jz 1.11.98 (egcs)  Sos_ptr<Field_descr> f = obj_copy( record_type->field_descr( i ) );
        Sos_ptr<Field_descr> f = OBJ_COPY( Field_descr, record_type->field_descr( i ) );
        t->add_field( f );
    }

    return t;
}

//--------------------------------------------------------------------------------moved_offsets

Sos_ptr<Record_type> moved_offsets( const Sos_ptr<Record_type>& record_type, int diff,
                                    const Sos_string& new_type_name )
{
    ZERO_RETURN_VALUE( Sos_ptr<Record_type> );

    Sos_ptr<Record_type> t = copy_fields( record_type, new_type_name );
    move_offsets( t, diff );
    return t;
}

//---------------------------------------------------------------------------------move_offsets

void move_offsets( Record_type* record_type, int diff )
{
    if( !record_type )  throw_xc( "SOS-1193", "move_offsets" );

    record_type->_field_size += diff;   // js 8.6.99: offensichtlich ein Fehler!  // jz 15.8.2002: Ich sehe keinen Fehler.

    int n = record_type->field_count();

    for( int i = 0; i < n; i++ ) {

        Field_descr* f = record_type->field_descr_ptr( i );
        
        f->offset( f->offset() + diff );
        if( (uint)( f->offset() + f->type_ptr()->field_size() ) > record_type->field_size() )   
        {
            Xc x ( "SOS-1448" );
            x.insert( f );
            x.insert( record_type->name() );
            throw x;
        }


        if( f->has_null_flag() ) 
        {
            f->_null_flag_offset += diff;

            if( f->_null_flag_offset < 0
             || f->_null_flag_offset + sizeof (Bool) > record_type->field_size() )
            {
                Xc x ( "SOS-1324" );
                x.insert( f );
                x.insert( record_type->name() );
                x.insert( f->_null_flag_offset );
                throw x;
            }
        }
    }

    //record_type->_field_size += diff;   // jz 18.10.96
}

//-------------------------------------------------------------------------------renamed_fields

Sos_ptr<Record_type> renamed_fields( const Sos_ptr<Record_type>& record_type,
                                     const Sos_string& names, const Sos_string& new_type_name )
{
    ZERO_RETURN_VALUE( Sos_ptr<Record_type> );

    Sos_ptr<Record_type>    t;
    int n = record_type->field_count();

    create_record_type( &t, record_type, new_type_name, "/renamed", "renamed_fields" );
    t->allocate_fields( n );

    const char* p = c_str( names );
    Bool        klammer = false;

    if( *p == '(' )  {
        p++;
        klammer = true;
    }

    int i = 0;
    while( i < n )
    {
        //jz 22.4.97 Sos_ptr<Field_descr> f = Field_descr::create( record_type->field_descr( i ) );
        //jz 1.11.98 (egcs) Sos_ptr<Field_descr> f = obj_copy( record_type->field_descr( i ) );
        Sos_ptr<Field_descr> f = OBJ_COPY( Field_descr, record_type->field_descr( i ) );
        skip_white( &p );
        const char* p2 = p;
        while( *p2 && !sos_isspace( *p2 ) && *p2 != ',' && *p2 != ')' ) p2++;
        Sos_string new_name = as_string( p, p2 - p );
        f->name( new_name );
        t->add_field( f );
        i++;
        p = p2;
        skip_white( &p );
        if( *p != ',' )  break;
        p++;
    }

    if( klammer ) {
        skip_white( &p );
        if( *p != ')' )  throw_syntax_error( "SOS-1184", ")", Source_pos( "<fieldlist>", -1, p - c_str( names ) ) );
        p++;
    }

    skip_white( &p );
    if( i != n )  {
        Xc x ( "SOS-1203" );
        x.pos( Source_pos( "-fields=", -1, p - c_str( names ) ) );
        x.insert( n );
        x.insert( t->name() );
        x.insert( i );
        throw x;
    }

    if( *p )  throw_xc( "SOS-1239", p );

    return t;
}

//-------------------------------------------------------------------------------rename_fields

Sos_ptr<Record_type> rename_fields( Sos_ptr<Record_type> record_type,
                                    const Sos_string& names, const Sos_string& new_type_name )
{
    ZERO_RETURN_VALUE( Sos_ptr<Record_type> );

    if( !record_type )  throw_xc( "SOS-1193", "rename_fields" );

    if( new_type_name != "" ) record_type->name( new_type_name );

    Sos_token_iterator token ( names, ',' );
	int i;
    for( i = 0; i < record_type->field_count(); i++ )
    {
        if( token.end() )  goto throw_sos_1203;
        Sos_string new_name = token.value();
        record_type->field_descr_ptr( i )->name( new_name );
        token.next();
    }

    if( !token.end() )  {
        throw_sos_1203: Xc x ( "SOS-1203" ); x.insert( i ); x.insert( record_type->name() ); throw x;
    }

    return record_type;
}

//------------------------------------------------------------------------------modified_fields

Sos_ptr<Record_type> modified_fields( Sos_ptr<Record_type> record_type, const Sos_string& names,
                                      const Sos_string& new_type_name )
{
    ZERO_RETURN_VALUE( Sos_ptr<Record_type> );

    if( !record_type )  throw_xc( "SOS-1193", "modified_fields" );

    if( !empty( new_type_name ) ) record_type->name( new_type_name );

    Sos_ptr<Record_type> t = copy_fields( record_type, new_type_name );
    modify_fields( t, names );
    return t;
}

//--------------------------------------------------------------------------------modify_fields

void modify_fields( Sos_ptr<Record_type> record_type, const Sos_string& names )
{
    // names enthält eine Liste von Namen, die durch Komma getrennt sind.
    // Die Liste kann in Klammern gesetzt sein. Blanks werden übersprungen.
    // Hinter einem Namen kann, evtl. durch einen Doppelpunkt getrennt, eine Typ-Interpretation
    // stehen.
    // Folgende Interpretationen können angegeben werden: (Groß-/Kleinschreibung beliebig)
    //   bool   0, f, F, n, n, <blank> ==> false    1, t, T, j, J, y, Y, x, X ==> true
    //   date( "format" )      Format-String für Sos_date
    //   numeric
    //   text

    if( !record_type )  throw_xc( "SOS-1193", "modify_fields" );

    Bool        geklammert = false;
    const char* p0         = c_str( names );
    const char* p          = p0;
    Sos_string  name;
    Sos_string  interpretation;

    if( *p == '(' )  { p++; geklammert = true; }

    while(1) {
        read_field_name_and_interpretation( &p, &name, &interpretation, ',' );
        modify_field( record_type->field_descr_ptr( c_str( name ) ), interpretation );

        if( *p != ',' )  break;
        p++;
    }

    skip_white( &p );

    if( geklammert )  {
        if( *p != ')' )  throw_syntax_error( "SOS-1184", ")", p - p0 );
        p++;
        skip_white( &p );
    }

    if( *p )  throw_syntax_error( "SOS-1184", "Parameterende", p - p0 );
}

//------------------------------------------------------------------------------key_field_descr

Sos_ptr<Field_descr> key_field_descr( const Sos_ptr<Record_type>& selected_key_type,
                                      const Sos_string& name )
{
    ZERO_RETURN_VALUE( Sos_ptr<Field_descr> );

    if( !selected_key_type )  throw_xc( "SOS-1193", "key_field_descr" );

    //jz 15.8.2002 int offset = selected_key_type->field_descr_ptr( 0 )->offset();  // Besser wäre das Minimum aller Offsets
    int offset = INT_MAX;
    int i;
    for( i = 0; i < selected_key_type->field_count(); i++ )  offset = min( offset, selected_key_type->field_descr_ptr( i )->offset() );

    // Prüfen, dass Felder zusammenhängen
    for( i = 1; i < selected_key_type->field_count(); i++ )  
    {
        Field_descr* f = selected_key_type->field_descr_ptr( i-1 );
        int expected_offset = f->offset() + f->type_ptr()->field_size();
        
        if( selected_key_type->field_descr_ptr( i )->offset() < expected_offset )
        {
            LOG( "**********************************************************************\n"
                 "*** A C H T U N G  *** SCHLÜSSELFELDER IN -KEY= SIND IN FALSCHER REIHENFOLGE: " << *f << ", " << *selected_key_type->field_descr_ptr( i ) << " *** A C H T U N G *** \n"
                 "**********************************************************************\n" );
            //if( sos_static_ptr()->since_version(1,5,36) )  throw_xc( "SOS-1449", f->name() );
        }

        if( selected_key_type->field_descr_ptr( i )->offset() > expected_offset )
        {
            LOG( "***** SCHLÜSSELFELDER IN -KEY= HÄNGEN NICHT ZUSAMMEN: " << *f << ", " << *selected_key_type->field_descr_ptr( i ) << " *****\n" );
            //if( sos_static_ptr()->since_version(1,5,36) )  throw_xc( "SOS-1449", f->name() );
        }
    }

    if( offset != selected_key_type->field_descr_ptr( 0 )->offset() )  throw_xc( "SOS-1449", selected_key_type->field_descr_ptr( 0 )->name() );

    Sos_ptr<Record_type> key_type;
    if( offset ) key_type = moved_offsets( selected_key_type, -offset );
            else key_type = selected_key_type;
    Sos_ptr<Field_descr> f = SOS_NEW( Field_descr( +key_type, c_str( name ), offset, -1, (Field_descr_flags)0 ) );  // Array_field_descr geht verloren!!

//int FEHLER_SOS_1325_NULL_FLAG_IM_SCHLUESSEL_AUSKOMMENTIERT;
/*
    // Prüfen:
    for( int i = 0; i < selected_key_type->field_count(); i++ ) {
        Field_descr* f = selected_key_type->field_descr_ptr( i );
        if( f->has_null_flag() )  throw_xc( "SOS-1325", f );
        if( !f->type_ptr()->info()->_field_copy_possible )  throw_xc( "SOS-1326", f );
        // BESSER: !info()->_clear_layout; // D.h. einfaches Speicherabbild wie int4, Ebcdic_text, aber nicht Sos_string, Sos_limited_text etc.
    }
*/
    return f;
}

//----------------------------------------------------------------------key_field_descr_by_name

Sos_ptr<Field_descr> key_field_descr_by_name( const Sos_ptr<Record_type>& record_type,
                                              const Sos_string& field_names, const Sos_string& name )
{
    ZERO_RETURN_VALUE( Sos_ptr<Field_descr> );

    if( !record_type )  throw_xc( "SOS-1193", "key_field_descr_by_name" );

    Sos_ptr<Record_type> key_type = record_type_of_selected_fields( record_type, field_names );
    return key_field_descr( key_type, name );
}

//----------------------------------------------------------------------------copy_field_descrs

Sos_ptr<Record_type> copy_field_descrs( const Sos_ptr<Record_type>& source_type,
                                        const Sos_string& new_type_name )
{
    // Baut neuen Record auf, offsets der Felder werden neu berechnet!

    ZERO_RETURN_VALUE( Sos_ptr<Record_type> );

    Sos_ptr<Record_type> t;

    create_record_type( &t, source_type, new_type_name, "/copy", "copy_field_descr" );

    for( int i = 0; i < source_type->field_count(); i++ ) {
        Field_descr* f = source_type->field_descr_ptr( i );
        if( f ) {
            Sos_ptr<Field_descr> g = SOS_NEW( Field_descr( f->type_ptr(), f->name() ) );   // Array_field_descr geht verloren!!
            g->add_to( t );
            if( f->has_null_flag() )  g->add_null_flag_to( t );
        } else {
            t->add_field( NULL );
        }
    }

    return t;
}

//-----------------------------------------------------------------------------copy_record_type

Sos_ptr<Record_type> copy_record_type( const Sos_ptr<Record_type>& source_type,
                                       const Sos_string& new_type_name )
{
    // Behält die offsets bei
    ZERO_RETURN_VALUE( Sos_ptr<Record_type> );

    Sos_ptr<Record_type> t;

    create_record_type( &t, source_type, new_type_name, "/copy", "copy_record_type" );

    for( int i = 0; i < source_type->field_count(); i++ ) {
        Field_descr* f = source_type->field_descr_ptr( i );
        if( f ) {
            //jz 1.11.98 t->add_field( obj_copy( *f ) );
            t->add_field( OBJ_COPY( Field_descr, *f ) );
        } else {
            t->add_field( NULL );
        }
    }

    return t;
}

//-------------------------------------------------------------------------flattend_record_type

void add_flattend_record_type( Record_type* type, Bool with_groups, Record_type* source_type )
{
    for( int i = 0; i < source_type->field_count(); i++ )
    {
        Field_descr* f = source_type->field_descr_ptr( i );
        if( f ) {
            if( f->type_ptr()  &&  f->type_ptr()->obj_is_type( tc_Record_type ) )
            {
                add_flattend_record_type( type, with_groups, (Record_type*)f->type_ptr() );

                if( with_groups  &&  ((Record_type*)f->type_ptr())->_group_type ) {
                    //jz 1.11.98 (egcs) Sos_ptr<Field_descr> group_field = obj_copy( *f );
                    Sos_ptr<Field_descr> group_field = OBJ_COPY( Field_descr, *f );
                    group_field->_type_ptr = ((Record_type*)f->type_ptr())->_group_type;
                    type->add_field( group_field );
                }
            } else {
                //1.11.98 type->add_field( obj_copy( *f ) );
                type->add_field( OBJ_COPY( Field_descr, *f ) );
            }
        } else {
            type->add_field( NULL );
        }
    }
}

//-------------------------------------------------------------------------flattend_record_type

Sos_ptr<Record_type> flattend_record_type( const Sos_ptr<Record_type>& source_type,
                                           Bool with_groups,
                                           const Sos_string& new_type_name )
{
    ZERO_RETURN_VALUE( Sos_ptr<Record_type> );

    Sos_ptr<Record_type> t;

    create_record_type( &t, source_type, new_type_name, "/flat", "flattend_record_type" );
    t->flat_scope( false );  //jz 21.4.97
    add_flattend_record_type( t, with_groups, source_type );

    return t;
}

//---------------------------------------------------------------------As_bool_type::_type_info
/*
Type_info As_bool_type::_type_info;

SOS_INIT( as_bool )
{
    As_bool_type::_type_info._std_type      = std_type_integer;  //std_type_bool
    As_bool_type::_type_info._name          = "As_bool";
    As_bool_type::_type_info._nullable      = true;          // = _base_type.info()._nullable
    As_bool_type::_type_info._max_size      = 0;             // = _base_type.info()._max_size
    As_bool_type::_type_info._max_precision = 1;
    As_bool_type::_type_info.normalize();
}
*/
//-------------------------------------------------------------------As_bool_type::As_bool_type

As_bool_type::As_bool_type( const Sos_ptr<Field_type>& base_type,
                                const Sos_string& falsch, const Sos_string& wahr )
:
    Field_subtype ( &_type_info, base_type ),
    _falsch       ( falsch ),
    _wahr         ( wahr )
{
    _type_info._std_type      = std_type_integer;  //std_type_bool
    _type_info._name          = "As_bool";
    _type_info._nullable      = true;          // = _base_type.info()._nullable
    _type_info._max_size      = 0;             // = _base_type.info()._max_size
    _type_info._max_precision = 1;
    _type_info.normalize();

    _empty_is_null = !::sos::empty( _falsch )  &&  !::sos::empty( _wahr );
}

//---------------------------------------------------------------------As_bool_type::_obj_print

void As_bool_type::_obj_print( ostream* s ) const
{
    *s << *_base_type << ":Boolean(" << _falsch << "\",\"" << _wahr << "\")";
}

//---------------------------------------------------------------------------As_bool_type::null

Bool As_bool_type::null( const Byte* p ) const
{
    return _base_type->null( p )  ||  _base_type->empty( p );
}

//---------------------------------------------------------------------As_bool_type::write_text

void As_bool_type::write_text( const Byte* p, Area* buffer, const Text_format& ) const
{
    Sos_limited_text<30> text;

    _base_type->write_text( p, &text, raw_text_format );

    if( text == _falsch )  { buffer->assign( "0" ); return; }
    if( text == _wahr   )  { buffer->assign( "1" ); return; }

    text.xlat( tablcase );

    if( text == "0"
     || text == "n"
     || text == "f" )  { buffer->assign( "0" ); return; }

    if( text == "1"
     || text == "j"
     || text == "y"
     || text == "t" )  { buffer->assign( "1" ); return; }

    if( _empty_is_null  &&  ( text.length() == 0 || ::sos::empty( text ) ) )  return;

    throw_xc( "SOS-1218", this, c_str( text ) );  // Der text ist hier in kleine Buchstaben umgesetzt. Besser das Original nehmen
}

//----------------------------------------------------------------------As_bool_type::read_text

void As_bool_type::read_text( Byte* p, const char* text, const Text_format& ) const
{
    const String0_area* value;

    value = as_bool( text )? &_wahr : &_falsch;

/*jz 23.6.98
    if( text[0] && text[1] )  goto FEHLER;

    if( *text == '0' )  value = &_falsch;
    else
    if( *text == '1' )  value = &_wahr;
    else
        goto FEHLER;
*/
    _base_type->read_text( p, c_str( *value ) );

    //Optimierung: _falsch und _var enthalten als Dynamic_areas bereits den mit read_text()
    //konvertierten Code.

    return;

  //FEHLER: throw_xc( "SOS-1240", text );
}

//------------------------------------------------------------------As_numeric_type::_type_info
/*
Type_info As_numeric_type::_type_info;

SOS_INIT( as_numeric )
{
    As_numeric_type::_type_info._std_type      = std_type_float;
    As_numeric_type::_type_info._name          = "As_numeric";
    As_numeric_type::_type_info._nullable      = true;          // = _base_type.info()._nullable
    As_numeric_type::_type_info._max_size      = 0;             // = _base_type.info()._max_size
    As_numeric_type::_type_info._max_precision = 0;             // ?
    As_numeric_type::_type_info.normalize();
}
**/
//-------------------------------------------------------------As_numeric_type::As_numeric_type

As_numeric_type::As_numeric_type( const Sos_ptr<Field_type>& base_type )
:
    Field_subtype( &_type_info, base_type)
{
    _type_info._std_type      = std_type_float;
    _type_info._name          = "As_numeric";
    _type_info._nullable      = true;          // = _base_type.info()._nullable
    _type_info._max_size      = 0;             // = _base_type.info()._max_size
    _type_info._max_precision = 0;             // ?
    _type_info.normalize();
}

//---------------------------------------------------------------------------------------------

Bool As_numeric_type::null( const Byte* p ) const
{
    return _base_type->empty( p );
}

//------------------------------------------------------------------------As_numeric_type::set_null

void As_numeric_type::set_null( Byte* p ) const
{
    //if( !_nullable )  throw_null_error( "SOS-1186", this );
    
    _base_type->read_text( p, "" );
}

//--------------------------------------------------------------------------check_number_simply

static void check_number_simply( const char* nr, int len )
{
    // Prüfen, vor allem, dass keine Sonderzeichen drin sind, die SQL oder HTML verwirren könnten:
    const Byte* b_end = (const Byte*)nr + len;
    for( const Byte* b = (const Byte*)nr; b < b_end; b++ )   
    {
        if( !isalnum(*b)  &&  *b != ' ' && *b != '.' && *b != '+' && *b != '-' )  
        {
            string str = string(nr,len);
            throw_conversion_error( "SOS-1140", str.c_str() );
        }
    }
}

//------------------------------------------------------------------As_numeric_type::write_text

void As_numeric_type::write_text( const Byte* p , Area* buffer, const Text_format& f ) const
{
    _base_type->write_text( p, buffer, raw_text_format );

    if( f.decimal_symbol() != '.' )
    {
        char* b_end = buffer->char_ptr() + buffer->length();
        for( char* b = buffer->char_ptr(); b < b_end; b++ )   if( *b == '.' )  *b = f.decimal_symbol();
    }

    check_number_simply( buffer->char_ptr(), buffer->length() );
}

//-------------------------------------------------------------------As_numeric_type::read_text

void As_numeric_type::read_text( Byte* p, const char* text, const Text_format& f ) const
{
    if( f.decimal_symbol() == '.' )
    {
        check_number_simply( text, strlen(text) );
        _base_type->read_text( p, text );
    }
    else
    {
        Sos_limited_text<100> buffer = text;
        c_str(buffer);
        for( char* b = buffer.char_ptr(); *b; b++ )  if( *b == f.decimal_symbol() )  *b = '.';
        check_number_simply( buffer.char_ptr(), buffer.length() );
        _base_type->read_text( p, buffer.char_ptr() );
    }


/*
    Diese Prüfung ist doch nicht wichtig. Das kann dann doch die Datenbank prüfen. 
    Vielleicht erlaubt die ja Zahlenformate, die wir nicht kennen, z.B. "nan".

    char        buffer[100+1];
    char*       b       = buffer;
    char*       b_end   = b + NO_OF(buffer)-1;
    const char* t       = text;

    // Syntax prüfen:
    skip_white( &t );
    
    if( *t == '+' )  t++;
    else
    if( *t == '-' )  *b++ = *t++;

    while( t[0] == '0'  &&  isdigit(t[1]) )  t++;

    while( isdigit(*t) ) *b++ = *t++;

    if( *t )   // optimierung
    {  
        if( *t == f.decimal_symbol() )  *b++ = '.',  t++;

        while( isdigit(*t) )  *b++ = *t++;

        if( tabucase[*t] == 'E' ) 
        {
            *b++ = *t++;
            if( *t == '+' )  t++;
            else
            if( *t == '-' )  *b++ = *t++;
            while( isdigit(*t) )  *b++ = *t++;
        }

        skip_white( &t );
    }

    if( *t )  if( *t == ',' )  throw_conversion_error( "SOS-1436", c_str(text) );
                         else  throw_conversion_error( "SOS-1140", c_str(text) );

    buffer->assign( start );

    _base_type->read_text( p, c_str(text) );
*/
}

//-------------------------------------------------------------As_decimal_type::As_decimal_type

As_decimal_type::As_decimal_type( const Sos_ptr<Field_type>& base_type, int precision, int scale, bool nullable )
:
    Field_subtype( &_type_info, base_type)
{
    if( precision < scale )  throw_xc( "SOS-1437", precision, scale );

    _precision = precision;
    _scale     = scale;
    _nullable  = nullable;

    _type_info._std_type      = std_type_decimal;
    _type_info._name          = "As_decimal";
    _type_info._nullable      = nullable;         // = _base_type.info()._nullable
    _type_info._max_size      = _base_type->info()->_max_size;
    _type_info._max_precision = precision;
    _type_info._min_scale     = scale;
    _type_info._max_scale     = scale;
    _type_info.normalize();
}

//---------------------------------------------------------------------As_decimal_type::_get_param

void As_decimal_type::_get_param( Type_param* param ) const
{
    param->_precision = _precision;
    param->_scale     = _scale;
}

//----------------------------------------------------------------------------As_decimal_type::null

Bool As_decimal_type::null( const Byte* p ) const
{
    return _base_type->empty( p );
}

//------------------------------------------------------------------------As_decimal_type::set_null

void As_decimal_type::set_null( Byte* p ) const
{
    if( !_nullable )  throw_null_error( "SOS-1186", this );
    
    _base_type->read_text( p, "" );
}

//----------------------------------------------------------------------As_decimal_type::write_text

void As_decimal_type::write_text( const Byte* p , Area* buffer, const Text_format& f ) const
{
    _base_type->write_text( p, buffer, raw_text_format );

    if( f.decimal_symbol() != '.' )
    {
        char* b_end = buffer->char_ptr() + buffer->length();
        for( char* b = buffer->char_ptr(); b < b_end; b++ )   if( *b == '.' )  *b = f.decimal_symbol();
    }

/*
    Sos_limited_text<100> text;
    char*                 start;

    _base_type->write_text( p, &text, raw_text_format );

    // Syntax prüfen:
    c_str( text );  // 0-Byte sicherstellen
    char* q = text.char_ptr();

    skip_white( &q );
    start = q;

    if( *q == '-' || *q == '+' ) q++;
    while( isdigit( *q ) ) q++;

    if( *q )  // optimierung
    {  
        if( *q == '.' )  { *q = f.decimal_symbol(); q++; }
        while( isdigit( *q ) ) q++;
        skip_white( &q );
    }

    if( *q )  if( *q == ',' )  throw_conversion_error( "SOS-1436", c_str(text) );
                         else  throw_conversion_error( "SOS-1140", c_str(text) );

    buffer->assign( start );
*/
}

//-------------------------------------------------------------------As_decimal_type::read_text

void As_decimal_type::read_text( Byte* p, const char* text, const Text_format& f ) const
{
    if( f.decimal_symbol() == '.' )
    {
        _base_type->read_text( p, text );
    }
    else
    {
        Sos_limited_text<100> buffer = text;
        c_str(buffer);
        for( char* b = buffer.char_ptr(); *b; b++ )  if( *b == f.decimal_symbol() )  *b = '.';
        _base_type->read_text( p, buffer.char_ptr() );
    }

    //_base_type->read_text( p, c_str( t ) );
}

//---------------------------------------------------------------------As_text_type::_type_info
/*
Type_info As_text_type::_type_info;

SOS_INIT( as_text )
{
    As_text_type::_type_info._std_type      = std_type_char;
    As_text_type::_type_info._name          = "As_text";
    As_text_type::_type_info._nullable      = true;          // = _base_type.info()._nullable
    As_text_type::_type_info._max_size      = 0;             // = _base_type.info()._max_size
    As_text_type::_type_info._max_precision = 0;
    As_text_type::_type_info.normalize();
}
*/
//-------------------------------------------------------------------As_text_type::As_text_type

As_text_type::As_text_type( const Sos_ptr<Field_type>& base_type, int size )
:
    Field_subtype( &_type_info, base_type ),
    _size(size)
{
    _type_info._std_type      = std_type_char;
    _type_info._name          = "As_text";
    _type_info._nullable      = true;          // = _base_type.info()._nullable
    _type_info._max_size      = 0;             // = _base_type.info()._max_size
    _type_info._max_precision = 0;
    _type_info.normalize();
}

//---------------------------------------------------------------------As_text_type::write_text

void As_text_type::write_text( const Byte* p , Area* buffer, const Text_format& ) const
{
    Dynamic_area text ( max_display_field_size );
    _base_type->write_text( p, &text, raw_text_format );

    // Saacke-Besonderheit: _size schneidet linksbündig ab.
    // Dabei wird angenommen, das der Grundtyp numerisch ist und nur Nullen abgeschnitten werden.
    // Sollte mit Typinformation oder Text_format gelöst werden.
    // Preislistenpositionnr PIC 9(8) COMP-3 liefert 9 Ziffern, aber 8 werden verlangt.
    //int TEXT_TYPE_MIT_GROESSE_SCHNEIDET_AB_FUER_SAACKE;
    int n   = text.length();
    int off = 0;

    if( _size  &&  n > _size )  {
        off = n - _size;
        if( off > 20 || memcmp( text.ptr(), "00000000000000000000", off ) != 0 )  throw_xc( "SOS-1113", n, _size );
        n = _size;
    }

    buffer->assign( text.char_ptr() + off, n );
    //write_string( text.char_ptr() + off, n, buffer, f );
}

//----------------------------------------------------------------------As_text_type::read_text

void As_text_type::read_text( Byte* p, const char* t, const Text_format& ) const
{
    _base_type->read_text( p, t );
}

//-------------------------------------------------------------As_saacke_posnr_type::_type_info
/*
Type_info As_saacke_posnr_type::_type_info;

SOS_INIT( as_saacke_posnr )
{
    As_saacke_posnr_type::_type_info._std_type      = std_type_char;
    As_saacke_posnr_type::_type_info._name          = "As_saacke_posnr";
    As_saacke_posnr_type::_type_info._nullable      = true;          // = _base_type.info()._nullable
    As_saacke_posnr_type::_type_info._max_size      = 0;             // = _base_type.info()._max_size
    As_saacke_posnr_type::_type_info._max_precision = 0;
    As_saacke_posnr_type::_type_info.normalize();
}
*/
//---------------------------------------------------As_saacke_posnr_type::As_saacke_posnr_type

As_saacke_posnr_type::As_saacke_posnr_type( const Sos_ptr<Field_type>& base_type )
:
    Field_subtype( &_type_info,base_type )
{
    _type_info._std_type      = std_type_char;
    _type_info._name          = "As_saacke_posnr";
    _type_info._nullable      = true;          // = _base_type.info()._nullable
    _type_info._max_size      = 0;             // = _base_type.info()._max_size
    _type_info._max_precision = 0;
    _type_info.normalize();
}

//-------------------------------------------------------------As_saacke_posnr_type::write_text

void As_saacke_posnr_type::write_text( const Byte* p , Area* buffer, const Text_format& ) const
{
    Sos_limited_text<9> text;
    _base_type->write_text( p, &text, raw_text_format );

    if( length( text ) == 9 ) {      // Kann eine gepackte Zahl mit ungerader Stellenzahl sein
        if( text[ 0 ] != '0' )  throw_xc( "SOS-1113", length( text ) , 8 );
        memmove( &text[0], &text[1], 8 );
        text.length( 8 );
    }
                                                             // '/' und '.' nur zur Verdeutlichung!
    if( text[ 7 ] == '0' ) {                                 // 9/99.99.99.0  => 9/99.99.99
        text.length( 7 );
        if( text[ 5 ] == '0'  &&  text[ 6 ] == '0' ) {       // 9/99.99.00.0  => 9/99.99
            text.length( 5 );
            if( text[ 3 ] == '0'  &&  text[ 4 ] == '0' ) {   // 9/99.00.00.0  => 9/99
                text.length( 3 );
            }
        }
    }

    buffer->assign( text );
}

//----------------------------------------------------------------------As_saacke_posnr_type::read_text

void As_saacke_posnr_type::read_text( Byte* p, const char* t, const Text_format& ) const
{
    Sos_limited_text<8> text = t;
    memset( &text[0] + text.length(), '0', 8 - text.length() );
    text.length( 8 );
    _base_type->read_text( p, c_str( text ) );
}

//-------------------------------------------------------------------------------modified_field

Sos_ptr<Field_descr> modified_field( Field_descr* field_descr, const Sos_string& descr )
{
    ZERO_RETURN_VALUE( Sos_ptr<Field_descr> );

    if( empty( descr ) )  return field_descr;
    //jz 22.4.97 Sos_ptr<Field_descr> f = Field_descr::create( *field_descr );
    //1.11.98 (egcs) Sos_ptr<Field_descr> f = obj_copy( *field_descr );
    Sos_ptr<Field_descr> f = OBJ_COPY( Field_descr, *field_descr );
    modify_field( f, descr );
    return f;
}

//---------------------------------------------------------------------------------modify_field

void modify_field( Field_descr* field_descr, const Sos_string& descr /*[name: type]*/ )
{
    const char* p = c_str( descr );
    while( sos_isalnum( *p )  ||  *p == '-'  ||  *p == '_' )  p++;
    int n = p - c_str( descr );
    skip_white( &p );
    if( !*p ) {
        if( !empty( descr ) )   field_descr->name( descr );
    } else {
        if( *p != ':' )  throw_syntax_error( "SOS-1184", ":", p - c_str( descr ) );
        p++;
        skip_white( &p );

        Sos_string  new_name = as_string( c_str( descr ), n );
        if( !empty( new_name ) )   field_descr->name( new_name );

        const char* p0 = p;   // Für Fehlermeldung

        if( strncmpi( p, "bool", 4 ) == 0 ) {
            p += 4;
            if( strncmpi( p, "ean", 3 ) == 0 )  p += 3;
            Sos_string falsch, wahr;
            if( *p++ != '(' )  goto FEHLER;
            get_string( &p, &falsch );
            if( *p++ != ',' )  goto FEHLER;
            get_string( &p, &wahr );
            if( *p++ != ')' )  goto FEHLER;
            Sos_ptr<As_bool_type> t = SOS_NEW( As_bool_type( field_descr->type_ptr(), falsch, wahr ) );
            field_descr->_type_ptr = t;
        }
        else
        if( strncmpi( p, "datetime", 8 ) == 0 ) {
            Sos_limited_text<50> format;
            p += 8;
            skip_white( &p );
            if( *p == '(' ) {
                p++;
                skip_white( &p );
                if( *p != '"' && *p != '\'' ) goto FEHLER;
                char quote = *p++;
                while( *p && *p != quote )  format += *p++;
                if( *p != quote ) goto FEHLER;
                p++;
                skip_white( &p );
                if( *p != ')' )  goto FEHLER;
                p++;
                skip_white( &p );
            } else {
                format = std_date_time_format_iso;
            }
            if( *p ) goto FEHLER;
            Sos_ptr<As_date_time_type> t = SOS_NEW( As_date_time_type( field_descr->type_ptr(), c_str( format ) ) );
            field_descr->_type_ptr = t;
        }
        else
        if( strncmpi( p, "date", 4 ) == 0 ) {
            Sos_limited_text<50> format;
            p += 4;
            skip_white( &p );
            if( *p == '(' ) {
                p++;
                skip_white( &p );
                if( *p != '"' && *p != '\'' ) goto FEHLER;
                char quote = *p++;
                while( *p && *p != quote )  format += *p++;
                if( *p != quote ) goto FEHLER;
                p++;
                skip_white( &p );
                if( *p != ')' )  goto FEHLER;
                p++;
                skip_white( &p );
            } else {
                format = std_date_format_iso;
            }
            if( *p ) goto FEHLER;
            Sos_ptr<As_date_type> t = SOS_NEW( As_date_type( field_descr->type_ptr(), c_str( format ) ) );
            field_descr->_type_ptr = t;
        }
        else
        if( strcmpi( p, "numeric" ) == 0 ) {
            Sos_ptr<As_numeric_type> t = SOS_NEW( As_numeric_type( field_descr->type_ptr() ) );
            field_descr->_type_ptr = t;
        }
        else
        if( strncmpi( p, "decimal", 7 ) == 0 ) 
        {
            int precision = 0, scale = 0;
            p += 7;  skip_white( &p );
            if( *p == '(' )
            {
                p++; skip_white( &p );
                while( isdigit(*p) )  precision = 10*precision + *p++ - '0';
                skip_white( &p );
                if( *p == ',' )
                {
                    p++; skip_white( &p );
                    while( isdigit(*p) )  scale = 10*scale + *p++ - '0';
                    skip_white( &p );
                }
                if( *p != ')' )  goto FEHLER;
                p++; skip_white( &p );
                if( *p )  goto FEHLER;
            }
            Sos_ptr<As_decimal_type> t = SOS_NEW( As_decimal_type( field_descr->type_ptr(), precision, scale ) );
            field_descr->_type_ptr = t;
        }
        else
        if( strncmpi( p, "text", 4 ) == 0 ) {
            p += 4;
            int n = 0;
            if( *p == '(' ) {
                p++;
                while( isdigit( *p ) )  n = n*10 + *p++ - '0';
                if( *p++ != ')' )  goto FEHLER;
            }
            if( *p )  goto FEHLER;

            Sos_ptr<As_text_type> t = SOS_NEW( As_text_type( field_descr->type_ptr(), n ) );
            field_descr->_type_ptr = +t;
        }
        else
        if( strncmpi( p, "saackeposnr", 11 ) == 0 ) {
            p += 11;
            if( *p )  goto FEHLER;

            Sos_ptr<As_saacke_posnr_type> t = SOS_NEW( As_saacke_posnr_type( field_descr->type_ptr() ) );
            field_descr->_type_ptr = +t;
        }
        else
        if( strncmpi( p, "ro", 2 ) == 0 ) {
            p += 2;
            field_descr->read_only( true ); // wird von den Dateitypen abgefragt
        }
        else
        if( strncmpi( p, "readonly", 8 ) == 0 ) {
            p += 8;
            field_descr->read_only( true ); // wird von den Dateitypen abgefragt
        }
        else {
            FEHLER: 
            if( empty( new_name ) )  new_name = field_descr->name();  
            throw_xc( "SOS-1217", c_str( new_name ), p0 );
        }
    }
}

} //namespace sos
