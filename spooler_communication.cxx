// $Id: spooler_communication.cxx,v 1.72 2004/01/06 16:39:36 jz Exp $
/*
    Hier sind implementiert

    Xml_end_finder
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
    const int ENOTSOCK   = 10038;
    const int EADDRINUSE = WSAEADDRINUSE;
    const int STDIN_FILENO  = (int)GetStdHandle( STD_INPUT_HANDLE );//0;
    const int STDOUT_FILENO = (int)GetStdHandle( STD_OUTPUT_HANDLE );//1;
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

//---------------------------------------------------------------------------------------get_errno

int get_errno() 
{
#   ifdef SYSTEM_WIN
        return WSAGetLastError();
#    else
        return errno;
#   endif
}

//---------------------------------------------------------------------------------------set_linger

static void set_linger( SOCKET socket )
{
    struct linger l; 
    
    l.l_onoff  = 1; 
    l.l_linger = 5;  // Sekunden

    setsockopt( socket, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof l );
}

//-------------------------------------------------------------------------------------is_ip_number
/*
bool is_ip_number( const string& host_addr )
{
    const Byte* p = host_addr.c_str();

    while( isdigit( *p )  ||  *p == '.' )  p++;

    return *p == '\0';
}
*/

//------------------------------------------------------------------------------------Host::netmask

uint32 Host::netmask() const
{ 
    uint32 net_nr = ntohl(_ip.s_addr);
    uint32 netmask_nr;
    
    // In Linux macht das inet_netof()
    if( ( net_nr >> 24 ) >= 192 )  netmask_nr = 0xFFFFFF00;
    else
    if( ( net_nr >> 24 ) >= 128 )  netmask_nr = 0xFFFF0000;
    else                           netmask_nr = 0xFF000000;

    return htonl(netmask_nr);
}

//----------------------------------------------------------------------------------------Host::net

Host Host::net() const
{
    return Host( _ip.s_addr & netmask() );
}

//-----------------------------------------------------------------------Host::get_host_set_by_name

set<Host> Host::get_host_set_by_name( const string& name )
{
    set<Host> host_set;

    uint32 ip = inet_addr( name.c_str() );

    if( ip != INADDR_NONE )
    {
        host_set.insert( ip );
    }
    else
    {
        LOG( "gethostbyname " << name << '\n' );
        hostent* h = gethostbyname( name.c_str() );     // In Unix nicht thread-sicher?
        if( !h )  throw_sos_socket_error("gethostbyname", name.c_str() );
        uint32** p = (uint32**)h->h_addr_list;
        while( *p )  host_set.insert( **p ), p++;
    }        

    return host_set;
}

//-----------------------------------------------------------------------------Named_host::set_name

void Named_host::set_name()
{
    hostent* h = gethostbyaddr( (char*)&_ip, sizeof _ip, AF_INET );
    if( !h )  return; //{ string n=as_string(); throw_sos_socket_error("gethostbyaddr", n.c_str() ); }

    _name = h->h_name;
}

//-------------------------------------------------------------------Xml_end_finder::Xml_end_finder

Xml_end_finder::Xml_end_finder()
: 
    _zero_(this+1) 
{
    _tok[cdata_tok  ]._begin = "<![CDATA[";
    _tok[cdata_tok  ]._end   = "]]>";
    _tok[comment_tok]._begin = "<!--";
    _tok[comment_tok]._end   = "-->";
}

//-----------------------------------------------------------------------Xml_end_finder::step_begin

bool Xml_end_finder::Tok_entry::step_begin( char c )      
{ 
    if( c == _begin[_index] )
    {
        _index++; 
    
        if( _begin[_index] == '\0' ) 
        {
            _index = 0; 
            _active = true;
            return true;
        }
    }
    else  
    {
        _index = 0; 
    }

    return false;
}

//--------------------------------------------------------------Xml_end_finder::Tok_entry::step_end

void Xml_end_finder::Tok_entry::step_end( char c )      
{ 
    if( c == _end[_index] )
    {
        _index++; 
        if( _end[_index] == '\0' )  _index = 0, _active = false;
    }
    else  
        _index = 0; 
}

//----------------------------------------------------------------------Xml_end_finder::is_complete

bool Xml_end_finder::is_complete( const char* p, int len )
{
    const char* p0 = p;
    while( p < p0+len )
    {
        if( *p != '\0' ) 
        {
            if( _tok[cdata_tok  ]._active )  _tok[cdata_tok  ].step_end( *p );
            else
            if( _tok[comment_tok]._active )  _tok[comment_tok].step_end( *p );
            else
            if( _in_tag )
            {
                if( _at_start_tag ) 
                {
                    if( *p == '/' )  _in_end_tag = true;               // "</"
                    else
                    if( !isalpha(*p) )  _in_special_tag = true;
                    _at_start_tag = false;
                }
                else
                if( *p == '>' ) 
                {
                    _in_tag = false;
                    
                    if( _in_end_tag ) 
                    {
                        _open_elements--;
                        if( _open_elements == 0 )  { _xml_is_complete = true; break; }
                    }
                    else
                    if( !_in_special_tag )
                    {
                        if( _last_char != '/' )  _open_elements++;
                        else 
                        if( _open_elements == 0 )  { _xml_is_complete = true; break; }     // Das Dokument ist nur ein leeres XML-Element
                    }
                    
                    _in_special_tag = false;
                }
            }
            else
            {
                bool in_something = false;
                in_something |= _tok[cdata_tok  ].step_begin( *p );    
                in_something |= _tok[comment_tok].step_begin( *p );

                if( in_something )  _at_start_tag = false, _in_tag = false, _in_special_tag = false;
                else
                if( *p == '<' )  _in_tag = true, _at_start_tag = true;
                else
                if( _open_elements == 0  &&  !isspace( (Byte)*p ) )  { _xml_is_complete = true; break; }  // Das ist ein Fehler
            }
        }

        _last_char = *p;
        p++;
    }

    return _xml_is_complete;
}

//----------------------------------------------------Communication::Listen_socket::async_continue_

bool Communication::Listen_socket::async_continue_( bool wait )
{
    bool something_done = false;

    //if( socket_read_signaled() )
    {
        ptr<Channel> new_channel = Z_NEW( Channel( _spooler ) );

        bool ok = new_channel->do_accept( _read_socket );
        if( ok )
        {
            _communication->_channel_list.push_back( new_channel );

            new_channel->add_to_socket_manager( _spooler->_connection_manager );
            new_channel->socket_expect_signals( Socket_operation::sig_read | Socket_operation::sig_write );
            something_done = true;
        }
    }

    async_clear_signaled();
    return something_done;
}

//-------------------------------------------------------Communication::Udp_socket::async_continue_

bool Communication::Udp_socket::async_continue_( bool wait )
{
    bool something_done = false;

    //if( socket_read_signaled() )
    {
        char buffer [4096];
        sockaddr_in addr;     
        socklen_t   addr_len = sizeof addr;

        addr.sin_addr.s_addr = 0;
        int len = recvfrom( _read_socket, buffer, sizeof buffer, 0, (sockaddr*)&addr, &addr_len );
        if( len > 0 ) 
        {
            Named_host host = addr.sin_addr;
            if( _spooler->security_level( host ) < Security::seclev_signal )
            {
                _spooler->log().error( "UDP-Nachricht von " + host.as_string() + " nicht zugelassen." );
            }
            else
            {
                Command_processor cp ( _spooler );
                cp.set_host( &host );
                string cmd ( buffer, len );
                _spooler->log().info( "UDP-Nachricht von " + host.as_string() + ": " + cmd );
                cp.execute( cmd, Time::now() );
            }
            
            something_done = true;
        }

    }

    async_clear_signaled();
    return something_done;
}

//------------------------------------------------------------------Communication::Channel::Channel

Communication::Channel::Channel( Spooler* spooler )
:
    _zero_(this+1),
    _spooler(spooler),
    _send_is_complete( true ),
    _log(_spooler)
{
    recv_clear();
}

//-----------------------------------------------------------------Communication::Channel::~Channel

Communication::Channel::~Channel()
{
    if( _read_socket != SOCKET_ERROR  &&  _read_socket != STDIN_FILENO )
    {
        LOG2( "socket.close", "close(" << _read_socket << ")\n" );
        closesocket( _read_socket );
    }

    _read_socket = SOCKET_ERROR;
    _write_socket = SOCKET_ERROR;
}

//----------------------------------------------------------------Communication::Channel::do_accept

bool Communication::Channel::do_accept( SOCKET listen_socket )
{
    try
    {
        struct sockaddr_in peer_addr;
        socklen_t          peer_addr_len = sizeof peer_addr;

        LOG2( "socket.accept", "accept(" << listen_socket << ")\n" );
        _read_socket = accept( listen_socket, (struct sockaddr*)&peer_addr, &peer_addr_len );
        if( _read_socket == SOCKET_ERROR )  throw_sos_socket_error( "accept" );
        
        _write_socket = _read_socket;

        set_linger( _read_socket );

        _host = peer_addr.sin_addr;
        _log.set_prefix( "TCP-Verbindung mit " + _host.as_string() );


        if( _spooler->security_level( _host ) <= Security::seclev_signal )
        {
            _log.info( "TCP-Verbindung nicht zugelassen" );
            do_close();
            return false;
        }

        _log.info( "TCP-Verbindung angenommen" );

    }
    catch( const Xc& x ) { _log.error(x.what()); return false; }

    return true;
}

//-----------------------------------------------------------------Communication::Channel::do_close

void Communication::Channel::do_close()
{
    if( _read_socket == STDIN_FILENO  &&  _eof  &&  isatty(STDIN_FILENO) )  _spooler->cmd_terminate();  // Ctrl-D (auch wenn Terminal-Sitzung beendet?)

    if( _read_socket != SOCKET_ERROR  &&  _read_socket != STDIN_FILENO )
    {
        LOG2( "socket.close", "close(" << _read_socket << ")\n" );
        closesocket( _read_socket );
    }
    
    _read_socket  = SOCKET_ERROR;
    _write_socket = SOCKET_ERROR;
}

//---------------------------------------------------------------Communication::Channel::recv_clear

void Communication::Channel::recv_clear()
{
    _receive_at_start = true; 
    _receive_is_complete = false;
    _text = "";
    _xml_end_finder = Xml_end_finder();
}

//------------------------------------------------------------------Communication::Channel::do_recv

bool Communication::Channel::do_recv()
{
    try
    {
        char buffer [ 4096 ];

        LOG2( "socket.recv", "recv/read(" << _read_socket << ")\n" );
        int len = _read_socket == STDIN_FILENO? read( _read_socket, buffer, sizeof buffer )
                                              : recv( _read_socket, buffer, sizeof buffer, 0 );

        if( len <= 0 ) {
            if( len == 0 )  { _eof = true;  return true; }
            if( get_errno() == EAGAIN )  return true; 
            throw_sos_socket_error( "recv" ); 
        }

        const char* p = buffer;

        if( _receive_at_start ) 
        {
            if( len == 1  &&  buffer[0] == '\x04' )  return false;      // Einzelnes Ctrl-D beendet Sitzung
            if( len == 1  &&  buffer[0] == '\n'   
             || len == 2  &&  buffer[0] == '\r'  &&  buffer[1] == '\n' )  { _spooler->signal( "do_something!" );  _spooler->_last_time_enter_pressed = Time::now().as_time_t(); return true; }

            _receive_at_start = false;
            while( p < buffer+len  &&  isspace( (Byte)*p ) )  p++;      // Blanks am Anfang nicht beachten
            len -= p - buffer;
        }

        _receive_is_complete = _xml_end_finder.is_complete( p, len );

        if( len >= 2  &&  buffer[len-2] == '\r' )  _indent = true;      // CR LF am Ende lässt Antwort einrücken. CR LF soll nur bei telnet-Eingabe kommen.
      //if( len >= 1  &&  buffer[len-1] == '\n' )  _indent = true;      // LF am Ende lässt Antwort einrücken. LF soll nur bei telnet-Eingabe kommen.
        _text.append( p, len );
    }
    catch( const Xc& x ) { _log.error(x.what()); return false; }

    return true;
}

//------------------------------------------------------------------Communication::Channel::do_send

bool Communication::Channel::do_send()
{
    try
    {
        if( _send_is_complete )  _send_progress = 0, _send_is_complete = false;     // Am Anfang

        int count = _text.length() - _send_progress;
        if( count == 0 )  return true;

        LOG2( "socket.send", "send/write(" << _write_socket << "," << count << " bytes)\n" );
        int len = _write_socket == STDOUT_FILENO? write ( _write_socket, _text.c_str() + _send_progress, count )
                                                : ::send( _write_socket, _text.c_str() + _send_progress, count, 0 );
        if( len < 0 ) {
            if( get_errno() == EAGAIN )  return true;
            throw_sos_socket_error( "send" );
        }

        _send_progress += len;

        if( _send_progress == _text.length() )
        {
            _send_is_complete = true;
            _text = "";
        }
    }
    catch( const Xc& x ) { _log.error(x.what()); return false; }

    return true;
}

//----------------------------------------------------------Communication::Channel::async_continue_

bool Communication::Channel::async_continue_( bool wait )
{
    bool something_done = false;

    //if( socket_write_signaled() ) 
    {
        something_done |= do_send();
        //if( !ok )  return false;
    }

    //if( socket_read_signaled() )
    {
        bool ok = do_recv();
        if( ok )
        {
            something_done = true;

            if( _receive_is_complete ) 
            {
                Command_processor cp ( _spooler );
                
                if( _read_socket != STDIN_FILENO )  cp.set_host( &_host );

                string cmd = _text;
                recv_clear();
                _log.info( "Kommando " + cmd );
                _text = cp.execute( cmd, Time::now(), _indent );
                //if( _indent )  channel->_text = _text.replace( "\n", "\r\n" );
                if( cp._error )  _log.error( cp._error->what() );
                ok = do_send();
                //if( !ok )  return false;
            }

            //if( _eof && _send_is_complete )  return false;
        }
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

//--------------------------------------------------Communication::Channel::~Communication::Channel

Communication::~Communication()
{
    close();

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
    //THREAD_LOCK( _semaphore )
    {
        _channel_list.clear();

        _listen_socket.close();
        _udp_socket.close();

        _terminate = true;
    }

/*
#   ifdef Z_WINDOWS
       thread_wait_for_termination( wait_time );
#    else
       thread_wait_for_termination();
#   endif

    thread_close();
*/
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

int Communication::bind_socket( SOCKET socket, struct sockaddr_in* sa )
{
    int ret;
    int true_ = 1;
    
    LOG( "setsockopt(" << socket << ",SOL_SOCKET,SO_REUSEADDR,1)  " );
    ret = setsockopt( socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&true_, sizeof true_ );
    LOG( "ret=" << ret );  if( ret == SOCKET_ERROR )  LOG( "errno=" << errno << "  "  << strerror(errno) );
    LOG( "\n" );

    ret = ::bind( socket, (struct sockaddr*)sa, sizeof (struct sockaddr_in) );

    if( ret == SOCKET_ERROR  &&  get_errno() == EADDRINUSE )
    {
        _spooler->_log.warn( "Port " + as_string( ntohs( sa->sin_port ) ) + " ist blockiert. Wir probieren es " + as_string(wait_for_port_available) + " Sekunden" );

        for( int i = 1; i <= wait_for_port_available; i++ )
        {
            if( ctrl_c_pressed || _spooler->state() == Spooler::s_stopping || _spooler->state() == Spooler::s_stopped )  return EINTR;
            //if( !main_thread_exists() )  return EINTR;  //?  Thread bleibt sonst hängen, wenn Java sich bei Ctrl-C sofort verabschiedet. Java lässt SIGINT zu, dieser Thread aber nicht.

            sos_sleep(1);
        
            ret = ::bind( socket, (struct sockaddr*)sa, sizeof (struct sockaddr_in) );
            if( ret != SOCKET_ERROR ) break;
            if( get_errno() != EADDRINUSE )  break;

            if( isatty( fileno(stderr) ) && isatty( fileno(stdin) ))  fputc( i % 10 == 0? '0' + i / 10 % 10 : '.', stderr );
        }

        if( isatty( fileno(stderr) ) )  fputc( '\n', stderr );
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
                if( _udp_socket._read_socket == SOCKET_ERROR )  throw_sos_socket_error( "socket" );

                set_linger( _udp_socket._read_socket );
                
                sa.sin_port        = htons( _spooler->udp_port() );
                sa.sin_family      = AF_INET;
                sa.sin_addr.s_addr = 0; // INADDR_ANY

                ret = bind_socket( _udp_socket._read_socket, &sa );
                if( ret == SOCKET_ERROR )  throw_sos_socket_error( "udp-bind ", as_string(_spooler->udp_port()).c_str() );

                ret = ioctlsocket( _udp_socket._read_socket, FIONBIO, &on );
                if( ret == SOCKET_ERROR )  throw_sos_socket_error( "ioctl(FIONBIO)" );

                _udp_port = _spooler->udp_port();
                _rebound = true;

                _spooler->log().info( "Scheduler erwartet Kommandos über UDP-Port " + sos::as_string(_udp_port) );
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
                _listen_socket._read_socket = socket( AF_INET, SOCK_STREAM, 0 );
                if( _listen_socket._read_socket == SOCKET_ERROR )  throw_sos_socket_error( "socket" );

                set_linger( _listen_socket._read_socket );
                
                sa.sin_port        = htons( _spooler->tcp_port() );
                sa.sin_family      = AF_INET;
                sa.sin_addr.s_addr = 0; // INADDR_ANY

                ret = bind_socket( _listen_socket._read_socket, &sa );
                if( ret == SOCKET_ERROR )  throw_sos_socket_error( "tcp-bind", as_string(_spooler->tcp_port()).c_str() );

                LOG2( "socket.listen", "listen()\n" );
                ret = listen( _listen_socket._read_socket, 5 );
                if( ret == SOCKET_ERROR )  throw_errno( get_errno(), "listen" );

                ret = ioctlsocket( _listen_socket._read_socket, FIONBIO, &on );
                if( ret == SOCKET_ERROR )  throw_sos_socket_error( "ioctl(FIONBIO)" );

                _tcp_port = _spooler->tcp_port();
                _rebound = true;

                _spooler->log().info( "Scheduler erwartet Kommandos über TCP-Port " + sos::as_string(_tcp_port) );
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
                Sos_ptr<Channel> new_channel = SOS_NEW( Channel( _spooler ) );
        
                new_channel->_read_socket  = STDIN_FILENO;
                new_channel->_write_socket = STDOUT_FILENO;
                new_channel->_indent = true;

                _channel_list.push_back( new_channel );

                _udp_socket.add_to_socket_manager( _spooler->_connection_manager );
                _udp_socket.socket_expect_signals( Socket_operation::sig_read );
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
        if( ret )  throw_sos_socket_error( ret, "WSAStartup" );
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
                if( get_errno() == EINTR )  continue;
                throw_sos_socket_error( get_errno(), "select" );
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
                        Named_host host = addr.sin_addr;
                        if( _spooler->security_level( host ) < Security::seclev_signal )
                        {
                            _spooler->log().error( "UDP-Nachricht von " + host.as_string() + " nicht zugelassen." );
                        }
                        else
                        {
                            Command_processor cp = _spooler;
                            cp.set_host( &host );
                            string cmd ( buffer, len );
                            _spooler->log().info( "UDP-Nachricht von " + host.as_string() + ": " + cmd );
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
        _spooler->log().error( string("Communication::thread: ") + x.what() );
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

