// $Id: spooler_communication.cxx,v 1.28 2002/03/03 16:59:41 jz Exp $
/*
    Hier sind implementiert

    Xml_end_finder
    Communication::Channer
    Communication
*/


#include "../kram/sos.h"
#include "spooler.h"

#ifdef SYSTEM_WIN
    const int ENOTSOCK = 10038;
#endif


using namespace std;

namespace sos {
namespace spooler {

//static const int wait_for_free_port = 15;   // Soviele Sekunden warten, bis TCP- oder UDP-Port frei wird

//---------------------------------------------------------------------------------------get_errno

int get_errno() 
{
#   ifdef SYSTEM_WIN
        return WSAGetLastError();
#    else
        return errno;
#   endif
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

//------------------------------------------------------------------Communication::Channel::Channel

Communication::Channel::Channel( Spooler* spooler )
:
    _zero_(this+1),
    _spooler(spooler),
    _socket( SOCKET_ERROR ),
    _send_is_complete( true ),
    _log(_spooler)
{
    recv_clear();
}

//-----------------------------------------------------------------Communication::Channel::~Channel

Communication::Channel::~Channel()
{
    if( _socket != SOCKET_ERROR )  closesocket( _socket );
}

//----------------------------------------------------------------Communication::Channel::do_accept

bool Communication::Channel::do_accept( SOCKET listen_socket )
{
    try
    {
        struct sockaddr_in peer_addr;
        int peer_addr_len = sizeof peer_addr;

        _socket = accept( listen_socket, (struct sockaddr*)&peer_addr, &peer_addr_len );
        if( _socket == SOCKET_ERROR )  throw_sos_socket_error( "accept" );

        _host = peer_addr.sin_addr;
        _log.set_prefix( "TCP-Verbindung mit " + _host.as_string() );

        struct linger l; l.l_onoff=0; l.l_linger=0;
        setsockopt( _socket, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof l );

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
    closesocket( _socket );
    _socket = SOCKET_ERROR;
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

        int len = recv( _socket, buffer, sizeof buffer, 0 );
        if( len <= 0 ) {
            if( len == 0 )  { _eof = true;  return true; }
            if( get_errno() == EAGAIN )  return true; 
            throw_sos_socket_error( "recv" ); 
        }

        const char* p = buffer;

        if( _receive_at_start ) 
        {
            _receive_at_start = false;
            while( p < buffer+len  &&  isspace( (Byte)*p ) )  p++;      // Blanks am Anfang nicht beachten
            len -= p - buffer;
        }

        _receive_is_complete = _xml_end_finder.is_complete( p, len );

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

        int len = send( _socket, _text.c_str() + _send_progress, _text.length() - _send_progress, 0 );
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

//--------------------------------------------------------------------Communication::Communication

Communication::Communication( Spooler* spooler )
: 
    _zero_(this+1), 
    _spooler(spooler),
    _udp_socket(SOCKET_ERROR),
    _listen_socket(SOCKET_ERROR)
{
}

//--------------------------------------------------Communication::Channel::~Communication::Channel

Communication::~Communication()
{
    close();

    TerminateThread( _thread, 99 );
    _thread.close();

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
    {
        Thread_semaphore::Guard guard = &_semaphore;

        _channel_list.clear();
        closesocket( _listen_socket );

        _terminate = true;
    }


    if( _thread ) 
    {
        WaitForSingleObject( _thread, min( (double)INT_MAX, wait_time * 1000.0 ) );
    }
}

//------------------------------------------------------------------------------Communication::bind

void Communication::bind()
{
    struct sockaddr_in  sa;
    int                 ret;
    unsigned long       on = 1;
    BOOL                true_ = 1;
  //int                 wait = 0;       // Längstens wait_for_free_port Sekunden warten


    // UDP:

    if( _udp_port != _spooler->udp_port() )
    {
        Thread_semaphore::Guard guard = &_semaphore;

        if( _udp_socket != SOCKET_ERROR )  closesocket( _udp_socket );
        _udp_port = 0;

        _udp_socket = socket( AF_INET, SOCK_DGRAM, 0 );
        if( _udp_socket == SOCKET_ERROR )  throw_sos_socket_error( "socket" );

      //setsockopt( _udp_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&true_, sizeof true_ );

        sa.sin_port        = htons( _spooler->udp_port() );
        sa.sin_family      = AF_INET;
        sa.sin_addr.s_addr = 0; // INADDR_ANY

        //do {
            ret = ::bind( _udp_socket, (struct sockaddr*)&sa, sizeof sa );
            if( ret == SOCKET_ERROR ) {
                //if( wait++ < wait_for_free_port  &&  get_errno() == EADDRINUSE )  { sos_sleep(1); continue; }
                throw_sos_socket_error( "udp-bind" );
            }
        //} while(0);

        ret = ioctlsocket( _udp_socket, FIONBIO, &on );
        if( ret == SOCKET_ERROR )  throw_sos_socket_error( "ioctl(FIONBIO)" );

        _udp_port = _spooler->udp_port();
        _rebound = true;

        _spooler->log().info( "Spooler erwartet Kommandos über UDP-Port " + as_string(_udp_port) );
    }


    // TCP: 

    if( _tcp_port != _spooler->tcp_port() )
    {
        Thread_semaphore::Guard guard = &_semaphore;

        if( _listen_socket != SOCKET_ERROR )  closesocket( _listen_socket );
        _tcp_port = 0;

        _listen_socket = socket( AF_INET, SOCK_STREAM, 0 );
        if( _listen_socket == SOCKET_ERROR )  throw_sos_socket_error( "socket" );

      //setsockopt( _listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&true_, sizeof true_ );

        sa.sin_port        = htons( _spooler->tcp_port() );
        sa.sin_family      = AF_INET;
        sa.sin_addr.s_addr = 0; // INADDR_ANY

        //do {
            ret = ::bind( _listen_socket, (struct sockaddr*)&sa, sizeof sa );
            if( ret == SOCKET_ERROR ) {
                //if( wait++ < wait_for_free_port  &&  get_errno() == EADDRINUSE )  { sos_sleep(1); continue; }
                throw_sos_socket_error( "tcp-bind" );
            }
        //} while(0);

        ret = listen( _listen_socket, 5 );
        if( ret == SOCKET_ERROR )  throw_errno( get_errno(), "listen" );

        ret = ioctlsocket( _listen_socket, FIONBIO, &on );
        if( ret == SOCKET_ERROR )  throw_sos_socket_error( "ioctl(FIONBIO)" );

        _tcp_port = _spooler->tcp_port();
        _rebound = true;

        _spooler->log().info( "Spooler erwartet Kommandos über TCP-Port " + as_string(_tcp_port) );
    }
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

bool Communication::handle_socket( Channel* channel )
{
    bool ok;

    if( FD_ISSET( channel->_socket, &_write_fds ) ) 
    {
        ok = channel->do_send();
        if( !ok )  return false;
    }

    if( FD_ISSET( channel->_socket, &_read_fds ) )
    {
        ok = channel->do_recv();
        if( !ok )  return false;
        
        if( channel->_receive_is_complete ) 
        {
            Command_processor cp = _spooler;;
            cp.set_host( &channel->_host );
            string cmd = channel->_text;
            channel->recv_clear();
            channel->_log.info( "Kommando " + cmd );
            channel->_text = cp.execute( cmd );
            if( cp._error )  channel->_log.error( cp._error->what() );
            ok = channel->do_send();
        if( !ok )  return false;
        }

        if( channel->_eof && channel->_send_is_complete )  return false;
    }

    return true;
}

//---------------------------------------------------------------------------Communication::_fd_set

void Communication::_fd_set( SOCKET socket, FD_SET* fdset )
{
    if( socket != SOCKET_ERROR )
    {
        FD_SET( socket, fdset );
        if( _nfds <= socket )  _nfds = socket + 1;
    }
}

//-------------------------------------------------------------------------------Communication::run

int Communication::run()
{
    bool ok;

    while(1) 
    {
        _nfds = 0;

        {
            Thread_semaphore::Guard guard = &_semaphore;

            FD_ZERO( &_read_fds );      
            FD_ZERO( &_write_fds );

            _fd_set( _udp_socket, &_read_fds );
            _fd_set( _listen_socket, &_read_fds );

            FOR_EACH( Channel_list, _channel_list, it )
            {
                Channel* channel = *it;
                if( channel->_send_is_complete    )  _fd_set( channel->_socket, &_read_fds );
                if( channel->_receive_is_complete )  _fd_set( channel->_socket, &_write_fds  );
            }

            if( _nfds == 0 )  { _started = false; break; }
        }

        int n = ::select( _nfds, &_read_fds, &_write_fds, NULL, NULL );

        {
            Thread_semaphore::Guard guard = &_semaphore;

            if( _terminate )  break;
            if( _rebound )  { _rebound = false; continue; }

            if( n < 0 )  throw_sos_socket_error( get_errno(), "select" );

            
            // UDP
            if( _udp_socket != SOCKET_ERROR  &&  FD_ISSET( _udp_socket, &_read_fds ) )
            {
                char buffer [4096];
                sockaddr_in addr;     
                int         addr_len = sizeof addr;

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
                        cp.execute( cmd );
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

    return 0;
}

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
//-----------------------------------------------------------------------------------------------go

int Communication::go()
{
    int result;

    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST );

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

//-------------------------------------------------------------------------------------------thread

static uint __stdcall thread( void* param )
{
    return ((Communication*)param)->go();
}

//----------------------------------------------------------------------Communication::start_thread

void Communication::start_thread()
{
    Thread_id thread_id;

    init();
    bind();

    _started = true;

    _thread = _beginthreadex( NULL,                        // no security attributes 
                              0,                           // use default stack size  
                              thread,                      // thread function 
                              this,                        // argument to thread function 
                              0,                           // use default creation flags 
                              &thread_id );                // returns the thread identifier 
 
    if( !_thread )  throw_mswin_error( "CreateThread" );
}

//-------------------------------------------------------------------Communication::start_or_rebind

void Communication::start_or_rebind()
{
    Thread_semaphore::Guard guard = &_semaphore;

    if( _started )  
    {
        rebind();
    }
    else 
    {
        if( _thread ) 
        {
            WaitForSingleObject( _thread, INT_MAX );    // Thread sollte beendet sein
            CloseHandle( _thread );
            _thread.close();
        }

        start_thread();
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos


