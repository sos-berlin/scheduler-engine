#define MODULE_NAME "testfs"

#define PORT "tcp 4001"

#include "../kram/sysdep.h"

#include <limits.h>

#if defined SYSTEM_UNIX
#   include <sys/resource.h>
#   include <errno.h>
#endif

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
//#include <pointer.h>
#include "../kram/log.h"
#include "../kram/sosfact.h"
#include "../kram/sosarray.h"
#include "fsman.h"

namespace sos {

//---------------------------------------------------------------------------------------static

const int4  max_demo_connections = 3;
ostream*    fs_log = 0;

//------------------------------------------------------------------------Fs_manager::Fs_manager

Fs_manager::Fs_manager( const Sos_string& port )
:
    _zero_( this+1 ),
    _port ( port )
{
    _connection_array.obj_const_name( "Fs_manager::_connection_array" );
}

//-----------------------------------------------------------------------Fs_manager::~Fs_manager

Fs_manager::~Fs_manager()
{
    //int KILL_CREATE_MSG_NICHT_MOEGLICH;

    *fs_log << "\n--- FILESERVER WIRD BEENDET ---\n\n";
}

//-------------------------------------------------------------------------Fs_manager::_obj_msg

SOS_BEGIN_MSG_DISPATCHER( Fs_manager )
    SOS_DISPATCH_MSG( run  )
    SOS_DISPATCH_MSG( ack  )
    SOS_DISPATCH_MSG( error )
SOS_END_MSG_DISPATCHER

//----------------------------------------------------------------------------Fs_manager::start

void Fs_manager::start()
{
    if( !fs_log ) {
#       if defined SYSTEM_WIN
            fs_log = new ofstream( "sosfs.log" );  // wird nicht freigegeben!
#        else
            fs_log = &std::cerr;
#       endif
    }

#   if defined SYSTEM_UNIX
        // Größere Anzahl File handles ermöglichen:
        struct rlimit limit;
        int    rc;
        rc = getrlimit( RLIMIT_NOFILE, &limit );
        if( rc == -1 )  throw_errno( errno, "getrlimit" );
        limit.rlim_cur = limit.rlim_max;
        rc = setrlimit( RLIMIT_NOFILE, &limit );
        if( rc == -1 )  throw_errno( errno, "setrlimit" );
#   endif
}

//---------------------------------------------------------------------Fs_manager::_obj_run_msg

void Fs_manager::_obj_run_msg( Run_msg* )
{
    request_connection();
    // wann wird der Fileserver beendet?
}

//---------------------------------------------------------------------Fs_manager::_obj_ack_msg

void Fs_manager::_obj_ack_msg( Ack_msg* m )
{
    // Neue TCP-Verbindung mit accept() aufgebaut

    int i;
    for( i = _connection_array.first_index(); i <= _connection_array.last_index(); i++ ) {
        if( +_connection_array[ i ]._conn == m->source_ptr() )  break;
    }

    if( i <= _connection_array.last_index() )
    {
        Connection* c = &_connection_array[ i ];
        switch( c->_status )
        {
            case sta_creation:
            {
                c->_status = sta_running;
                *fs_log << *c->_conn << " baut Verbindung auf" << endl;
                post_request( Run_msg( c->_conn, this ) );
                request_connection();
                break;
            }

            case sta_running:
            {
                c->_status = sta_closing;
                *fs_log << *c->_conn << " baut Verbindung ab" << endl;
                post_request( End_msg( c->_conn, this ) );
                break;
            }

            case sta_closing:
            {
                *fs_log << *c->_conn << " geschlossen" << endl;
                c->_status = sta_none;
                if( c->_conn->obj_request_semaphore() == 0 )  c->_conn.del();
                break;
            }

            default:
                *fs_log << *c->_conn << " liefert unerwartete Bestätigung" << endl;
        }
    }
    else {
        *fs_log << "Ack_msg von unbekanntem Objekt" << endl;
    }
}

//-----------------------------------------------------------------------Fs_manager::_obj_error_msg

void Fs_manager::_obj_error_msg( Error_msg* m )
{
    int last_index = _connection_array.last_index();

    Sos_object_ptr src = m->source_ptr();  // wegen Compiler-Absturz in Solaris

    int i;
    for( i = _connection_array.first_index(); i <= last_index; i++ ) {
        if( _connection_array[ i ]._conn == src )  break;
    }

    if( i <= last_index )
    {
        Connection* c = &_connection_array[ i ];
        *fs_log << *c->_conn << ": Fehler " << m->error() << endl;
/*
        switch( c->_status )
        {
            case sta_creation:
            {
                //request_connection();
                break;
            }

            default: ;
        }
*/
        c->_status = sta_none;

        if( c->_conn->obj_request_semaphore() == 0 ) {     // Keine Nachrichten offen?
            c->_conn.del();
        }

    }
    else {
        *fs_log << *m->source_ptr() << ": " << m->error() << endl;
    }

    //jz 13.5.97 Führt zu SOCKET-71 protocol error, offenbar weil schon ein listen/accept aktiv ist:
    //jz 13.5.97 request_connection();
}

//----------------------------------------------------------------------Fs_manager::request_connection

void Fs_manager::request_connection()
{
    if( _allowed_connections_count == 0 )  throw_xc( "SOS-1249", max_demo_connections );
    _allowed_connections_count--;

    LOGI( "Fs_manager::request_connection\n");
    Sos_string conn_name ( _port );
    append_option( &conn_name, " -sam3 |fileserver -filename-prefix=", _filename_prefix );
    _connection_array.add( Connection( sos_factory_ptr()->request_create( this, conn_name ),
                                       sta_creation ) );

    if( !_first_request_logged ) {
        _first_request_logged = true;
        *fs_log << "Fileserver erwartet Clients auf Port " << _port << endl;
    }

    //int BEI_FEHLER_ABBRUCH;
}

//---------------------------------------------------------------------------------------fs_run

static void fs_run( const Sos_string& port )
{
    *fs_log << "Fileserver erwartet Clients auf Port " << port << endl;
    Fs_manager( port ).obj_run();
}

} //namespace sos
