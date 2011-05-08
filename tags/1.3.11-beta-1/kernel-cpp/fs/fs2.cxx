#define MODULE_NAME "fs2"
#define COPYRIGHT   "© SOS GmbH Berlin"
#define AUTHOR      "Joacim Zschimmer"

//#define USE_SOCKET
#define HOST "192.0.0.20"
#define PORT "4100"

#include <sysdep.h>

#if defined SYSTEM_SOLARIS
    #define O_BINARY 0
    #define SERVER
#else
    #define CLIENT
#endif

#include <sosstrng.h>

#if 0
    #define STRICT
    #include <windows.h>
    int __stdcall WinMain(

        HINSTANCE  hInstance,	// handle of current instance
        HINSTANCE  hPrevInstance,	// handle of previous instance
        LPSTR  lpszCmdLine,	// address of command line
        int  nCmdShow 	// show state of window
    )
    {
        MessageBox( 0/*hwndParent*/, "Hallo", "SOS", MB_TASKMODAL | MB_ICONINFORMATION );
        return 0;
    }
#else


#include <stdlib.h>    // errno
#include <fcntl.h>
//#include <io.h>
#include <stdio.h>
#include <time.h>      // Für Sos_object_id._check_value
#include <sys/stat.h>


#include <sos.h>
//#include <sosstrea.h>

#include <msec.h>
#include <log.h>

#include <sosfield.h>
#include <sosobjd.h>
#include <sosmsg.h>
#include <sossock.h>
#include <sam.h>
#include <streamfl.h>

//#include <sosarray.h>
//#include <soslimtx.h>




//----------------------------------------------------------------------------Msg_proxy_output_file
/*
struct Msg_proxy_output_file : Msg_output_file_if
{
    typedef Msg_output_file_if  Base_class;

                                Msg_proxy_output_file   ( Sos_proxy_client*, const char* filename, uint flags, uint protection = 0  );
                               ~Msg_proxy_output_file   ();

  //void                        open                    ( const char* filename, uint flags, uint protection = 0 );

  private:
  //void                       _obj_msg                 ( Sos_msg* );
    void                       _obj_print               ( ostream* ) const;

    Sos_proxy*                 _proxy_ptr;
    Sos_proxy_client*          _proxy_client_ptr;
  //Sos_object*                _source_ptr;             // EIGENTLICH PRO AUFTRAG EINMAL
  //Bool                       _answered;
};

//-----------------------------------------------------Msg_proxy_output_file::Msg_proxy_output_file

Msg_proxy_output_file::Msg_proxy_output_file( Sos_proxy_client* proxy_client_ptr,
                                              const char* filename, uint flags, uint protection )
:
    _proxy_client_ptr ( proxy_client_ptr )
{
    _proxy_ptr = proxy_client_ptr->open( filename, flags, protection );
}

//-----------------------------------------------------Msg_proxy_output_file::Msg_proxy_output_file

Msg_proxy_output_file::~Msg_proxy_output_file()
{
    delete _proxy_ptr;
}
*/
//----------------------------------------------------------------------Msg_proxy_output_file::open
/*
void Msg_proxy_output_file::open( const char* filename, uint flags, uint protection )
{
    _answered = false;

    Byte parameter_record [ sos_client__open_field_length ];

    write_field_by_descr( Sos_client_interface::Ext_open::_name, parameter_record, filename );
    send( &Spec_msg( _proxy_client_ptr, this, (int)sc_open, CONST_AREA( parameter_record ) ) );

/// Msg_proxy_output_file wird implizit im client_register eingetragen, aber nicht entfernt ///
    int AUS_REGISTER_ENTFERNEN;

    while( !_answered ) {
        sos_msg_dispatcher( 1 );
    }
}
*/
//------------------------------------------------------------------Msg_proxy_output_file::_obj_msg
/*
void Msg_proxy_output_file::_obj_msg( Sos_msg* m )
{
    if( m->source_ptr() == _proxy_client_ptr )
    {
        switch( m->type() )
        {
            case msg_object_ref:             // Antwort für open()
            {
                _answered = true;
                _proxy_ptr = static_cast< Sos_proxy* >( static_cast< Object_ref_msg* >( m )->object_ptr() );
                break;
            }

            default:
            {
                Base_class::_obj_msg( m );
            }
        }
    }
    else
    if( m->source_ptr() == _proxy_ptr )  // Botschaft vom Server
    {                                // an Client weiterreichen
        //Sos_msg* msg_ptr = ms.new_copy();
        m->dest_ptr( _source_ptr );
        _source_ptr = 0;             // Nur, wenn dies die letzte Antwort ist; Auftrag ist abgeschlossen
        m->source_ptr( this );
        send( m );
        //delete msg_ptr;              // NICHT BEI EXCEPTION?
    }
    else                             // Botschaft vom Client
    {                                // an Server weiterreichen
        if( _source_ptr )  throw Xc( "BUSY" );
        _source_ptr = m->source_ptr();      // merken für die Antwort. NUR EIN AUFTRAG AUF EINMAL!
        //Sos_msg* msg_ptr = ms.new_copy();
        m->dest_ptr( _proxy_ptr );
        m->source_ptr( this );
        send( m );
        //delete msg_ptr;              // NICHT BEI EXCEPTION?
    }
}
*/
//----------------------------------------------------------------Msg_proxy_output_file::_obj_print
/*
void Msg_proxy_output_file::_obj_print( ostream* s ) const
{
    *s << "Msg_proxy_output_file";
}
*/
//---------------------------------------------------------------------------------------------Main
/*
struct Main : Sos_object
{
    typedef Sos_object          Base_class;


                                Main                    ();
                               ~Main                    ();


  private:
};
*/
//-------------------------------------------------------------------------------------------Server

struct Server : Sos_object
{
    BASE_CLASS( Sos_object )

                                Server                  ();
                               ~Server                  ()       { LOG( "~Server\n" ); }

  //void                       _obj_open_msg            ( Open_msg* );
  //void                       _obj_run_msg             ( Run_msg* );
    void                       _obj_ack_msg             ( const Ack_msg* );
    void                       _obj_object_ref_msg      ( const Object_ref_msg* );

    Sos_ptr<Sos_object_register>  _register_ptr;
    Msg_comm_agent             _comm_agent;
    Sos_ptr<Sos_login>         _login_ptr;
    Sos_object_ptr             _socket_ptr;
    Bool                       _socket_waits_for_connect;
    Record_to_sam              _record_to_sam;
    Sam_to_record              _sam_to_record;
    Sos_object*                _input_ptr;
};


Server::Server()
:
    _comm_agent ( "server" ),
    _socket_waits_for_connect ( false ),
    _register_ptr ( SOS_NEW_PTR( Sos_object_register ) ),
    _login_ptr ( SOS_NEW_PTR( Sos_login ) )
{
    _register_ptr->register_object( _login_ptr, Sos_login_interface::id() );
    _comm_agent.register_ptr( _register_ptr );

    _comm_agent.channel_ptr( &_record_to_sam );
    _sam_to_record.obj_output_ptr( &_comm_agent );

    #if defined USE_SOCKET
        _socket_waits_for_connect = true;
        Create_msg create_msg ( sos_factory_ptr, this, "tcp:" PORT );
        request( &create_msg );
    #endif
}

/*
void Server::_obj_open_msg( Open_msg* m )
{
    _input_ptr = m->source_ptr();

    #if defined USE_SOCKET
        _record_to_sam.obj_output_ptr( &_socket );
        _socket.obj_output_ptr( &_sam_to_record );

        request( &Open_msg( &_socket, this, "tcp:" PORT ) );
     #else
        reply( Ack_msg( _input_ptr, this ) );
    #endif
}

void Server::_obj_run_msg( Run_msg* m )
{
    _input_ptr = m->source_ptr();

    #if defined USE_SOCKET
        request( &Run_msg( &_socket, this ) );
     #else
        reply( Ack_msg( _input_ptr, this ) );
    #endif
}
*/


void Server::_obj_ack_msg( const Ack_msg* )
{
    ASSERT_VIRTUAL( _obj_ack_msg );

    //reply( Ack_msg( _input_ptr, this ) );
    if( _socket_waits_for_connect ) {
        _socket_waits_for_connect = false;
        Run_msg run_msg ( _socket_ptr, this );
        request( &run_msg );
    }
}

void Server::_obj_object_ref_msg( const Object_ref_msg* m )
{
    ASSERT_VIRTUAL( _obj_object_ref_msg );

    //reply( Ack_msg( _input_ptr, this ) );
    if( _socket_waits_for_connect ) {
        _socket_waits_for_connect = false;
        _socket_ptr = m->object_ptr();
        _record_to_sam.obj_output_ptr( _socket_ptr );
        ((Sos_msg_filter*)(Sos_object*)_socket_ptr)->obj_output_ptr( &_sam_to_record );
        Run_msg run_msg ( _socket_ptr, this );
        request( &run_msg );
    }
}

//-------------------------------------------------------------------------------------------Client

struct Client : Sos_object
{
    typedef Sos_object          Base_class;

                                Client                  ( Server* );
                               ~Client                  ();

  //void                        login                   ( const char* name );

  protected:
    void                       _obj_run_msg             ( Run_msg* );
    void                       _obj_ack_msg             ( const Ack_msg* );

    Sos_new_ptr<Sos_object_register> _register_ptr;
    Sos_object*                _runner_ptr;
    Sos_object_ptr             _socket_ptr;
    Record_to_sam              _record_to_sam;
    Sam_to_record              _sam_to_record;
    Msg_comm_agent             _comm_agent;
    Sos_login_handle           _login_handle;
    Sos_client_handle          _client_handle;

    Sos_stream_file            _input_file;
    Sos_object*                _output_file_ptr;
};


Client::Client( Server* server_ptr )
:
    _comm_agent        ( "client"     ),
    _login_handle      ( &_comm_agent ),
    _client_handle     ( &_login_handle ),
    _input_file        ( "z:/jz.frm", O_RDONLY | O_BINARY )
{
    _comm_agent.register_ptr( _register_ptr );

    #if defined USE_SOCKET
        _socket_ptr = sos_factory_ptr->obj_create( "tcp:" HOST "/" PORT );
        _record_to_sam.obj_output_ptr( _socket_ptr );
        _socket.obj_output_ptr( &_sam_to_record );
     #else
        _record_to_sam.obj_output_ptr( &server_ptr->_sam_to_record );
        server_ptr->_record_to_sam.obj_output_ptr( &_sam_to_record );
    #endif

    _comm_agent.channel_ptr( &_record_to_sam );
    _sam_to_record.obj_output_ptr( &_comm_agent );

    #if defined USE_SOCKET
        Run_msg run_msg( &_socket, this );
        request( &run_msg );
    #endif

    _client_handle.login( "jz", "password" );

    _output_file_ptr = _client_handle.open( "c:/tmp/output", O_CREAT | O_BINARY, S_IREAD | S_IWRITE );
    _input_file.obj_owner_ptr( this );
    _input_file.obj_pipe_out( _output_file_ptr );
}

Client::~Client()
{
    LOG( "~Client\n" );
}

//------------------------------------------------------------------------------Client::_obj_run_msg

void Client::_obj_run_msg( Run_msg* m )
{
    _runner_ptr = m->source_ptr();
    m->dest_ptr( &_input_file );
    m->source_ptr( this );
    request( m );
}

//-----------------------------------------------------------------------------Client::_obj_ack_msg

void Client::_obj_ack_msg( const Ack_msg* m )
{
    if( m->source_ptr() == &_input_file )
    {
        reply( Ack_msg( _runner_ptr, this ) );
    }
    else
    {
        Base_class::_obj_ack_msg( m );
    }
}

//-----------------------------------------------------------------------------------------sos_main

int sos_main( int, char** )
{
try {
//#if !defined CLIENT
    Server server;
  //server.obj_open( "" );
  //server.obj_run();
//#else
    Client client ( &server );
    client.obj_run();
//#endif

    uint4 time = elapsed_msec();

    SHOW_MSG( ( elapsed_msec() - time ) << "ms" );
    return 0;

  exception_handler:
    SHOW_EXCEPTION( MODULE_NAME );
    return 1;
}
catch(...)
{
    throw;
}
}
#endif
