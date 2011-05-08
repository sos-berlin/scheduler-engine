#define MODULE_NAME "storable"
#define COPYRIGHT   "(c) SOS GmbH Berlin"
#if 0
//#pragma implementation

#include <stdlib.h>

#include <sosstrng.h>
#include <sos.h>
#include <sosfield.h>
#include <storable.h>
#include <absfile.h>

/* s. sosstrg0.cxx
int as_int( const char* str )
{
    int as_int_convertiert_maessig;
    for( int i = 0; i < length( str ); i++ ) {
        if( str[ i ] < '0'  ||  str[ i ] > '9' )  THROW( Numeric_syntax_error( i ));
    }

    return atoi( str );
}
*/

#if !defined ( SYSTEM_WIN )
//    extern char* itoa( int, char*, int );
#endif

/*
Sos_string as_string( int i )
{
    char buffer [ 17 ];
    itoa( i, buffer, 10 );
    return Sos_string( buffer );
}
*/
//---------------------

Storable_as_string_type storable_as_string_type;



void Storable_as_string_type::print( const Byte* p, ostream* s, const Text_format& ) const
{
    Sos_string string;
    ((const Storable_as<Sos_string>*)p)->object_store( &string );
    s->write( c_str( string ), length( string ) );
}


void Storable_as_string_type::input( Byte* p, istream* s, const Text_format& ) const
{
    Dynamic_area buffer ( 2000 );
    s->read( buffer.char_ptr(), buffer.size() );
    buffer.length( s->gcount() );
    ((Storable_as<Sos_string>*)p)->object_load( as_string( buffer ) );
}


// ------------------------------------------------------------- Abs_name_file::rewind

void  Abs_name_file::rewind( Key::Number n )
{
    _file_ptr->rewind( n );
}

// ------------------------------------------------------------- Abs_name_file::get_record

void Abs_name_file::get_record( Area& area )
{
    Dynamic_area input_area;
    area.allocate_min( 100 );  //?
    ostrstream s ( area.char_ptr(), area.size() ); xc;

    input_area.allocate_min( 500 );  //??? sehr unschoen
    _file_ptr->get( input_area ); xc;

    format( input_area, &s );

    area.length( strlen( area.char_ptr() ) ); // NTS erwartet !!!
// ???    if ( current_key().length() == 0 )
// ???    {
// ???      current_key_ptr( &(_file_ptr->current_key_ptr()) );
// ???    }

  exceptions
}

// ------------------------------------------------------------- Abs_name_file::get_record_key

void Abs_name_file::get_record_key( Area& area, const Key& key )
{
    set( key );
    get_record( area );
}

// ------------------------------------------------------------- Abs_name_file::set

void Abs_name_file::set( const Key& key )
{
    _file_ptr->set( key );
}

#endif
