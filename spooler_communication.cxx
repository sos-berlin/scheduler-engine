// $Id$
/*
    Hier sind implementiert

    Communication::Connection
    Communication
*/


#include "spooler.h"
#include "../kram/sleep.h"
#include "../kram/sos_java.h"

using namespace std;

namespace sos {
namespace scheduler {

const int wait_for_port_available = 2*60;   // Soviele Sekunden warten, bis TCP- oder UDP-Port frei wird

#ifdef Z_WINDOWS
#   include <io.h>
    const int EWOULDBLOCK   = 10035;
    const int ENOTSOCK      = 10038;
    const int EADDRINUSE    = WSAEADDRINUSE;
    const int ENOBUFS       = WSAENOBUFS;
#   define ioctl    ioctlsocket
#   define isatty   _isatty
#else
#   include <unistd.h>
#   include <sys/types.h>
#   include <signal.h>
#endif

#ifndef INADDR_NONE
#   define INADDR_NONE (-1)
#endif

//-------------------------------------------------------Communication::Operation_connection::close

void Communication::Operation_connection::close()
{
    Z_FOR_EACH( Task_process_register, _task_process_register, it )
    {
        Process* process = it->second;
        process->close_async();
    }

    _task_process_register.clear();
}

//---------------------------------------Communication::Operation_connection::register_task_process

void Communication::Operation_connection::register_task_process( Process* process )
{
    assert( process->pid() );
    _task_process_register[ process->pid() ] = process;
}

//-------------------------------------Communication::Operation_connection::unregister_task_process

void Communication::Operation_connection::unregister_task_process( pid_t pid )
{
    assert( pid );

    Process* process = get_task_process( pid );
    process->close_async();

    _task_process_register.erase( pid );
}

//--------------------------------------------Communication::Operation_connection::get_task_process

Process* Communication::Operation_connection::get_task_process( pid_t pid )
{
    assert( pid );

    Task_process_register::iterator it = _task_process_register.find( pid );
    if( it == _task_process_register.end() )  z::throw_xc( __FUNCTION__, "unknown pid", pid );

    return it->second;
}

//-----------------------------------------------Xml_operation_connection::Xml_operation_connection

Xml_operation_connection::Xml_operation_connection( Communication::Connection* connection )
: 
    Communication::Operation_connection( connection ),
    _zero_(this+1)
{
    _indent = connection->_read_socket == STDIN_FILENO;
}

//--------------------------------------------------Xml_operation_connection::connection_lost_event

void Xml_operation_connection::connection_lost_event( const exception* x )
{
    if( _remote_scheduler )
    {
        _remote_scheduler->connection_lost_event( x );
    }
}

//---------------------------------------------------------------------Xml_operation::Xml_operation

Xml_operation::Xml_operation( Xml_operation_connection* operation_connection )
: 
    _zero_(this+1), 
    Operation( operation_connection ),
    _operation_connection( operation_connection )
{
}

//--------------------------------------------------------------------Xml_operation::~Xml_operation
    
Xml_operation::~Xml_operation()
{
    if( _response )  
    {
        _response->set_connection( NULL );
        _response->close();
    }

    if( _operation_connection )
    {
        _operation_connection->close();
    }
}

//-----------------------------------------------------------------------------Xml_operation::close

void Xml_operation::close()
{ 
    _operation_connection = NULL; 
    Communication::Operation::close(); 
}

//-----------------------------------------------------------------------Xml_operation::dom_element

xml::Element_ptr Xml_operation::dom_element( const xml::Document_ptr& doc, const Show_what& ) const 
{ 
    xml::Element_ptr result = doc.createElement( "xml_operation" ); 
    result.setAttribute( "async_state_text", async_state_text() );  // Debug
    return result;
}

//-----------------------------------------------------------------Xml_operation::async_state_text_
    
string Xml_operation::async_state_text_() const
{ 
    S result;
    
    result << Operation::async_state_text_();
    result << ", Xml_operation";

    if( _request != "" )  result << " " << quoted_string( truncate_to_one_line_with_ellipsis( _request, 100 ) );

    return result;
}

//------------------------------------------------------------------Xml_operation::put_request_part

void Xml_operation::put_request_part( const char* data, int length )
{
    _request_is_complete = _xml_end_finder.is_complete( data, length );

    if( length >= 2  &&  data[ length - 2 ] == '\r' )
    {
        _operation_connection->_indent = true;      // CR LF am Ende lässt Antwort einrücken. CR LF soll nur bei telnet-Eingabe kommen.
    }

    _request.append( data, length );
}

//-----------------------------------------------------------------------------Xml_operation::begin

void Xml_operation::begin()
{
    Command_processor command_processor ( _spooler, _connection->_security_level, this );
    command_processor.set_log( &_connection->_log );

    //command_processor.set_communication_operation( this );
    //command_processor.set_host( _connection->peer_host() );

    if( string_begins_with( _request, " " ) )  _request = ltrim( _request );

    //_connection->_log.info( message_string( "SCHEDULER-932", _request ) );

    _response = command_processor.response_execute( _request, _operation_connection->_indent );
    _response->set_connection( _connection );

    //if( _operation_connection->_indent )  _response->_response = replace_regex( response->_response, "\n", "\r\n" );      // Für Windows-telnet

    if( Synchronous_command_response* scr = dynamic_cast<Synchronous_command_response*>( +_response ) )
    {
        scr->write( io::Char_sequence( "\0", 1 ) );  // Null-Byte terminiert die XML-Antwort
    }
    else
    {
        //? Z_DEBUG_ONLY( int NULL_BYTE_ANHAENGEN; )
    }

    //set_log() hat das bereits ausgegeben:  if( command_processor._error )  _connection->_log.error( command_processor._error->what() );
}

//------------------------------------------------------------------------------Xml_response::close

void Xml_response::close()
{
    _xml_writer.close();        // Zirkel auflösen!
}

//--------------------------------------------------------------------Xml_response::signal_new_data

void Xml_response::signal_new_data()
{ 
    if( _connection )
    {
        if( _connection->state() == zschimmer::Buffered_socket_operation::s_ready )
        {
            _connection->_socket_event.signal( __FUNCTION__ ); 
        }
    }
}

//----------------------------------------------------Communication::Listen_socket::async_continue_

bool Communication::Listen_socket::async_continue_( Continue_flags )
{
    bool something_done = false;

    //if( socket_read_signaled() )
    {
        ptr<Connection> new_connection = Z_NEW( Connection( _communication ) );

        bool ok = new_connection->do_accept( _read_socket );
        if( ok )
        {
            if( _communication->_connection_list.size() >= max_communication_connections )
            {
                _spooler->log()->error( message_string( "SCHEDULER-286", max_communication_connections ) );
            }
            else
            {
                _communication->_connection_list.push_back( new_connection );

                new_connection->add_to_socket_manager( _spooler->_connection_manager );
                new_connection->socket_expect_signals( Socket_operation::sig_read | Socket_operation::sig_write | Socket_operation::sig_except );
            }

            something_done = true;
        }
    }

    async_clear_signaled();
    return something_done;
}

//-------------------------------------------------------Communication::Udp_socket::async_continue_

bool Communication::Udp_socket::async_continue_( Continue_flags )
{
    bool something_done = false;

    //if( socket_read_signaled() )
    {
        char           buffer [4096];
        sockaddr_in    addr;     
        sockaddrlen_t  addr_len = sizeof addr;

        addr.sin_addr = _spooler->_ip_address.as_in_addr();     // 0.0.0.0: Über alle Schnittstellen erreichbar

        int len = recvfrom( _read_socket, buffer, sizeof buffer, 0, (sockaddr*)&addr, &addr_len );
        if( len > 0 ) 
        {
            Host host = addr.sin_addr;
            if( _spooler->security_level( host ) < Security::seclev_signal )
            {
                _spooler->log()->error( message_string( "SCHEDULER-301", host.as_string() ) );   //"UDP-Nachricht von " + host.as_string() + " nicht zugelassen." );
            }
            else
            {
                Command_processor command_processor ( _spooler, _spooler->security_level( host ) );
                //command_processor.set_host( host );
                string cmd ( buffer, len );
                _spooler->log()->info( message_string( "SCHEDULER-955", host.as_string(), cmd ) );
                command_processor.execute( cmd );
            }
            
            something_done = true;
        }
    }

    async_clear_signaled();
    return something_done;
}

//------------------------------------------------------Communication::Operation::async_state_text_

string Communication::Operation::async_state_text_() const
{
    S result;
    result << "Operation on ";
    result << ( _operation_connection  &&  _operation_connection->_connection? _operation_connection->_connection->async_state_text()
                                                                             : result << "null" );
    return result;
}

//------------------------------------------------------------Communication::Connection::Connection

Communication::Connection::Connection( Communication* communication )
:
    _zero_(this+1),
    _spooler(communication->_spooler),
    _communication(communication),
    _log(_spooler)
{
    //_receive_at_start = true;
    //recv_clear();
}

//-----------------------------------------------------------Communication::Connection::~Connection

Communication::Connection::~Connection()
{
    remove_from_socket_manager();

    if( _read_socket != SOCKET_ERROR  &&  _read_socket != STDIN_FILENO )
    {
        Z_LOG2( "socket.close", "close(" << _read_socket << ")\n" );
        closesocket( _read_socket );
    }

    _read_socket = SOCKET_ERROR;
    _write_socket = SOCKET_ERROR;
}

//----------------------------------------------------Communication::Connection::connection_state_name

string Communication::Connection::connection_state_name( Connection_state state )
{
    switch( state )
    {
        case s_none:        return "none";
        case s_ready:       return "ready";
        case s_receiving:   return "receiving";
        case s_processing:  return "processing";
        case s_responding:  return "responding";
        case s_closing:     return "closing";
        default:            return S() << "Connection_state(" << (int)state << ")";
    }
}

//----------------------------------------------------------------Communication::Connection::remove_me

void Communication::Connection::remove_me( const exception* x )
{
    ptr<Connection> holder = this;

    remove_operation();
    if( _operation_connection )  _operation_connection->connection_lost_event( x );
    
    _spooler->_communication.remove_connection( this );     // Ruft do_close()
}

//----------------------------------------------------------------Communication::Connection::terminate
/*
void Communication::Connection::terminate()
{
    _dont_receive = true;
    async_continue( false );  // Wir warten nicht. Was nicht sofort gesendet werden kann, geht verloren (nur bei sehr vollem Puffer)
}
*/
//----------------------------------------------------------------Communication::Connection::do_accept

bool Communication::Connection::do_accept( SOCKET listen_socket )
{
    try
    {
        bool ok = this->accept( listen_socket );
        if( !ok )  return false;        // EWOULDBLOCK

        //_read_socket.set_linger( true, 0 );
        call_ioctl( FIONBIO, 1 );

        set_buffer_size();

        _security_level = _spooler->security_level( peer_host() );

        _log.set_prefix( "TCP connection to " + _peer_host_and_port.as_string() );


        if( _security_level <= Security::seclev_signal )
        {
            _log.warn( message_string( "SCHEDULER-287" ) );
            do_close();
            return false;
        }

        //set_event_name( S() << "TCP:" << _host.as_string() << ":" << ntohs( _peer_addr.sin_port ) );

        _log.info( message_string( "SCHEDULER-933" ) );
        _connection_state = s_ready;

    }
    catch( const exception& x ) { _log.error(x.what()); return false; }

    return true;
}

//--------------------------------------------------------------Communication::Connection::do_close

void Communication::Connection::do_close()
{
    if( _operation_connection )  _operation_connection->close();

    remove_operation();
    remove_from_socket_manager();

    if( _read_socket == STDIN_FILENO  &&  _eof  &&  isatty(STDIN_FILENO) )  _spooler->cmd_terminate();  // Ctrl-D (auch wenn Terminal-Sitzung beendet?)

    if( _read_socket != SOCKET_ERROR  &&  _read_socket != STDIN_FILENO )
    {
        Z_LOG2( "socket.shutdown", "shutdown(" << _read_socket << ",SHUT_WR)\n" );
        shutdown( _read_socket, SHUT_WR );

        Z_LOG2( "socket.close", "close(" << _read_socket << ")\n" );
        closesocket( _read_socket );
    }
    
    _read_socket  = SOCKET_ERROR;
    _write_socket = SOCKET_ERROR;
}

//------------------------------------------------------Communication::Connection::remove_operation

void Communication::Connection::remove_operation()
{
    if( _operation )
    {
        _operation->set_async_manager( NULL );
        _operation->close();
        _operation = NULL;
    }
}

//---------------------------------------------------------------Communication::Connection::do_recv

bool Communication::Connection::do_recv()
{
    bool something_done = false;

    char buffer [ 4096 ];

    int len = call_recv( buffer, sizeof buffer );
    if( len == 0 )  
    {
        something_done = _eof;
    }
    else
    {
        something_done = true;

        const char* p = buffer;

        if( _connection_state == s_ready )   // Das sind die ersten empfangen Bytes der Anforderung
        {
            if( len == 1  &&  buffer[0] == '\x04' )  { _eof = true; return true; }      // Einzelnes Ctrl-D beendet Sitzung

            if( len == 1  &&  buffer[0] == '\n'   
             || len == 2  &&  buffer[0] == '\r'  &&  buffer[1] == '\n' )  { _spooler->signal( "do_something!" );  _spooler->_last_time_enter_pressed = Time::now().as_time_t(); return true; }

            //_receive_at_start = false;
            _connection_state = s_receiving;

            if( !_operation_connection )
            {
                //if( buffer[ 0 ] == '\0' )   // Das erste von vier Längenbytes ist bei der ersten Nachricht auf jeden Fall 0
                //{
                //    _operation_connection = Z_NEW( Object_server_processor_connection( this ) );
                //}
                //else
                //if( string_begins_with( buffer, "GET /" )  ||  string_begins_with( buffer, "POST /" ) )     // Muss vollständig im ersten Datenblock enthalten sein!
                if( isupper( (unsigned char)buffer[ 0 ] ) )  // CONNECT, DELETE, GET, HEAD, OPTIONS, POST, PUT, TRACE 
                {
                    _operation_connection = Z_NEW( http::Operation_connection( this ) );
                }
                else
                {
                    _operation_connection = Z_NEW( Xml_operation_connection( this ) );
                }
            }

            _operation = _operation_connection->new_operation();
            _operation->set_async_manager( _spooler->_connection_manager );     // Für set_async_next_gmtime()
        }

        //if( _read_socket != STDIN_FILENO )  _operation->set_host( &_peer_host_and_port._host );

        if( len > 0 )
        {
            if( z::Log_ptr log = "socket.data" )  *log << "recv: ", log->write( p, len ), *log << endl;
            _operation->put_request_part( p, len );
        }
    }

    return something_done;
}

//-------------------------------------------------------Communication::Connection::async_continue_

bool Communication::Connection::async_continue_( Continue_flags )
{
    //Z_LOG2( "joacim", __FUNCTION__ << state() << "\n" );

    bool something_done = false;

    try
    {
        if( _connection_state == s_responding )  assert_no_recv_data();  //2005-10-25 wegen endloser read-Signalisierung zwischen Firefox und Linux-Scheduler (F5 im Protokollfenster)

        while( _connection_state == s_ready  ||  _connection_state == s_receiving )
        {
            bool ok = do_recv();
            if( !ok )  break;

            something_done = true;

            if( _eof )  break;

            if( _operation  &&  _operation->request_is_complete() ) 
            {
                _connection_state = s_processing;
                _operation->begin();
            }
        }

        if( _connection_state == s_processing )
        {
            //check_for_eof();  Wird nicht durchlaufen, weil kein Ereignis bestellt ist. Lösung: Empfangs- und Sendekanäle trennen (zwei Async_operation)
            if( _operation->async_finished() )  _connection_state = s_responding;
        }

        if( _connection_state == s_responding )
        {
            if( state() == s_sending )  something_done |= send__continue();

            while( state() == Buffered_socket_operation::s_ready )
            {
                string data = _operation->get_response_part();
                if( data == "" )  break;

                Z_LOG2( "socket.data", "send: " << data << '\n' );

                something_done = true;

                send__start( data );
            }
                

            if( state() == Buffered_socket_operation::s_ready  &&  _operation->response_is_complete() )
            {
                if( _operation->should_close_connection() )  _eof = true;   // Wir tun so, als ob der Client EOF geliefert hat. Das führt zum Schließen der Verbindung.

                remove_operation();
                _connection_state = s_ready;
            }
        }


        if( _eof )
        { 
            // Bei _eof sofort handeln! Nämlich Verbindung schließen. 
            // Sonst liefert select() immer wieder den Socket, was zur Schleife führt.
            // Andere Lösung: Socket aus read-Menge des select() herausnehmen.

            // _http_response kann bei einem Log endlos sein. Also kein Kriterium, das Schließen zu verzögern.
            //if( _http_response )  Z_LOG2( "scheduler.http", "Browser schließt Verbindung bevor HTTP-Response fertig gesendet werden konnte\n" );
            remove_me();
            return true; 
        }
    }
    catch( const exception& x ) 
    { 
        if( string_begins_with( x.what(), "ERRNO-103 "     ) == 0                                  // ECONNABORTED, Firefox trennt sich so
         || string_begins_with( x.what(), "WINSOCK-10053 " ) == 0                                  // ECONNABORTED, Firefox trennt sich so
         || string_begins_with( x.what(), "ERRNO-104 "     ) == 0                                  // ECONNRESET, Internet Explorer trennt sich so
         || string_begins_with( x.what(), "WINSOCK-10054 " ) == 0 )  _log.debug( x.what() );       // ECONNRESET, Internet Explorer trennt sich so
                                                               else  _log.error( x.what() );  

        remove_me( &x );
        return true; 
    }

    async_clear_signaled();
    return something_done;
}

//-----------------------------------------------------------Communication::Connection::dom_element

xml::Element_ptr Communication::Connection::dom_element( const xml::Document_ptr& document, const Show_what& what ) const
{
    xml::Element_ptr result = document.createElement( "connection" );

    result.setAttribute( "state", connection_state_name() + "/" + state_name() );

    result.setAttribute( "received_bytes", recv_total_byte_count() );
    result.setAttribute( "sent_bytes"    , send_total_byte_count() );

    xml::Element_ptr peer_element = result.append_new_element( "peer" );
    peer_element.setAttribute( "host_ip", peer().host().ip_string() );
    peer_element.setAttribute( "port"   , peer().port() );

    if( peer().host().name() != "" )
    peer_element.setAttribute( "host_name", peer().host().name() );

    if( _operation_connection )
    {
        result.setAttribute( "operation_type", _operation_connection->connection_type() );

        if( !_operation_connection->_task_process_register.empty() )
        {
            xml::Element_ptr processes_element = result.append_new_element( "remote_processes" );
            Z_FOR_EACH( Operation_connection::Task_process_register, _operation_connection->_task_process_register, it )
            {
                xml::Element_ptr process_element = processes_element.append_new_element( "remote_process" );
                process_element.setAttribute( "pid", it->second->pid()) ;
            }
        }
    }

    if( _operation )
    {
        //result.setAttribute( "operation", _operation->obj_name() );
        xml::Element_ptr operation_element = result.append_new_element( "operation" );
        
        double gmtimeout_at = _operation->gmtimeout();
        if( gmtimeout_at < double_time_max )  operation_element.setAttribute( "timeout_at", Time( localtime_from_gmtime( gmtimeout_at ) ).as_string() );

        operation_element.appendChild( _operation->dom_element( document, what ) );
    }

    return result;
}

//---------------------------------------------------------------------Communication::Communication

Communication::Communication( Spooler* spooler )
: 
    _zero_(this+1), 
    _spooler(spooler),
    _listen_socket(this),
    _udp_socket(this)
{
}

//--------------------------------------------------------------------Communication::~Communication

Communication::~Communication()
{
    close( 0.0 );

    if( _initialized ) 
    {
#       ifdef SYSTEM_WIN
            WSACleanup();
#       endif
    }
}

//-----------------------------------------------------------------------------Communication::close

void Communication::close( double wait_time )
{
    // Funktioniert nicht: double until = Time::now() + wait_time;

    _listen_socket.set_linger( true, 0 );
    _listen_socket.close();

    _udp_socket.set_linger( true, 0 );
    _udp_socket.close();

    bool connection_was_open = false;

    //while(1)
    {
        //bool responding = false;

        while( !_connection_list.empty() )
        {
            Connection_list::iterator c = _connection_list.begin();

            ptr<Connection> connection = *c;
            c++;
            
            connection->set_linger( true, 1 );     // 1: Eine Sekunde Zeit, um asynchron (non-blocking) die Verbindung abzubauen. sleep(2) in spooler.cxx!
                                                // 0: close() bricht Verbindung ab (jedenfalls unter Unix). Schickt RST. 
                                                // Damit bleibt beim Neustart Windows-Schedulers der Browser nicht kleben (der kriegt den Verbindungsabbau nicht mit)

            connection->remove_me();
            assert( _connection_list.empty()  ||  *_connection_list.begin() != connection );  // connection muss jetzt gelöscht sein

            connection_was_open = true;
            //connection->_connection_state = Connection::s_closing;
            //connection->async_continue();
            //if( connection->_responding )
            //{
            //    responding = true;
            //    Z_LOG2( "scheduler", "Warten auf " << *connection << '\n' );
            //}
        }

        //if( !responding )  break;
        //if( Time::now() > until )  break;

        //sleep( 0.1 );
    }


    //c = _connection_list.begin();
    //while( c != _connection_list.end() )  remove_connection( *c );

    _connection_list.clear();

    if( connection_was_open ) 
    {
        Z_LOG2( "scheduler", __FUNCTION__ << "  sleep(" << wait_time << ")\n" );
        sleep( wait_time );
    }

    _terminate = true;
}

//------------------------------------------------------------------Communication::finish_responses

void Communication::finish_responses( double wait_time )
{
    Connection_list copy = _connection_list;        // Nicht über Original-Liste laufen, die kann verändert werden

    FOR_EACH( Connection_list, copy, it )
    {
        ptr<Connection> connection = *it;
        connection->async_continue();
        //...?
    }
}

//-----------------------------------------------------------------Communication::remove_connection

void Communication::remove_connection( Connection* connection )
{
    FOR_EACH( Connection_list, _connection_list, it )
    {
        if( *it == connection )  
        {
            ptr<Connection> c = *it;
            _connection_list.erase( it ); 
            c->do_close(); 
            break;
        }
    }
}

//----------------------------------------------------------------Communication::main_thread_exists
/*
bool Communication::main_thread_exists()
{
#   ifndef Z_LINUX

        return true;

#    else

        if( kill( _spooler->_pid, 0 ) == -1  &&  errno == ESRCH )
        {
            //_spooler->log()->error( "Kommunikations-Thread wird beendet, weil der Hauptthread (pid=" + as_string(_spooler->_pid) + ") verschwunden ist" );
            //_spooler->_log verklemmt sich manchmal nach Ausgabe des Zeitstemples (vor "[ERROR]"). Vielleicht wegen einer Semaphore?

            //fprintf( stderr, "Kommunikations-Thread wird beendet, weil der Hauptthread (pid=%d) verschwunden ist\n", _spooler->_pid );
            return false;  //?  Thread bleibt sonst hängen, wenn Java sich bei Ctrl-C sofort verabschiedet. Java lässt SIGINT zu, dieser Thread aber nicht.
        }

        return true;

#   endif
}
*/
//-----------------------------------------------------------------------Communication::bind_socket

int Communication::bind_socket( SOCKET socket, struct sockaddr_in* sa, const string& tcp_or_udp )
{
    int ret;
    int true_ = 1;

    string port_name = tcp_or_udp + "-port " + as_string( ntohs( sa->sin_port ) );

    if( _spooler->_reuse_port )     // War für Suse 8 nötig. Und für Windows XP, wenn Scheduler zuvor abgestürzt ist (mit Debugger), denn dann bleibt der Port ewig blockiert
    {
        _spooler->log()->warn( message_string( "SCHEDULER-288", port_name ) );
        Z_LOG2( "scheduler", "setsockopt(" << socket << ",SOL_SOCKET,SO_REUSEADDR,1)  " );
        ret = setsockopt( socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&true_, sizeof true_ );
        Z_LOG2( "scheduler", "ret=" << ret );  if( ret == SOCKET_ERROR )  Z_LOG2( "scheduler", "errno=" << errno << "  "  << z_strerror(errno) );
        Z_LOG2( "scheduler", "\n" );
    }

    bool print_dots = isatty( fileno(stderr) ) && isatty( fileno(stdin) );

    ret = ::bind( socket, (struct sockaddr*)sa, sizeof (struct sockaddr_in) );

    if( ret == SOCKET_ERROR  &&  socket_errno() == EADDRINUSE )
    {
        int my_errno = errno;  // Nur für Unix

        _spooler->log()->warn( message_string( "SCHEDULER-289", port_name, wait_for_port_available ) );

        for( int i = 1; i <= wait_for_port_available; i++ )
        {
            if( ctrl_c_pressed || _spooler->state() == Spooler::s_stopping || _spooler->state() == Spooler::s_stopped )  { errno = EINTR; return SOCKET_ERROR; }
            //if( !main_thread_exists() )  return EINTR;  //?  Thread bleibt sonst hängen, wenn Java sich bei Ctrl-C sofort verabschiedet. Java lässt SIGINT zu, dieser Thread aber nicht.

            sos_sleep(1);
        
            ret = ::bind( socket, (struct sockaddr*)sa, sizeof (struct sockaddr_in) );
            my_errno = errno;
            if( ret != SOCKET_ERROR ) break;
            if( socket_errno() != EADDRINUSE )  break;
            if( ctrl_c_pressed )  break;

            if( print_dots )  fputc( i % 10 == 0? '0' + i / 10 % 10 : '.', stderr );
        }

        if( print_dots )  fputc( '\n', stderr );

        errno = my_errno;   // Nur für Unix
    }

    return ret;
}

//------------------------------------------------------------------------------Communication::bind

void Communication::bind()
{
    struct sockaddr_in  sa;
    int                 ret;
    unsigned long       on = 1;


    // UDP:

    if( _udp_port != _spooler->udp_port() )
    {
        //THREAD_LOCK( _semaphore )
        {
            _udp_socket.close();
            _udp_port = 0;

            if( _spooler->udp_port() != 0 )
            {
                _udp_socket._read_socket = socket( AF_INET, SOCK_DGRAM, 0 );
                if( _udp_socket._read_socket == SOCKET_ERROR )  throw_socket( socket_errno(), "socket" );

                set_socket_not_inheritable( _udp_socket._read_socket );
                //_udp_socket.set_linger( false );
                
                sa.sin_port   = htons( _spooler->udp_port() );
                sa.sin_family = AF_INET;
                sa.sin_addr   = _spooler->_ip_address.as_in_addr();     // 0.0.0.0: Über alle Schnittstellen erreichbar

                ret = bind_socket( _udp_socket._read_socket, &sa, "UDP" );
                if( ret == SOCKET_ERROR )  throw_socket( socket_errno(), "udp-bind ", as_string(_spooler->udp_port()).c_str() );

                ret = ioctlsocket( _udp_socket._read_socket, FIONBIO, &on );
                if( ret == SOCKET_ERROR )  throw_socket( socket_errno(), "ioctl(FIONBIO)" );

                _udp_port = _spooler->udp_port();
                _rebound = true;

                _udp_socket.set_event_name( S() << "UDP:" << ntohs( sa.sin_port ) );

                {
                    Message_string m ( "SCHEDULER-956", "UDP", _udp_port );     // "Scheduler erwartet Kommandos über $1-Port " 
                    if( sa.sin_addr.s_addr )  m.insert_string( 2, Ip_address( sa.sin_addr ).as_string() );    
                    _spooler->log()->info( m );
                }
            }
        }

        _udp_socket.add_to_socket_manager( _spooler->_connection_manager );
        _udp_socket.socket_expect_signals( Socket_operation::sig_read );
    }


    // TCP: 

    if( _tcp_port != _spooler->tcp_port() )
    {
        //Z_MUTEX( _semaphore )
        {
            _listen_socket.close();
            _tcp_port = 0;

            if( _spooler->tcp_port() != 0 )
            {
                _listen_socket.set_event_name( S() << "TCP listen(" << _spooler->tcp_port() << ")" );

                _listen_socket._read_socket = socket( AF_INET, SOCK_STREAM, 0 );
                if( _listen_socket._read_socket == SOCKET_ERROR )  throw_socket( socket_errno(), "socket" );

                set_socket_not_inheritable( _listen_socket._read_socket );
                //_listen_socket.set_linger( false );
                
                sa.sin_port   = htons( _spooler->tcp_port() );
                sa.sin_family = AF_INET;
                sa.sin_addr   = _spooler->_ip_address.as_in_addr();     // 0.0.0.0: Über alle Schnittstellen erreichbar

                ret = bind_socket( _listen_socket._read_socket, &sa, "TCP" );
                if( ret == SOCKET_ERROR )  throw_socket( socket_errno(), "tcp-bind", as_string(_spooler->tcp_port()).c_str() );

                Z_LOG2( "socket.listen", "listen()\n" );
                ret = listen( _listen_socket._read_socket, 5 );
                if( ret == SOCKET_ERROR )  throw_socket( socket_errno(), "listen" );

                ret = ioctlsocket( _listen_socket._read_socket, FIONBIO, &on );
                if( ret == SOCKET_ERROR )  throw_socket( socket_errno(), "ioctl(FIONBIO)" );

                _tcp_port = _spooler->tcp_port();
                _rebound = true;

                {
                    Message_string m ( "SCHEDULER-956", "TCP", _tcp_port );     // "Scheduler erwartet Kommandos über $1-Port " 
                    if( sa.sin_addr.s_addr )  m.insert_string( 2, Ip_address( sa.sin_addr ).as_string() );    
                    _spooler->log()->info( m );
                }
            }
        }

        _listen_socket.add_to_socket_manager( _spooler->_connection_manager );
        _listen_socket.socket_expect_signals( Socket_operation::sig_read );
    }


#   ifndef Z_WINDOWS
        if( _spooler->_interactive )
        {
            ptr<Connection> new_connection = Z_NEW( Connection( this ) );
    
            new_connection->_read_socket  = STDIN_FILENO;
            new_connection->_write_socket = STDOUT_FILENO;
            new_connection->_socket_send_buffer_size = 1024;

            new_connection->call_ioctl( FIONBIO, on );   // In Windows nicht möglich
            new_connection->add_to_socket_manager( _spooler->_connection_manager );
            new_connection->socket_expect_signals( Socket_operation::sig_read );

            new_connection->set_event_name( "stdin" );
            new_connection->_connection_state = Connection::s_ready;
            new_connection->_security_level = Security::seclev_all;

            _connection_list.push_back( new_connection );
        }
#   endif
}

//-----------------------------------------------------------------------------Communication::init

void Communication::init()
{
    if( _initialized )  return;

#   ifdef SYSTEM_WIN
        WSADATA wsa_data;
        int ret = WSAStartup( 0x0101, &wsa_data );
        if( ret )  throw_socket( ret, "WSAStartup" );
#   endif

    _initialized = true;
}

//-------------------------------------------------------------------Communication::start_or_rebind

void Communication::start_or_rebind()
{
    if( !_started )  
    {
        init();
        bind();
    }
    else
    {
        rebind();
    }
}

//-----------------------------------------------------------------------Communication::dom_element

xml::Element_ptr Communication::dom_element( const xml::Document_ptr& document, const Show_what& what ) const
{
    xml::Element_ptr connections_element = document.createElement( "connections" );

    Z_FOR_EACH_CONST( Connection_list, _connection_list, c )
    {
        const Connection* connection = *c;

        connections_element.appendChild( connection->dom_element( document, what ) );
    }

    return connections_element;
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

