// $Id$
/*
    Hier sind implementiert

    Communication::Channer
    Communication
*/


#include "spooler.h"
#include "../kram/sleep.h"
#include "../kram/sos_java.h"

using namespace std;

namespace sos {
namespace spooler {

const int wait_for_port_available = 60;   // Soviele Sekunden warten, bis TCP- oder UDP-Port frei wird

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

//-----------------------------------------------------Xml_processor_channel::Xml_processor_channel

Xml_processor_channel::Xml_processor_channel( Communication::Channel* channel )
: 
    Communication::Processor_channel( channel ) 
{
    _indent = channel->_read_socket == STDIN_FILENO;
}

//-----------------------------------------------------Xml_processor_channel::connection_lost_event

void Xml_processor_channel::connection_lost_event( const exception* x )
{
    if( _remote_scheduler )
    {
        _remote_scheduler->connection_lost_event( x );
    }
}

//---------------------------------------------------------------------Xml_processor::Xml_processor

Xml_processor::Xml_processor( Xml_processor_channel* processor_class )
: 
    _zero_(this+1), 
    Processor( processor_class ),
    _processor_channel( processor_class )
{
}

//------------------------------------------------------------------Xml_processor::put_request_part

void Xml_processor::put_request_part( const char* data, int length )
{
    _request_is_complete = _xml_end_finder.is_complete( data, length );

    if( length >= 2  &&  data[ length - 2 ] == '\r' )
    {
        _processor_channel->_indent = true;      // CR LF am Ende lässt Antwort einrücken. CR LF soll nur bei telnet-Eingabe kommen.
    }

    _request.append( data, length );
}

//---------------------------------------------------------------------------Xml_processor::process

void Xml_processor::process()
{
    Command_processor command_processor ( _spooler );

    command_processor.set_communication_processor( this );
    command_processor.set_host( _host );

    if( string_begins_with( _request, " " ) )  _request = ltrim( _request );

    _channel->_log.info( "Kommando " + _request );
    _response = command_processor.execute( _request, Time::now(), _processor_channel->_indent );

    if( _processor_channel->_indent )  _response = replace_regex( _response, "\n", "\r\n" );      // Für Windows-telnet

    _response += '\0';  // Null-Byte terminiert die XML-Antwort

    if( command_processor._error )  _channel->_log.error( command_processor._error->what() );
}

//----------------------------------------------------Communication::Listen_socket::async_continue_

bool Communication::Listen_socket::async_continue_( Continue_flags flags )
{
    bool something_done = false;

    //if( socket_read_signaled() )
    {
        ptr<Channel> new_channel = Z_NEW( Channel( _communication ) );

        bool ok = new_channel->do_accept( _read_socket );
        if( ok )
        {
            if( _communication->_channel_list.size() >= max_communication_channels )
            {
                _spooler->_log.error( S() << "Mehr als " << max_communication_channels << " Kommunikationskanäle. Verbindung abgelehnt.\n" );
            }
            else
            {
                _communication->_channel_list.push_back( new_channel );

                new_channel->add_to_socket_manager( _spooler->_connection_manager );
                new_channel->socket_expect_signals( Socket_operation::sig_read | Socket_operation::sig_write | Socket_operation::sig_except );
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

        addr.sin_addr.s_addr = 0;
        int len = recvfrom( _read_socket, buffer, sizeof buffer, 0, (sockaddr*)&addr, &addr_len );
        if( len > 0 ) 
        {
            Host host = addr.sin_addr;
            if( _spooler->security_level( host ) < Security::seclev_signal )
            {
                _spooler->log()->error( "UDP-Nachricht von " + host.as_string() + " nicht zugelassen." );
            }
            else
            {
                Command_processor command_processor ( _spooler );
                command_processor.set_host( &host );
                string cmd ( buffer, len );
                _spooler->log()->info( "UDP-Nachricht von " + host.as_string() + ": " + cmd );
                command_processor.execute( cmd, Time::now() );
            }
            
            something_done = true;
        }

    }

    async_clear_signaled();
    return something_done;
}

//------------------------------------------------------------------Communication::Channel::Channel

Communication::Channel::Channel( Communication* communication )
:
    _zero_(this+1),
    _spooler(communication->_spooler),
    _communication(communication),
    _log(_spooler)
{
    _receive_at_start = true;
    //recv_clear();
}

//-----------------------------------------------------------------Communication::Channel::~Channel

Communication::Channel::~Channel()
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

//----------------------------------------------------------------Communication::Channel::remove_me

void Communication::Channel::remove_me( const exception* x )
{
    if( _processor_channel )  _processor_channel->connection_lost_event( x );

    _communication->remove_channel( this );
}

//----------------------------------------------------------------Communication::Channel::terminate
/*
void Communication::Channel::terminate()
{
    _dont_receive = true;
    async_continue( false );  // Wir warten nicht. Was nicht sofort gesendet werden kann, geht verloren (nur bei sehr vollem Puffer)
}
*/
//----------------------------------------------------------------Communication::Channel::do_accept

bool Communication::Channel::do_accept( SOCKET listen_socket )
{
    try
    {
        bool ok = this->accept( listen_socket );
        if( !ok )  return false;        // EWOULDBLOCK

        //_read_socket.set_linger( true, 0 );
        call_ioctl( FIONBIO, 1 );

        set_buffer_size();

      //_host = _peer_addr.sin_addr;
        _log.set_prefix( "TCP-Verbindung mit " + _peer_host_and_port._host.as_string() );


        if( _spooler->security_level( _peer_host_and_port._host ) <= Security::seclev_signal )
        {
            _log.warn( "TCP-Verbindung nicht zugelassen" );
            do_close();
            return false;
        }

        //set_event_name( S() << "TCP:" << _host.as_string() << ":" << ntohs( _peer_addr.sin_port ) );

        _log.info( "TCP-Verbindung angenommen" );

    }
    catch( const exception& x ) { _log.error(x.what()); return false; }

    return true;
}

//-----------------------------------------------------------------Communication::Channel::do_close

void Communication::Channel::do_close()
{
    remove_from_socket_manager();

    if( _read_socket == STDIN_FILENO  &&  _eof  &&  isatty(STDIN_FILENO) )  _spooler->cmd_terminate();  // Ctrl-D (auch wenn Terminal-Sitzung beendet?)

    if( _read_socket != SOCKET_ERROR  &&  _read_socket != STDIN_FILENO )
    {
        Z_LOG2( "socket.close", "close(" << _read_socket << ")\n" );
        closesocket( _read_socket );
    }
    
    _read_socket  = SOCKET_ERROR;
    _write_socket = SOCKET_ERROR;
}

//------------------------------------------------------------------Communication::Channel::do_recv

bool Communication::Channel::do_recv()
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

        if( _receive_at_start ) 
        {
            if( len == 1  &&  buffer[0] == '\x04' )  { _eof = true; return true; }      // Einzelnes Ctrl-D beendet Sitzung

            if( len == 1  &&  buffer[0] == '\n'   
             || len == 2  &&  buffer[0] == '\r'  &&  buffer[1] == '\n' )  { _spooler->signal( "do_something!" );  _spooler->_last_time_enter_pressed = Time::now().as_time_t(); return true; }

            _receive_at_start = false;

            if( !_processor_channel )
            {
                //if( buffer[ 0 ] == '\0' )   // Das erste von vier Längenbytes ist bei der ersten Nachricht auf jeden Fall 0
                //{
                //    _processor_channel = Z_NEW( Object_server_processor_channel( this ) );
                //}
                //else
                if( string_begins_with( buffer, "GET /" )  ||  string_begins_with( buffer, "POST /" ) )     // Muss vollständig im ersten Datenblock enthalten sein!
                {
                    _processor_channel = Z_NEW( Http_processor_channel( this ) );
                }
                else
                {
                    _processor_channel = Z_NEW( Xml_processor_channel( this ) );
                }
            }

            _processor = _processor_channel->processor();
        }

        if( _read_socket != STDIN_FILENO )  _processor->set_host( &_peer_host_and_port._host );

        if( len > 0 )
        {
            _processor->put_request_part( p, len );
        }
    }

    return something_done;
}

//------------------------------------------------------------------Communication::Channel::do_send
/*
bool Communication::Channel::do_send()
{
    bool something_done = false;

    while(1)
    {
        int count = _send_data.length() - _send_progress; 
        if( count <= 0 )  break;

        int c   = min( _socket_send_buffer_size, count );
        int err = 0;

      //do
      //{
            int len = _write_socket == STDOUT_FILENO? write ( _write_socket, _send_data.data() + _send_progress, c )
                                                    : ::send( _write_socket, _send_data.data() + _send_progress, c, 0 );
            err = len < 0? socket_errno() : 0;
            Z_LOG2( "socket.send", "send/write(" << _write_socket << "," << c << " bytes) ==> " << len << "  errno=" << err << "\n" );
            if( len == 0 )  break;   // Vorsichtshalber
            if( len < 0 ) 
            {
                if( err == EWOULDBLOCK )  break;
              //{
              //    _socket_manager->set_fd( Socket_manager::write_fd, _write_socket );
              //    return false;
              //}
              //else
              //if( err == ENOBUFS  &&  c > 1000 )      // Windows XP meldet bei c==34MB ENOBUFS. 
              //{
              //    c /= 2;
              //}
                else
                    throw_sos_socket_error( err, "send" );
            }
            else 
                _send_progress += len;
      //}
      //while( err == ENOBUFS );

        something_done = true;
    }


    if( _send_progress < _send_data.length() )
    {
        _socket_manager->set_fd( Socket_manager::write_fd, _write_socket );
    }
    else
    {
        _send_data = "";
        _send_progress = 0;
        _socket_manager->clear_fd( Socket_manager::write_fd, _write_socket );
    }

    return something_done;
}
*/
//----------------------------------------------------------Communication::Channel::async_continue_

bool Communication::Channel::async_continue_( Continue_flags )
{
    bool something_done = false;

    try
    {
        while( !_responding  &&  !_dont_receive )
        {
            bool ok = do_recv();
            if( !ok )  break;

            something_done = true;

            if( _eof )  break;

            if( _processor  &&  _processor->request_is_complete() ) 
            {
                _processor->process();
                _responding = true;
            }
        }


        if( _responding )
        {
            if( state() == s_sending )  something_done |= send__continue();

            while( state() == s_stand_by )
            {
                string data = _processor->get_response_part();
                if( data == "" )  break;

                something_done = true;

                send__start( data );
            }
                

            if( state() == s_stand_by  &&  _processor->response_is_complete() )
            {
                if( _processor->should_close_connection() )  _eof = true;   // Wir tun so, als ob der Client EOF geliefert hat. Das führt zum Schließen der Verbindung.

                _processor        = NULL;
                _responding       = false;
                _receive_at_start = true; 
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

//--------------------------------------------------------------------Communication::Communication

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


    //while(1)
    {
        //bool responding = false;

        Channel_list::iterator c = _channel_list.begin();
        while( c != _channel_list.end() )
        {
            ptr<Channel> channel = *c;
            c++;
            
            channel->set_linger( true, 1 );     // 1: Eine Sekunde Zeit, um asynchron (non-blocking) die Verbindung abzubauen. sleep(2) in spooler.cxx!
                                                // 0: close() bricht Verbindung ab (jedenfalls unter Unix). Schickt RST. D
                                                // Damit bleibt beim Neustart Windows-Schedulers der Browser nicht kleben (der kriegt den Verbindungsabbau nicht mit)

            //channel->terminate();    // Kann Channel aus _channel_list entfernen.
            channel->_dont_receive = true;
            channel->async_continue();
            //if( channel->_responding )
            //{
            //    responding = true;
            //    Z_LOG( "Warten auf " << *channel << '\n' );
            //}
        }

        //if( !responding )  break;
        //if( Time::now() > until )  break;

        //sleep( 0.1 );
    }


    //c = _channel_list.begin();
    //while( c != _channel_list.end() )  remove_channel( *c );

    _channel_list.clear();

    _terminate = true;
}

//--------------------------------------------------------------------Communication::remove_channel

void Communication::remove_channel( Channel* channel )
{
    FOR_EACH( Channel_list, _channel_list, it )
    {
        if( *it == channel )  
        {
            ptr<Channel> c = *it;
            _channel_list.erase( it ); 
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
            //_spooler->_log.error( "Kommunikations-Thread wird beendet, weil der Hauptthread (pid=" + as_string(_spooler->_pid) + ") verschwunden ist" );
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

    string port_name = tcp_or_udp + "-Port " + as_string( ntohs( sa->sin_port ) );

    if( _spooler->_reuse_port )     // War für Suse 8 nötig. Und für Windows XP, wenn Scheduler zuvor abgestürzt ist (mit Debugger), denn dann bleibt der Port ewig blockiert
    {
        _spooler->_log.warn( S() << "-reuse-port: " << port_name << " wird verwendet, auch wenn er schon von einer anderen Anwendung belegt ist\n" );
        LOG( "setsockopt(" << socket << ",SOL_SOCKET,SO_REUSEADDR,1)  " );
        ret = setsockopt( socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&true_, sizeof true_ );
        LOG( "ret=" << ret );  if( ret == SOCKET_ERROR )  LOG( "errno=" << errno << "  "  << strerror(errno) );
        LOG( "\n" );
    }

    bool print_dots = isatty( fileno(stderr) ) && isatty( fileno(stdin) );

    ret = ::bind( socket, (struct sockaddr*)sa, sizeof (struct sockaddr_in) );

    if( ret == SOCKET_ERROR  &&  socket_errno() == EADDRINUSE )
    {
        int my_errno = errno;  // Nur für Unix

        _spooler->_log.warn( port_name + " ist blockiert. Wir probieren es " + as_string(wait_for_port_available) + " Sekunden" );

        for( int i = 1; i <= wait_for_port_available; i++ )
        {
            if( ctrl_c_pressed || _spooler->state() == Spooler::s_stopping || _spooler->state() == Spooler::s_stopped )  { errno = EINTR; return SOCKET_ERROR; }
            //if( !main_thread_exists() )  return EINTR;  //?  Thread bleibt sonst hängen, wenn Java sich bei Ctrl-C sofort verabschiedet. Java lässt SIGINT zu, dieser Thread aber nicht.

            sos_sleep(1);
        
            ret = ::bind( socket, (struct sockaddr*)sa, sizeof (struct sockaddr_in) );
            my_errno = errno;
            if( ret != SOCKET_ERROR ) break;
            if( socket_errno() != EADDRINUSE )  break;

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

                //_udp_socket.set_linger( false );
                
                sa.sin_port        = htons( _spooler->udp_port() );
                sa.sin_family      = AF_INET;
                sa.sin_addr.s_addr = 0; // INADDR_ANY

                ret = bind_socket( _udp_socket._read_socket, &sa, "UDP" );
                if( ret == SOCKET_ERROR )  throw_socket( socket_errno(), "udp-bind ", as_string(_spooler->udp_port()).c_str() );

                ret = ioctlsocket( _udp_socket._read_socket, FIONBIO, &on );
                if( ret == SOCKET_ERROR )  throw_socket( socket_errno(), "ioctl(FIONBIO)" );

                _udp_port = _spooler->udp_port();
                _rebound = true;

                _udp_socket.set_event_name( S() << "UDP:" << ntohs( sa.sin_port ) );

                _spooler->log()->info( "Scheduler erwartet Kommandos über UDP-Port " + sos::as_string(_udp_port) );
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

                //_listen_socket.set_linger( false );
                
                sa.sin_port        = htons( _spooler->tcp_port() );
                sa.sin_family      = AF_INET;
                sa.sin_addr.s_addr = 0; // INADDR_ANY

                ret = bind_socket( _listen_socket._read_socket, &sa, "TCP" );
                if( ret == SOCKET_ERROR )  throw_socket( socket_errno(), "tcp-bind", as_string(_spooler->tcp_port()).c_str() );

                Z_LOG2( "socket.listen", "listen()\n" );
                ret = listen( _listen_socket._read_socket, 5 );
                if( ret == SOCKET_ERROR )  throw_socket( socket_errno(), "listen" );

                ret = ioctlsocket( _listen_socket._read_socket, FIONBIO, &on );
                if( ret == SOCKET_ERROR )  throw_socket( socket_errno(), "ioctl(FIONBIO)" );

                _tcp_port = _spooler->tcp_port();
                _rebound = true;

                _spooler->log()->info( "Scheduler erwartet Kommandos über TCP-Port " + sos::as_string(_tcp_port) );
            }
        }

        _listen_socket.add_to_socket_manager( _spooler->_connection_manager );
        _listen_socket.socket_expect_signals( Socket_operation::sig_read );
    }


#   ifndef Z_WINDOWS
        if( _spooler->_interactive )
        {
            ret = ioctl( STDIN_FILENO, FIONBIO, &on );   // In Windows nicht möglich
            if( ret == 0 )
            {
                ptr<Channel> new_channel = Z_NEW( Channel( this ) );
        
                new_channel->_read_socket  = STDIN_FILENO;
                new_channel->_write_socket = STDOUT_FILENO;
                new_channel->_socket_send_buffer_size = 1024;

                new_channel->add_to_socket_manager( _spooler->_connection_manager );
                new_channel->socket_expect_signals( Socket_operation::sig_read );

                new_channel->set_event_name( "stdin" );

                _channel_list.push_back( new_channel );
            }
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

//---------------------------------------------------------------------Communication::handle_socket
/*
bool Communication::handle_socket( Channel* channel )
{
    bool ok;

    if( FD_ISSET( channel->_write_socket, &_write_fds ) ) 
    {
        ok = channel->do_send();
        if( !ok )  return false;
    }

    if( FD_ISSET( channel->_read_socket, &_read_fds ) )
    {
        ok = channel->do_recv();
        if( !ok )  return false;
        
        if( channel->_receive_is_complete ) 
        {
            Command_processor cp = _spooler;;
            
            if( channel->_read_socket != STDIN_FILENO )  cp.set_host( &channel->_host );

            string cmd = channel->_text;
            channel->recv_clear();
            channel->_log.info( "Kommando " + cmd );
            channel->_text = cp.execute( cmd, Time::now(), channel->_indent );
          //if( channel->_indent )  channel->_text = channel->_text.replace( "\n", "\r\n" );
            if( cp._error )  channel->_log.error( cp._error->what() );
            ok = channel->do_send();
            if( !ok )  return false;
        }

        if( channel->_eof && channel->_send_is_complete )  return false;
    }

    return true;
}
*/
//---------------------------------------------------------------------------Communication::_fd_set
/*
void Communication::_fd_set( SOCKET socket, fd_set* fdset )
{
    if( socket != SOCKET_ERROR )
    {
        FD_SET( socket, fdset );
        if( _nfds <= socket )  _nfds = socket + 1;
    }
}
*/
//-------------------------------------------------------------------------------Communication::run
/*
int Communication::run()
{
    bool ok;

    if( get_java_vm(false)->running() )  get_java_vm(false)->attach_thread( thread_name() );

    while(1) 
    {
        _nfds = 0;

        Z_MUTEX( _semaphore );
        {

            FD_ZERO( &_read_fds );      
            FD_ZERO( &_write_fds );

            _fd_set( _udp_socket   , &_read_fds );
            _fd_set( _listen_socket, &_read_fds );

            FOR_EACH( Channel_list, _channel_list, it )
            {
                Channel* channel = *it;
                if( channel->_send_is_complete    )  _fd_set( channel->_read_socket , &_read_fds );
                if( channel->_receive_is_complete )  _fd_set( channel->_write_socket, &_write_fds  );
            }

            if( _nfds == 0 )  { _started = false; break; }
        }


        int n;

#       ifdef Z_WINDOWS
            n = ::select( _nfds, &_read_fds, &_write_fds, NULL, NULL );
#        else
            timeval tv;
            tv.tv_sec = 1, tv.tv_usec = 0;

            n = ::select( _nfds, &_read_fds, &_write_fds, NULL, &tv );
            
            if( !main_thread_exists() )  return 0;  //?  Thread bleibt sonst hängen, wenn Java sich bei Ctrl-C sofort verabschiedet. Java lässt SIGINT zu, dieser Thread aber nicht.
#       endif

        THREAD_LOCK( _semaphore )
        {
            if( _terminate )  break;
            if( _rebound )  { _rebound = false; continue; }

            if( n < 0 )  
            {
                if( socket_errno() == EINTR )  continue;
                throw_sos_socket_error( socket_errno(), "select" );
            }


            if( n > 0 )
            {
                // UDP
                if( _udp_socket != SOCKET_ERROR  &&  FD_ISSET( _udp_socket, &_read_fds ) )
                {
                    char buffer [4096];
                    sockaddr_in addr;     
                    socklen_t   addr_len = sizeof addr;

                    addr.sin_addr.s_addr = 0;
                    int len = recvfrom( _udp_socket, buffer, sizeof buffer, 0, (sockaddr*)&addr, &addr_len );
                    if( len > 0 ) 
                    {
                        Host host = addr.sin_addr;
                        if( _spooler->security_level( host ) < Security::seclev_signal )
                        {
                            _spooler->log()->error( "UDP-Nachricht von " + host.as_string() + " nicht zugelassen." );
                        }
                        else
                        {
                            Command_processor cp = _spooler;
                            cp.set_host( &host );
                            string cmd ( buffer, len );
                            _spooler->log()->info( "UDP-Nachricht von " + host.as_string() + ": " + cmd );
                            cp.execute( cmd, Time::now() );
                        }
                    }
                }

                // Neue TCP-Verbindung
                if( _listen_socket != SOCKET_ERROR  &&  FD_ISSET( _listen_socket, &_read_fds ) )
                {
                    Sos_ptr<Channel> new_channel = SOS_NEW( Channel( _spooler ) );
            
                    ok = new_channel->do_accept( _listen_socket );
                    if( ok )
                    {
                        _channel_list.push_back( new_channel );
                    }
                }


                // TCP-Nachricht
                FOR_EACH( Channel_list, _channel_list, it )
                {
                    Channel* channel = *it;

                    ok = handle_socket( channel );
                    if( !ok ) { channel->do_close(); it = _channel_list.erase( it ); }
                }
            }
        }
    }

    if( current_thread_id() == thread_id() 
     && current_thread_id() != _spooler->thread_id()  
     && java_is_running()                            )  get_java_vm(false)->detach_thread();
      
    return 0;
}
*/
//---------------------------------------------------------------------------------------is_started
/*
bool Communication::is_started()
{
    if( !_thread )  return;

    DWORD exit_code;

    BOOL ok = GetExitCodeThread( _thread, exit_code );
    if( !ok )  throw_mswin_error( "GetExitCodeThread" );

    return exit_code == STILL_ACTIVE;
}
*/
//-----------------------------------------------------------------------Communication::thread_main
/*
int Communication::thread_main()
{
    int result;

    Z_WINDOWS_ONLY( SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST ) );

    Ole_initialize ole;

    try 
    {
        result = run();
    }
    catch( const Xc& x )
    {
        _spooler->log()->error( string("Communication::thread: ") + x.what() );
        result = 1;
    }

    _started = false;

    return result;
}
*/
//----------------------------------------------------------------------Communication::start_thread
/*
void Communication::start_thread()
{
    init();
    bind();

    set_thread_name( "Communication" );

    thread_start();

    _started = true;
}
*/
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

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

