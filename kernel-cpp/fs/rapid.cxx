//#include <precomp.h>
//#define MODULE_NAME "rapid"
/* rapid.cpp                                    (c) SOS GmbH Berlin
                                                Joacim Zschimmer

   Dateiverarbeitung (Fd, Openpar) in rapidfil.cpp.
*/

#include "../kram/sos.h"
#include "../kram/sosalloc.h"
#include "../kram/xlat.h"
#include "../kram/sosstrea.h"
#include "../kram/sosobj.h"
#include "../kram/log.h"
#include "rapid.h"

#include <stdlib.h>          // time()
#include <time.h>            // time()

namespace sos {

//--------------------------------------Rapid::Task_connection::Task_connection

Rapid::Task_connection::Task_connection( const Sos_object_ptr& conn_ptr )
:
    _zero_              ( this+1 ),
    _conn_ptr           ( conn_ptr )
{
}

//--------------------------------------Rapid::Task_connection::~Task_connection

Rapid::Task_connection::~Task_connection()
{
    sos_free( _buffer_ptr );
    _buffer_ptr = 0;
}

//------------------------------------------Rapid::Task_connection::write_cmd

void Rapid::Task_connection::write_cmd( Cmd cmd )
{
    _my_word = (uint4)time(0) ^ (int4)rand();

    _output_stream.write_byte( (Byte)cmd );
    _output_stream.write_fixed( "\0\0\0", 3 );
    _output_stream.write_fixed( &_server_word, sizeof _server_word );
    _output_stream.write_fixed( &_my_word    , sizeof _my_word );

    if( cmd == taxccal ) {
        _output_stream.write_fixed( "\0\0\0\0", 4 );
        _output_stream.write_fixed( &_task_id, sizeof _task_id );
    }
}

//-------------------------------------------------Rapid::Task_connection::call

void Rapid::Task_connection::call( const char* entry_name )
{
    if( _connection_lost )  throw_connection_lost_error( "SOS-1213" );
    if( _active )  throw_xc( "SOS-1226" );  // Locked_error?
    _active = true;

    if( !_buffer_ptr ) {
        _buffer_ptr = (Byte*)sos_alloc( rapid_max_task_buffer_size, "Rapid::Task_connection" );
    }

    _output_stream.reset( _buffer_ptr, rapid_max_task_buffer_size );
    write_cmd( taxccal );
    write_string_ebcdic( &_output_stream, entry_name, 8 );
}

//------------------------------------------------Rapid::Task_connection::write_record

void Rapid::Task_connection::write_record()
{
    if( _connection_lost )  throw_connection_lost_error( "SOS-1213" );

    try {
        _conn_ptr->obj_put( _output_stream.area() );
    }
    catch( const Connection_lost_error& ) {     // Original durchlassen
        _connection_lost = true;
        throw;
    }
    catch( const Eof_error& ) {
        _connection_lost = true;
        throw_connection_lost_error( "SOS-1213" );
    }
    catch( const Xc& ) {
        _connection_lost = true;
        //Connection_lost_error y ( "SOS-1213" );
        //y.chain( x );
        //int CHAIN_EXCEPTIONS;
        //throw y;
        throw;
    }
}

//-----------------------------------------Rapid::Task_connection::_read_header

void Rapid::Task_connection::_read_header()
{
    if( _connection_lost )  throw_connection_lost_error( "SOS-1213" );

    Byte                   c;
    uint4                  my_word;

    Area answer ( _buffer_ptr, rapid_max_task_buffer_size );
    try {
        answer.assign( _conn_ptr->obj_get() );
        _active = false;
    }
    catch( const Connection_lost_error& ) {      // Original durchlassen
        _connection_lost = true;
        throw;
    }
    catch( const Eof_error& ) {                  // Fileserver hat TCP-Verbindung geschlossen
        _connection_lost = true;
        throw_connection_lost_error( "SOS-1213" );
    }
    catch( const Xc& ) {
        _connection_lost = true;
        throw;
        //Connection_lost_error y ( "SOS-1213" );
        //y.chain( x );
        //int CHAIN_EXCEPTIONS;
        //throw y;
    }

    _input_stream.reset( answer );

    _input_stream.read_byte( &c );
    _input_stream.skip_bytes( 3 );
    _input_stream.read_fixed( &_server_word, sizeof _server_word );
    _input_stream.read_fixed( &my_word, sizeof my_word );
    _input_stream.skip_bytes( 8 );

    if( my_word != _my_word )  throw_data_error( "SOS-1165" );

    switch( (Cmd) c )
    {
        case taxcret: _input_stream.skip_bytes( 8 );  break;

        case taxcxcp:
        {
            char  exception_name [ 8 + 1 ];
            char  error_code     [ 16  + 1 ];

            xlat( exception_name,
                  (const Byte*) _input_stream.read_bytes( sizeof exception_name - 1 ),
                  sizeof exception_name, ebc2iso );

            exception_name[ sizeof exception_name - 1 ] = 0;

            char *p = strchr( exception_name, ' ' );
            if ( p ) {
                *p = 0;  // js ueberfluessige Blanks weg
            }

            xlat( error_code, (const Byte*) _input_stream.read_bytes( sizeof error_code - 1 ),
                  sizeof error_code, ebc2iso );
            error_code[ sizeof error_code - 1 ] = 0;

            p = strchr( error_code, ' ' );
            if ( p ) {
                *p = 0;  // js ueberfluessige Blanks weg
            }

            Named_xc x ( exception_name, error_code );

            if( strcmp( exception_name, "CONNLOST" ) == 0 )  _connection_lost = true;  // RU02 Client-Id ungültig

            if( strcmp( exception_name, "NOMEMORY" ) == 0 )  throw x;
                                                       else  x.throw_right_typed();
        }

        default: throw_data_error( "SOS-1165" );
    }
}

//--------------------------------------------Rapid::Task_connection::read

void Rapid::Task_connection::read_answer()
{
    if( _connection_lost )  throw_connection_lost_error( "SOS-1213" );

    _read_header();
}

} //namespace
