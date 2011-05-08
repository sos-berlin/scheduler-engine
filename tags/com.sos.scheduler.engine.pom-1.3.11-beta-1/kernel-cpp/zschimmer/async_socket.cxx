// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "zschimmer.h"
#include "async_socket.h"
#include "log.h"
#include "z_sockets.h"

#include <math.h>

#ifdef Z_WINDOWS
#   include <io.h>
#   ifndef ENOTSOCK
        const int ENOTSOCK   = 10038;
        const int EADDRINUSE = WSAEADDRINUSE;
#   endif
#   define ioctl    ioctlsocket
#   define isatty   _isatty
#else
#   include <unistd.h>
#   include <sys/types.h>
#   include <signal.h>
#endif

using namespace std;


namespace zschimmer {

//--------------------------------------------------------------------------------------------const

const int                       default_buffer_size                 = 4096; //1024;
const size_t                    Socket_stream::read_bytes_maximum   = 10000;

static Message_code_text error_codes[] =
{
    { "Z-ASYNC-SOCKET-001", "Too many file handles: $1, allowed maximum is FD_SETSIZE=$2" },        // Wird auch in com_remote.cxx verwendet
    { NULL }
};

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( async_socket )
{
    add_message_code_texts( error_codes ); 
}

//-------------------------------------------------------------------------------------socket_errno

inline int socket_errno() 
{
#   ifdef Z_WINDOWS
        return WSAGetLastError();
#    else
        return errno;
#   endif
}

//-------------------------------------------------------------------------------------print_fd_set

void print_fd_set( ostream* s, const fd_set& fds, int length )
{
    *s << '{';
    bool first = true;

    #ifndef Z_WINDOWS
        if( length >= FD_SETSIZE )  z::throw_xc( "Z-ASYNC-SOCKET-001", length, FD_SETSIZE, Z_FUNCTION );
    #endif

    for( int i = 0; i < length; i++ )
    {
        if( FD_ISSET( i, &fds ) )
        {
            if( !first )  *s << ' ';
            first = false;
            *s << i;
        }
    }

    *s << '}';
}

//------------------------------------------------------------------------------------print_fd_sets

void print_fd_sets( ostream* s, const fd_set* read_fds, const fd_set* write_fds, const fd_set* except_fds, int length )
{
    if( read_fds   )  *s << "read=",        print_fd_set( s, *read_fds  , length );
    if( write_fds  )  *s << " write=",      print_fd_set( s, *write_fds , length );
    if( except_fds )  *s << " exception=",  print_fd_set( s, *except_fds, length );
}

//---------------------------------------------------------------Socket_operation::Socket_operation

Socket_operation::Socket_operation( SOCKET s )
: 
    //Event_operation( manager ),
    _zero_(this+1),
    _read_socket ( s ),
    _write_socket( s ),
    _close_socket_at_end( s == SOCKET_ERROR )
{
    _socket_event.set_name( "socket" );

    //if( manager )  add_to_socket_manager( manager );
}

//--------------------------------------------------------------Socket_operation::~Socket_operation

Socket_operation::~Socket_operation()
{
    remove_from_socket_manager();

    if( _close_socket_at_end )
    {
        if( _write_socket != SOCKET_ERROR ) 
        {
            Z_LOG2( "socket.close", "close(" << _write_socket << ")\n" );
            closesocket( _write_socket );
        }

        if( _read_socket  != SOCKET_ERROR &&  _read_socket != _write_socket )  
        {
            Z_LOG2( "socket.close", "close(" << _read_socket << ")\n" );
            closesocket( _read_socket );
        }
    }
}

//--------------------------------------------------------------------------Socket_operation::close

void Socket_operation::close()
{
    set_async_parent( NULL );
    remove_from_socket_manager();

    if( _close_socket_at_end )
    {
        if( _write_socket != SOCKET_ERROR )
        {
            Z_LOG2( "socket.close", "close(" << _write_socket << ")\n" );
            int ret = closesocket( _write_socket );
            
            if( _read_socket == _write_socket )  _read_socket = SOCKET_ERROR;

            _write_socket = SOCKET_ERROR;

            if( ret == SOCKET_ERROR )  throw_socket( errno, _peer_host_and_port.as_string().c_str() );   // In Windows WSA..() verwenden?
        }

        if( _read_socket != SOCKET_ERROR )
        {
            Z_LOG2( "socket.close", "close(" << _read_socket << ")\n" );
            int ret = closesocket( _read_socket );
            _read_socket = SOCKET_ERROR;

            if( ret == SOCKET_ERROR )  throw_socket( errno, _peer_host_and_port.as_string().c_str() );   // In Windows WSA..() verwenden?
        }
    }

    set_socket_event_name( "closed" );
/*
#   ifdef Z_WINDOWS
        if( _socket_event != WSA_INVALID_EVENT )
        {
            WSAEventSeelct( _socket, NULL, 0 );
            WSACloseEvent( _socket_event );
            _socket_event = WSA_INVALID_EVENT;
        }
#   endif
*/
    _eof = false;
}

//----------------------------------------------------------Socket_operation::add_to_socket_manager

void Socket_operation::add_to_socket_manager( Socket_manager* socket_manager )
{
    if( _socket_manager )
    {
        if( _socket_manager != socket_manager )  throw_xc( Z_FUNCTION );
        return;
    }

    _socket_manager = socket_manager;
    _socket_manager->add_socket_operation( this );
}

//-----------------------------------------------------Socket_operation::remove_from_socket_manager

void Socket_operation::remove_from_socket_manager()
{
    if( _socket_manager ) 
    {
        _socket_manager->remove_socket_operation( this );
        _socket_manager = NULL;
    }
}

//-------------------------------------------------------------------------Socket_operation::listen
/*
void Socket_operation::listen()
{
    Z_LOG2( "socket.listen", "listen()\n" );
    int ret = ::listen( _socket, 5 );
    if( ret == SOCKET_ERROR )  throw_socket( socket_errno(), "listen" );

    unsigned long on = 1;
    ret = ioctlsocket( _socket, FIONBIO, &on );
    if( ret == SOCKET_ERROR )  throw_socket( socket_errno(), "ioctl(FIONBIO)" );

#   ifdef Z_WINDOWS
        BOOL err = WSAEventSelect( _socket, _socket_event, FD_ACCEPT );
        if( err )  throw_socket( socket_errno(), "WSAEventSelect" );
#   endif
}
*/
//-------------------------------------------------------------------------Socket_operation::accept

bool Socket_operation::accept( SOCKET listen_socket )
{
    struct sockaddr_in  peer_addr;
    sockaddrlen_t       peer_addr_len = sizeof peer_addr;

    memset( &peer_addr, 0, sizeof peer_addr );

    Z_LOG2( "socket.accept", "accept(" << listen_socket << ")\n" );
    _read_socket = ::accept( listen_socket, (struct sockaddr*)&peer_addr, &peer_addr_len );

    if( _read_socket == SOCKET_ERROR )  
    {
        int err = socket_errno();
        Z_LOG2( "socket.accept","  errno=" << err << "\n" );
        if( err == Z_EWOULDBLOCK )  return false;
        if( err == ENOENT      )  return false;
        throw_socket( err, "accept", _peer_host_and_port.as_string().c_str() );
    }

    set_socket_not_inheritable( _read_socket );

    _peer_host_and_port = peer_addr;
    _write_socket = _read_socket;

    set_socket_event_name( "accepted" );

    return true;
}

//----------------------------------------------------------Socket_operation::set_socket_event_name

void Socket_operation::set_socket_event_name( const string& state_name)
{
    S s;
    s << "Socket_operation(" << _read_socket << ", TCP " << _peer_host_and_port;
    if( state_name != "" )  s << " " << state_name;
    s << ")";

    _socket_event.set_name( s );
}

//---------------------------------------------------------------------Socket_operation::call_ioctl

void Socket_operation::call_ioctl( int what, unsigned long value )
{
    if( _read_socket != SOCKET_ERROR )
    {
        Z_LOG2( "socket", "ioctl(" << _read_socket << "," << what << "," << value << ")\n" );
        int ret = ioctlsocket( _read_socket, what, &value );
        if( ret == SOCKET_ERROR )  throw_socket( socket_errno(), "ioctl()", _peer_host_and_port.as_string().c_str() );
    }

    if( _write_socket != SOCKET_ERROR  &&  _read_socket != _write_socket )
    {
        Z_LOG2( "socket", "ioctl(" << _read_socket << "," << what << "," << value << ")\n" );
        int ret = ioctlsocket( _write_socket, what, &value );
        if( ret == SOCKET_ERROR )  throw_socket( socket_errno(), "ioctl()", _peer_host_and_port.as_string().c_str() );
    }
}

//------------------------------------------------------------------Socket_operation::set_keepalive

void Socket_operation::set_keepalive( bool b )
{
    bool ok = try_set_keepalive( b );
    if( !ok )  throw_socket( socket_errno(), "setsockopt", "SO_KEEPALIVE" );
}

//--------------------------------------------------------------Socket_operation::try_set_keepalive

bool Socket_operation::try_set_keepalive( bool b )
{
    bool ok    = true;
    int  value = b;

    if( _write_socket != SOCKET_ERROR  &&  _write_socket != _read_socket )
    {
        Z_LOG2( "socket.setsockopt", "setsockopt(" << _write_socket << ",SOL_SOCKET,SO_KEEPALIVE," << value << ")\n" );
        int ret = ::setsockopt( _write_socket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&value, sizeof value );
        if( ret == -1 )  ok = false;    // Nichts run, was socket_errno() ändern könnte! Kein Z_LOG!
    }

    if( ok  &&  _read_socket != SOCKET_ERROR )
    {
        Z_LOG2( "socket.setsockopt", "setsockopt(" << _read_socket << ",SOL_SOCKET,SO_KEEPALIVE," << value << ")\n" );
        int ret = ::setsockopt( _read_socket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&value, sizeof value );
        if( ret == -1 )  ok = false;    // Nichts run, was socket_errno() ändern könnte! Kein Z_LOG!
    }

    // Nichts run, was socket_errno() ändern könnte! Kein Z_LOG!

    return ok;
}

//----------------------------------------------------------------Socket_operation::set_tcp_nodelay
/*
    Nagles Algorithmus verzögert nur, wenn ein noch nicht bestätigtes Paket unterwegs ist.
    Wenn wir immer nur ein Paket senden, kann der Algorithmus nicht verzögern.

void Socket_operation::set_tcp_nodelay( bool b )
{
    int value = b;

    if( _read_socket != SOCKET_ERROR )
    {
        Z_LOG2( "socket.setsockopt", "setsockopt(" << _read_socket << ",IPPROTO_TCP,TCP_NODELAY," << value << ")\n" );
        ::setsockopt( _read_socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&value, sizeof value );
    }

    if( _write_socket != SOCKET_ERROR  &&  _write_socket != _read_socket )
    {
        Z_LOG2( "socket.setsockopt", "setsockopt(" << _write_socket << ",IPPROTO_TCP,TCP_NODELAY," << value << ")\n" );
        ::setsockopt( _write_socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&value, sizeof value );
    }
}
*/
//---------------------------------------------------------------------------------------set_linger

void Socket_operation::set_linger( bool on, int seconds )
{
    struct linger l; 
    
    l.l_onoff  = on;        // 0: Kein Linger, 1: Linger (mit l_linger)
    l.l_linger = seconds;   // Sekunden. 0: Sofort schließen (schickt RST, Verbindungsabbruch?)
                            // >0: Bei Scheduler-Restart kann der Browser hängen. Windows schließt die Verbindung manchmal nie, obwohl Prozess längst beendet ist.

    if( _read_socket != SOCKET_ERROR )
    {
        Z_LOG2( "socket.setsockopt", "setsockopt(" << _read_socket << ",SOL_SOCKET,SO_LINGER," << on << ',' << seconds << ")\n" );
        setsockopt( _read_socket, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof l );
    }

    if( _write_socket != SOCKET_ERROR  &&  _write_socket != _read_socket )
    {
        Z_LOG2( "socket.setsockopt", "setsockopt(" << _read_socket << ",SOL_SOCKET,SO_LINGER," << on << ',' << seconds << ")\n" );
        setsockopt( _write_socket, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof l );
    }
}

//--------------------------------------------------------Socket_operation::socket_use_accept_event
/*
#ifdef Z_WINDOWS

void Socket_operation::socket_use_accept_event()
{
    BOOL err = WSAEventSelect( _socket, _socket_event, FD_ACCEPT );
    if( err )  throw_socket( socket_errno(), "WSAEventSelect" );
}

#endif
*/
//----------------------------------------------------------------------------socket_expect_signals

void Socket_operation::socket_expect_signals( Signals signals )
{
#   ifdef Z_WINDOWS

        if( signals & sig_read   )  _wsa_event_select_flags |= FD_READ  | FD_CLOSE | FD_ACCEPT;
        if( signals & sig_write  )  _wsa_event_select_flags |= FD_WRITE | FD_CLOSE | FD_CONNECT;
      //if( signals & sig_except )  _wsa_event_select_flags |= FD_...;

        if( _write_socket != SOCKET_ERROR )
        {
            BOOL err = WSAEventSelect( _write_socket, _socket_event, _wsa_event_select_flags );
            if( err )  throw_socket( socket_errno(), "WSAEventSelect" );
        }

        if( _read_socket != SOCKET_ERROR  &&  _read_socket != _write_socket )
        {
            BOOL err = WSAEventSelect( _read_socket, _socket_event, _wsa_event_select_flags );
            if( err )  throw_socket( socket_errno(), "WSAEventSelect" );
        }

#    else

        if( signals & sig_read   )  
        {
            if( _read_socket != SOCKET_ERROR )  _socket_manager->set_fd( Socket_manager::read_fd, _read_socket );
        }

        if( signals & sig_write  )
        {
            //fdset erst setzen, wenn send() ein Z_EWOULDBLOCK liefert.
            //if( _write_socket != SOCKET_ERROR )  _socket_manager->set_write_fd( _write_socket );
        }

        if( signals & sig_except )  
        {
            if( _read_socket  != SOCKET_ERROR )  _socket_manager->set_fd( Socket_manager::except_fd, _read_socket );
            if( _write_socket != SOCKET_ERROR )  _socket_manager->set_fd( Socket_manager::except_fd, _write_socket );
        }
        
#   endif
}

//----------------------------------------------------------------Socket_operation::async_signaled_

bool Socket_operation::async_signaled_()
{
#   ifndef Z_WINDOWS

        if( _socket_manager )
        {
            if( _read_socket  != SOCKET_ERROR  &&                                     _socket_manager->socket_signaled( _read_socket  ) )  return true;
            if( _write_socket != SOCKET_ERROR  &&  _write_socket != _read_socket  &&  _socket_manager->socket_signaled( _write_socket ) )  return true;
        }

#   endif

    return _socket_event.signaled();        // In Unix nur bei Http_response/Log_chunk_reader
}

//----------------------------------------------------------Socket_operation::async_clear_signaled_

void Socket_operation::async_clear_signaled_()
{
#   ifndef Z_WINDOWS

    if( _socket_manager )
    {
        if( _read_socket  != SOCKET_ERROR                                    )  _socket_manager->clear_socket_signaled( _read_socket  );
        if( _write_socket != SOCKET_ERROR  &&  _write_socket != _read_socket )  _socket_manager->clear_socket_signaled( _write_socket );
    }

#   endif

    _socket_event.reset();
}

//--------------------------------------------------------------Socket_operation::async_state_text_

string Socket_operation::async_state_text_() const
{
    S result;

    result << "socket(" << _read_socket;
    if( _read_socket != _write_socket )  result << ",write=" << _write_socket;
    result << ",";
    result << _peer_host_and_port.as_string();
    result << ")";

    return result;
}

//-------------------------------------------------------------------------------------------------

/*
bool Socket_operation::socket_read_signaled()
{ 
    if( _socket_signals & sig_read   )  return true;

#   ifdef Z_WINDOWS
        if( _socket_event.signaled() )  return true;
#   endif

    return false;

}
    bool                        socket_write_signaled       ()                                      { return ( _socket_signals & sig_write  ) != 0; }
    bool                        socket_except_signaled      ()                                      { return ( _socket_signals & sig_except ) != 0; }
*/

//---------------------------------------------Buffered_socket_operation::Buffered_socket_operation

Buffered_socket_operation::Buffered_socket_operation( SOCKET socket )
: 
    Socket_operation( socket ),
    _zero_(this+1),
    _socket_send_buffer_size( default_buffer_size )
{
}

//-----------------------------------------------------------------Buffered_socket_operation::close

void Buffered_socket_operation::close()
{
    _send_progress = 0;
    _send_data = "";
    _recv_data = "";
    _state = s_initial;

    Socket_operation::close();
}

//------------------------------------------------------------Buffered_socket_operation::state_name

string Buffered_socket_operation::state_name( State state )
{
    switch( state )
    {
        case s_initial:     return "initial";
        case s_connecting:  return "connecting";
        case s_ready:       return "ready";
        case s_sending:     return "sending";
        case s_receiving:   return "receiving";
        default: return S() << "State(" << (int)state << ")";
    }
}

//---------------------------------------------------Buffered_socket_operation::assert_no_recv_data

void Buffered_socket_operation::assert_no_recv_data()
{
    //2005-10-25 wegen endloser read-Signalisierung zwischen Firefox und Linux-Scheduler (F5 im Protokollfenster)

#   ifndef Z_WINDOWS

        State saved_state = _state;

        char buffer[1];
        int length = call_recv( buffer, sizeof buffer );                    // Setzt _eof
        _state = saved_state;
        if( length > 0 )  throw_xc( "Z-SOCKET-002", obj_name() );


#   endif
}

//---------------------------------------------------------Buffered_socket_operation::check_for_eof

bool Buffered_socket_operation::check_for_eof()
{
    if( !_eof  &&  _read_socket != STDIN_FILENO )  // read() kennt nicht MSG_PEEK
    {
        Z_LOG2( "socket.recv", "recv(" << _read_socket << ",MSG_PEEK)  checking for eof\n" );
           
        char byte;
        int  len = recv( _read_socket, &byte, 1, MSG_PEEK );

        if( len <= 0 )  
        { 
            if( len < 0 )
            {
                int err = socket_errno();
                if( err != Z_EWOULDBLOCK )  throw_socket( err, "recv", _peer_host_and_port.as_string().c_str() ); 
            }
            else
                _eof = true;  
        }
    }

    return _eof;
}

//-------------------------------------------------------Buffered_socket_operation::async_continue_

bool Buffered_socket_operation::async_continue_( Continue_flags )
{
    async_clear_signaled();

    bool something_done = false;

    switch( _state )
    {
        case s_initial:     break;

        case s_connecting:  something_done |= connect__continue();  
                            break;

        case s_ready:       assert_no_recv_data();
                            break;

        case s_sending:     something_done |= send__continue();  
                            assert_no_recv_data();
                            break;

        case s_receiving:   something_done |= recv__continue();
                            //if( async_parent() )  async_parent()->set_async_next_gmtime( 0 );        // Mutter-Operation fortsetzen!
                            break;

        default:            break;
    }


    return something_done;
}

//-----------------------------------------------------Buffered_socket_operation::async_state_text_

string Buffered_socket_operation::async_state_text_() const
{
    return "TCP " + Socket_operation::async_state_text_() + " (" + state_name() + ")";
}

//--------------------------------------------------------Buffered_socket_operation::connect__start

void Buffered_socket_operation::connect__start( const Host_and_port& host_and_port )
{
    if( _read_socket != SOCKET_ERROR  ||  _write_socket != SOCKET_ERROR )  throw_xc( "Buffered_socket_operation::connect__start" );

    _write_socket = _read_socket = socket( AF_INET, SOCK_STREAM, 0 );
    if( _write_socket == SOCKET_ERROR )   throw_socket( socket_errno(), "socket" );

    set_socket_not_inheritable( _read_socket );
  //set_socket_not_inheritable( _write_socket );

    if( !_blocking )  call_ioctl( FIONBIO, 1 );   // Non blocking


    struct sockaddr_in  peer_addr;

    memset( &peer_addr, 0, sizeof peer_addr );

    peer_addr.sin_family      = AF_INET;
    peer_addr.sin_addr.s_addr = inet_addr( host_and_port._host.ip_string().c_str() );
    peer_addr.sin_port        = htons( host_and_port._port );

    _peer_host_and_port = peer_addr;
    set_socket_event_name( "connecting" );

    Z_LOG2( "socket.connect", "connect(" << _write_socket << ',' << host_and_port << ") ...\n" );

    int ret = ::connect( _write_socket, (sockaddr*)&peer_addr, sizeof peer_addr );
    if( ret == SOCKET_ERROR ) 
    {
        int err = socket_errno();
        
        if( err != Z_EWOULDBLOCK      // Windows
         && err != Z_EINPROGRESS )    // Linux
        {
            throw_socket( err, "connect", _peer_host_and_port.as_string().c_str() );
        }

        //#ifdef Z_HPUX
        //    Z_LOG2( "socket.connect", "connect(" << _write_socket << ',' << host_and_port << ") ==> " << strerror( err ) << "\n" );
        //#endif

        socket_expect_signals( sig_write );
        _socket_manager->set_fd( Socket_manager::write_fd, _write_socket );

        set_state( s_connecting );
        return;
    }

    // Diese Stelle scheint nur unter HP-UX und AIX durchlaufen zu werden:

    Z_LOG2( "socket.connect", "connect(" << _write_socket << ',' << host_and_port << ") connected!  (" << Z_FUNCTION << ")\n" );

    socket_expect_signals( sig_read | sig_write | sig_except );
    set_socket_event_name( "connected" );
    set_state( s_ready );
    //if( async_parent() )  async_parent->async_on_signal_from_child( this );
}

//-------------------------------------------------Buffered_socket_operation::connect__continue

bool Buffered_socket_operation::connect__continue()
{
    int connect_errno = 0;
    sockaddrlen_t s = sizeof connect_errno;
    int err = getsockopt( _write_socket, SOL_SOCKET, SO_ERROR, (char*)&connect_errno, &s );
    if( err )  throw_socket( socket_errno(), "getsockopt" ); 


    struct sockaddr_in  peer_addr = _peer_host_and_port.as_sockaddr_in();

    Z_LOG2( "socket.connect", "connect(" << _write_socket << ',' << _peer_host_and_port << ") ...\n" );
    int ret = connect( _write_socket, (sockaddr*)&peer_addr, sizeof peer_addr );
    if( ret == -1 )
    {
        if( connect_errno )  throw_socket( connect_errno, "connect", _peer_host_and_port.as_string().c_str() );

        int err = socket_errno();

        if( err == Z_EINPROGRESS      // Linux
         || err == Z_EALREADY    )    // Windows
        {
            socket_expect_signals( sig_write );
            _socket_manager->set_fd( Socket_manager::write_fd, _write_socket );
            return false;
        }

        if( err != Z_EISCONN )  throw_socket( err, "connect", _peer_host_and_port.as_string().c_str() );
    }


    Z_LOG2( "socket.connect", "connect(" << _write_socket << ',' << _peer_host_and_port << "): connected!\n" );

    socket_expect_signals( sig_read | sig_write | sig_except ); // Siehe auch connect__start() (könnte zusammengefasst werden)

    set_socket_event_name( "connected" );
    set_state( s_ready );
    //if( async_parent() )  async_parent->async_on_signal_from_child( this );

    return true;
}

//-------------------------------------------------------Buffered_socket_operation::set_buffer_size

void Buffered_socket_operation::set_buffer_size()
{
    sockaddrlen_t s = sizeof _socket_send_buffer_size;
    int ret = getsockopt( _write_socket, SOL_SOCKET, SO_SNDBUF, (char*)&_socket_send_buffer_size , &s );
    if( ret == SOCKET_ERROR  ||  _socket_send_buffer_size <= 0 ) 
    {
        Z_LOG( "getsockopt(,,SO_SNDBUF)  errno=" << socket_errno() << "\n" );
        _socket_send_buffer_size = default_buffer_size;
    }
}

//-----------------------------------------------------------Buffered_socket_operation::send__start

bool Buffered_socket_operation::send__start( const string& data )
{ 
    _send_data += data;  
    _state = s_sending;  
    
    return send__continue(); 
}

//--------------------------------------------------------Buffered_socket_operation::send__continue

bool Buffered_socket_operation::send__continue()
{
    bool something_done = false;

    while(1)
    {
        int count = _send_data.length() - _send_progress; 
        if( count <= 0 )  break;

        int c   = min( _socket_send_buffer_size, count );
        int err = 0;

        int len = _write_socket == STDOUT_FILENO? write ( _write_socket, _send_data.data() + _send_progress, c )
                                                : ::send( _write_socket, _send_data.data() + _send_progress, c, 0 );
        err = len < 0? socket_errno() : 0;
        Z_LOG2( "socket.send", "send/write(" << _write_socket << "," << c << " bytes) ==> " << len << "  errno=" << err << "\n" );
        if( len > 0 )  if( z::Log_ptr log = "socket.data" )  log << "send/write ", log->write( _send_data.data() + _send_progress, len ), *log << endl;
            

        if( len == 0 )  break;   // Vorsichtshalber
        if( len < 0 ) 
        {
            if( err == Z_EWOULDBLOCK )  break;
            else
                throw_socket( err, "send", _peer_host_and_port.as_string().c_str() );
        }
        else 
        {
            _send_progress += len;
            _send_total_byte_count += len;
        }

        something_done = true;
    }


    if( _send_progress < _send_data.length() )
    {
        _socket_manager->set_fd( Socket_manager::write_fd, _write_socket );
    }
    else
    {
        _socket_manager->clear_fd( Socket_manager::write_fd, _write_socket );

        _send_data = "";
        _send_progress = 0;
        _state = s_ready;
        _response_count++;
        //if( async_parent() )  async_parent->async_on_signal_from_child( this );
    }
        
    return something_done;
}

//---------------------------------------------------------Buffered_socket_operation::recv_continue

int Buffered_socket_operation::call_recv( char* buffer, int size )
{
    Z_LOG2( "socket.recv", "recv/read(" << _read_socket << ")\n" );
    int len = _read_socket == STDIN_FILENO? read( _read_socket, buffer, size )
                                          : recv( _read_socket, buffer, size, 0 );

    if( len <= 0 ) 
    {
        if( len < 0 )  
        { 
            int err = socket_errno();
            if( err != Z_EWOULDBLOCK )  throw_socket( err, "recv", _peer_host_and_port.as_string().c_str() ); 

            _state = s_receiving;
            return 0; 
        }
           
        _eof = true;  
    }
    else
    {
        if( z::Log_ptr log = "socket.data" )  *log << "recv/read: ", log->write( buffer, len ), *log << endl;

        _recv_total_byte_count += len;
        _state = s_ready;
        //if( async_parent() )  async_parent->async_on_signal_from_child( this );
    }

    return len;
}

//------------------------------------------------Buffered_socket_operation::is_received_data_ready

//bool Buffered_socket_operation::is_received_data_ready()
//{
//
//}

//-----------------------------------------------------------Buffered_socket_operation::recv__start

bool Buffered_socket_operation::recv__start()
{
    return recv__continue( );
}

//--------------------------------------------------------Buffered_socket_operation::recv__continue

bool Buffered_socket_operation::recv__continue()
{
    bool something_done = false;
    char buffer [ default_buffer_size ];


    while(1)
    {
        int length = call_recv( buffer, sizeof buffer );

        if( length == 0  &&  !_eof )
        {
            _socket_manager->set_fd( Socket_manager::read_fd, _read_socket );
        }
        else
        {
            something_done = true;
        }

        if( _recv_data.length() + length > _recv_data.capacity() )  _recv_data.reserve( 2*_recv_data.capacity() + default_buffer_size );
        _recv_data.append( buffer, length );

        if( length < sizeof buffer )  break;
    }

    if( something_done ) 
    {
        _state = s_ready;
        //if( async_parent() )  async_parent->async_on_signal_from_child( this );
    }

    return something_done;
}

//---------------------------------------------------------------------------------------add_filenr

//void Socket_wait::add_filenr( int file_nr, Read_or_write rw )
//{
//#   ifdef Z_WINDOWS
//#       error "In Windows werden die Dateinummern sehr groß, sie beginnen nicht bei 0. Also kein _event_array verwenden!"
//#   else
//        if( _event_array.size() - 1 < filenr )  _event_array.resize( filenr < FD_SETSIZE? FD_SETSIZE : filenr + 1000 );
//#   endif
//    _event_array[ filenr ] = e;
//    FD_SET( file_nr, rw == readfd? &_fds[read_fd] : &_fds[write_fd] );
//}

//-------------------------------------------------------------------------Socket_wait::Socket_wait
#ifndef Z_WINDOWS

Socket_wait::Socket_wait( Socket_manager* m )//, int n, const fd_set& readfds, const fd_set& writefds, const fd_set& exceptfds )
: 
    _zero_(this+1),
    _socket_manager(m),
  //_n(n),
    _polling_interval( 1.0 )
{
    //memcpy( &_fds[readfd  ], &readfds  , sizeof readfds   );
    //memcpy( &_fds[writefd ], &writefds , sizeof writefds  );
    //memcpy( &_fds[exceptfd], &exceptfds, sizeof exceptfds );
}

//---------------------------------------------------------------------------------Socket_wait::add

//void Socket_wait::add( int file_nr, Read_or_write rw, Has_set_signaled* s )
//{
//#   ifdef Z_WINDOWS
//#       error "In Windows werden die Dateinummern sehr groß, sie beginnen nicht bei 0. Also kein _event_array verwenden!"
//#   endif
//    if( _signaled_array.size() - 1 < file_nr )  _signaled_array.resize( file_nr < FD_SETSIZE? FD_SETSIZE : file_nr + 1000 );
//    _signaled_array[ file_nr ] = s;
//    FD_SET( file_nr, &_fds[rw] );
//}

//---------------------------------------------------------------------------------Socket_wait::add

void Socket_wait::add( Event_base* event )
{
//#   ifdef Z_WINDOWS
//#       error "In Windows werden die Dateinummern sehr groß, sie beginnen nicht bei 0. Also kein _event_array verwenden!"
//#   endif
//    if( _signaled_array.size() - 1 < file_nr )  _signaled_array.resize( file_nr < FD_SETSIZE? FD_SETSIZE : file_nr + 1000 );
//    _signaled_array[ file_nr ] = s;
//    FD_SET( file_nr, &_fds[rw] );

    _event_list.push_back( event );
}

//--------------------------------------------------------------------------------Socket_wait::wait

int Socket_wait::wait( double seconds )
{
    bool   polling = !_event_list.empty();
    
    // In Linux wird exceptfd nicht gesetzt, wenn bei socketpair die Verbindung abbricht. 
    // Also liefern wir bei Timeout eine 1, um com_remote.cxx die Verbindung prüfen zu lassen.
    // Nicht effizient, aber so funktioniert es. Joacim 12.11.03
    //bool   simulate_except = false;
    //if( seconds > 0 )  for( int i = 0; i < FD_SETSIZE; i++ )  if( FD_ISSET( i, &_fds[exceptfd] ) )  { simulate_except = true;  polling = true; }

    double now   = double_from_gmtime();
    double until = polling? now + seconds : 0.0;


    while(1)
    {
        double wait_seconds = polling? min( _polling_interval, max( 0.0, until - now ) ) 
                                     : seconds;  

        int ret = _socket_manager->wait( wait_seconds );

        if( ret > 0 )  return ret;

        //if( simulate_except )  return 1;


        // Timeout
        if( !polling )  return 0;


        // Das war für Spooler::Directory_watcher gedacht, ist aber inzwischen in Spooler::Job selbst realisiert
        // (über _directory_watcher_next_time). 
        // Denn diese Stelle wirkt nur, wenn der Scheduler sekundenlang nichts zu tun hat.
        // Am besten sei Directory_watcher eine Async_operation. Joacim 31.12.2003

        Z_FOR_EACH( Event_list, _event_list, e )
        {
            //Z_LOG2( "zschimmer", Z_FUNCTION << " " << **e << "->signaled()?\n" );
            if( (*e)->signaled() )
            { 
                Z_LOG2( "object_server.wait", **e << " signaled!\n" );  
                return 1; 
            }
        }

        now = double_from_gmtime();
        if( now - until > -0.000001 )  return 0;
        //if( now > until )  return 0;
    }
}

#endif

//-------------------------------------------------------------------Socket_manager::Socket_manager

Socket_manager::Socket_manager()
:
    _zero_(this+1)
{
#   ifdef Z_WINDOWS
        WSADATA wsa_data;
        int ret = WSAStartup( 0x0101, &wsa_data );
        if( ret )  throw_socket( ret, "WSAStartup" );

        _wsastartup_called = true;
#   endif
}

//-------------------------------------------------------------------ocket_manager::~Socket_manager

Socket_manager::~Socket_manager()
{
#   ifdef Z_WINDOWS
        if( _wsastartup_called )
        {
            WSACleanup();
            _wsastartup_called = false;
        }
#   endif
}

//-------------------------------------------------------------Socket_manager::add_socket_operation

void Socket_manager::add_socket_operation( Socket_operation* op )
{
    add_event_operation( op );

    //if( op->_write_socket != SOCKET_ERROR )  _socket_operation_map[ op->_write_socket ] = op;
    //if( op->_read_socket  != SOCKET_ERROR )  _socket_operation_map[ op->_read_socket  ] = op;
}

//----------------------------------------------------------Socket_manager::remove_socket_operation

void Socket_manager::remove_socket_operation( Socket_operation* op )
{
    //#ifdef Z_HPUX
    //    Z_LOG2( "socket", Z_FUNCTION << " " << op->_write_socket << " " << op->_read_socket << "\n" );
    //#endif

    if( op->_write_socket != SOCKET_ERROR  &&  op->_write_socket != op->_read_socket )
    {
        clear_fd( read_fd  , op->_write_socket );
        clear_fd( write_fd , op->_write_socket );
        clear_fd( except_fd, op->_write_socket );

        clear_socket_signaled( op->_write_socket );

        //Socket_operation_map::iterator it = _socket_operation_map.find( op->_write_socket );
        //if( it != _socket_operation_map.end() )  _socket_operation_map.erase( it );
    }

    if( op->_read_socket != SOCKET_ERROR )
    {
        clear_fd( read_fd  , op->_read_socket );
        clear_fd( write_fd , op->_read_socket );
        clear_fd( except_fd, op->_read_socket );

        clear_socket_signaled( op->_read_socket );

        //Socket_operation_map::iterator it = _socket_operation_map.find( op->_read_socket );
        //if( it != _socket_operation_map.end() )  _socket_operation_map.erase( it );
    }

    remove_event_operation( op );
}

//---------------------------------------------------------------------------Socket_manager::set_fd
#ifndef Z_WINDOWS

void Socket_manager::set_fd( Read_or_write rw, SOCKET s )
{ 
    //Z_LOG2( "socket", Z_FUNCTION << ( rw == read_fd? " read=" : rw == write_fd? " write=" : " except=" ) << s << "\n" );

    if( s >= FD_SETSIZE )  z::throw_xc( "Z-ASYNC-SOCKET-001", s, FD_SETSIZE, Z_FUNCTION );
    FD_SET( s, &_fds[rw] );  if( _n < s+1 )  _n = s+1; 
}

//-------------------------------------------------------------------------Socket_manager::clear_fd

void Socket_manager::clear_fd( Read_or_write rw, SOCKET s )
{ 
    //Z_LOG2( "socket", Z_FUNCTION << ( rw == read_fd? " read=" : rw == write_fd? " write=" : " except=" ) << s << "\n" );

    if( s >= FD_SETSIZE )  z::throw_xc( "Z-ASYNC-SOCKET-001", s, FD_SETSIZE, Z_FUNCTION );
    FD_CLR( s, &_fds[rw] ); 
}

//----------------------------------------------------------------------Socket_manager::create_wait

ptr<Socket_wait> Socket_manager::create_wait()
{
  //return Z_NEW( Socket_wait( this, _n, _fds[read_fd], _fds[write_fd], _fds[except_fd] ) );
    return Z_NEW( Socket_wait( this ) );
}

#endif
//---------------------------------------------------------------Socket_manager::set_socket_signals
/*
void Socket_manager::set_socket_signals( SOCKET s, Socket_operation::Signals signals )
{
    //if( signals & Socket_operation::sig_read   )  FD_CLR( s, &_fds[read_fd]   );
    //if( signals & Socket_operation::sig_write  )  FD_CLR( s, &_fds[write_fd]  );
    //if( signals & Socket_operation::sig_except )  FD_CLR( s, &_fds[except_fd] );

    Socket_operation_map::iterator it = _socket_operation_map.find( s );
    if( it != _socket_operation_map.end() )  it->second->set_socket_signals( signals );  
}
*/
//------------------------------------------------------------------------Event_manager::get_events

void Event_manager::get_events( vector<Socket_event*>* events )
{
    events->resize( _event_operation_list.size() );

    int n = 0;

    Z_FOR_EACH( Event_operation_list, _event_operation_list, s )
    {
        (*events)[ n++ ] = (*s)->async_event();
    }
}

//-----------------------------------------------------------Event_manager::async_continue_selected

bool Event_manager::async_continue_selected( Operation_is_ok* operation_is_ok, double sleep_seconds )
{
    bool something_done = false;


    if( sleep_seconds <= 0.0 )
    {
        something_done |= async_continue_selected_( operation_is_ok );
    }
    else
    {
        double until = double_from_gmtime() + sleep_seconds;

        while(1)
        {
            bool short_sleep     = false;
            bool something_done2;
            
            do
            {
                if( double_from_gmtime() >= until )  goto TIMEOUT;

                something_done2 = false;
                _event_operation_list_modified = false;

                Z_FOR_EACH( Event_operation_list, _event_operation_list, o )
                {
                    Event_operation* op = *o;

                    if( ( !operation_is_ok || (*operation_is_ok)( op ) ) )  // &&  op->async_signaled() )  
                    {
                        something_done2 |= op->async_continue();

                        something_done |= something_done2;
                        if( _event_operation_list_modified )  break;    // iterator ist ungültig.
                    }
                }

                short_sleep |= something_done2;
            }
            while( something_done2  ||  _event_operation_list_modified );

            something_done |= Async_manager::async_continue_selected( operation_is_ok );

            sleep( short_sleep? 0.1 : 1.0 );
            // Besser sollte hier mit select() (bzw. MsgWaitForMultipleObjects()) gewartet werden. Ist aber zu aufwendig.
            // Deshalb ist oben der Aufruf von async_continue_selected() durch dessen Implementierung ersetzt,
            // bei der async_signaled() nicht abgefragt wird. Also werden alle Operationen ausgeführt, denn sie werden nicht signalisiert 
            // (jedenfalls nicht in Unix, denn hier fehlt das select, was die fdset-Bits setzen würde).
        }

        TIMEOUT: ;
    }

    something_done |= _event_operation_list_modified;

    return something_done;
}

//----------------------------------------------------------Event_manager::async_continue_selected_

bool Event_manager::async_continue_selected_( Operation_is_ok* operation_is_ok )
{
    bool something_done = false;



#   ifndef Z_WINDOWS
        // Gilt nicht, wenn ein _socket_event gesetzt ist.  if( _socket_signaled_count == 0 )  return false;
#   endif

    do
    {
        _event_operation_list_modified = false;

        Z_FOR_EACH( Event_operation_list, _event_operation_list, o )
        {
            Event_operation* op = *o;

            if( ( !operation_is_ok || (*operation_is_ok)( op ) )  &&  op->async_signaled() )  
            {
                something_done |= op->async_continue( Async_operation::cont_signaled );

#               ifndef Z_WINDOWS
                    // Gilt nicht, wenn ein _socket_event gesetzt ist.  if( --_socket_signaled_count == 0 )  break;
#               endif
            }

            if( _event_operation_list_modified )  break;    // iterator ist ungültig.
        }
    }
    while( _event_operation_list_modified );


    something_done |= _event_operation_list_modified;
    something_done |= Async_manager::async_continue_selected( operation_is_ok );


    return something_done;
}

//------------------------------------------------------------------------------Event_manager::wait
#ifdef Z_WINDOWS

int Event_manager::wait( double wait_seconds )
{
    //assert( wait_seconds == 0 );        // Mehr ist nicht implementiert.

    bool   again = false;
    double now   = wait_seconds <= 0? 0 : double_from_gmtime();
    double until = min( now + wait_seconds, (double)time_max );

    while(1)
    {
        double wait_time = until - now;
        int    sleep_time_ms = INT_MAX;
        int    t = (int)ceil( min( (double)sleep_time_ms, wait_time * 1000.0 ) );


        if( t <= 0 && again )  return 0;
                         else  t = 0;
/*        
        if( t > 0  &&  log_category_is_set( "wait" ) )
        {
            string msg = "MsgWaitForMultipleObjects " + sos::as_string(t/1000.0) + "s (bis " + until.as_string() + ")  ";
            for( int i = 0; i < _handles.size(); i++ )
            {
                if( i > 0 )  msg += ", ";
                if( _events[i] )  msg += _events[i]->as_text() + " (0x" + as_hex_string( (int)_handles[i] ) + ")";
                           else   msg += "NULL";
            }
            LOG( msg << "\n" );
        }
*/
        HANDLE* handles = new HANDLE [ _event_operation_list.size() ];

        int n = 0;
        Z_FOR_EACH( Event_operation_list, _event_operation_list, o )  handles[ n++ ] = *(*o)->async_event();

        DWORD ret = MsgWaitForMultipleObjects( n, handles, FALSE, t, 0 ); //QS_ALLINPUT ); 
        
        delete [] handles;  handles = NULL;

        if( ret == WAIT_FAILED )  throw_mswin( "MsgWaitForMultipleObjects" );

        if( ret >= WAIT_OBJECT_0  &&  ret < WAIT_OBJECT_0 + n )
        {
            int             index = ret - WAIT_OBJECT_0;
            int             i     = 0;
            z::Event_base*  event = NULL;

            Z_FOR_EACH( Event_operation_list, _event_operation_list, o )
            {
                if( i == index ) 
                { 
                    event = (*o)->async_event(); 
                    break; 
                }

                i++;
            }
        
            event->set_signaled( "Event_manager::wait" );

            return 1;
        }
        else
        if( ret == WAIT_OBJECT_0 + _event_operation_list.size() )
        {
            // QS_ALLINPUT setzen!
            //windows_message_step();
        }
        else
        if( ret == WAIT_TIMEOUT )  
        {
            if( wait_time <= 0.0 )  return 0;
            now = double_from_gmtime();
            again = true;
        }
        else
            throw_mswin( "MsgWaitForMultipleObjects" );
    }
}

#endif
//-----------------------------------------------------------------------------Socket_manager::wait
#ifdef Z_WINDOWS

int Socket_manager::wait( double wait_seconds )
{
    return Event_manager::wait( wait_seconds );
}

#endif
//-----------------------------------------------------------------------------Socket_manager::wait
#ifndef Z_WINDOWS

int Socket_manager::wait( double wait_seconds )
{
    {
        int element_size = sizeof _fds[read_fd  ].fds_bits[0];
        int bytes = ( ( _n + 7 ) / 8 + element_size - 1 ) / element_size * element_size;
        assert( bytes <= sizeof _fds[read_fd  ].fds_bits );
        memcpy( &_signaled_fds[read_fd  ].fds_bits, &_fds[read_fd  ].fds_bits, bytes );
        memcpy( &_signaled_fds[write_fd ].fds_bits, &_fds[write_fd ].fds_bits, bytes );;
    }

    //memcpy( &_signaled_fds[read_fd  ], &_fds[read_fd  ], sizeof (fd_set) );
    //memcpy( &_signaled_fds[write_fd ], &_fds[write_fd ], sizeof (fd_set) );
  //memcpy( &_signaled_fds[except_fd], &_fds[except_fd], sizeof (fd_set) );     // Wir scheinen ohne except_fd auszukommen. 7.1.04

    if( wait_seconds >= 0.000001 )  wait_seconds += 0.000002;     // Etwas länger, weil in Linux sonst select() zu kurz wartet (999ms statt 1s)
    timeval t = timeval_from_seconds( wait_seconds );

    if( Log_ptr log = "socket.select" )
    {
        log << "Socket_manager::wait: select(" << _n << ",";
        print_fd_sets( log, &_signaled_fds[read_fd], &_signaled_fds[write_fd], &_signaled_fds[except_fd], _n );
        log << ',' << ( (t.tv_sec*1000) + (t.tv_usec/1000) ) << printf_string( ".%03d", t.tv_usec%1000) << "ms)\n";
    }

    _socket_signaled_count = 0;

    int ret = select( _n, &_signaled_fds[read_fd], &_signaled_fds[write_fd], NULL, &t );

    int err = ret == -1? errno : 0;

    if( ret != 0  &&  err != EINTR )
    {
        if( Log_ptr log = "socket.select" )
        {
            log << "Socket_manager::wait: select() ret=" << ret;
            if( ret == -1 )  log << " errno=" << err << " " << z_strerror(err);
            if( ret > 0 )  log << "  ", print_fd_sets( log, &_signaled_fds[read_fd], &_signaled_fds[write_fd], &_signaled_fds[except_fd], _n );
            log << '\n';
        }
    }

    if( ret == -1 ) 
    {
        if( err == EINTR )  return 0;
        throw_socket( err, "select" );
    }
    else
    if( ret > 0 )
    {
        _socket_signaled_count = ret;

        //int n = 0;

        //for( int i = 0; i < FD_SETSIZE; i++ )
        //{
        //    Socket_operation::Signals signals = Socket_operation::sig_none;

        //    if( FD_ISSET( i, &fds[read_fd  ] ) != 0 )  signals = (Socket_operation::Signals)( signals | Socket_operation::sig_read   );
        //    if( FD_ISSET( i, &fds[write_fd ] ) != 0 )  signals = (Socket_operation::Signals)( signals | Socket_operation::sig_write  );
        //    if( FD_ISSET( i, &fds[except_fd] ) != 0 )  signals = (Socket_operation::Signals)( signals | Socket_operation::sig_except );

        //    if( signals )
        //    {
        //        _socket_manager->set_socket_signals( i, signals );
        //        if( ++n == ret )  break;
        //    }
        //}
    }

    return ret;
}

#endif
//------------------------------------------------------------------Socket_manager::socket_signaled
#ifndef Z_WINDOWS

bool Socket_manager::socket_signaled( SOCKET s )
{ 
    if( s >= FD_SETSIZE )  z::throw_xc( "Z-ASYNC-SOCKET-001", s, FD_SETSIZE, Z_FUNCTION );

    return FD_ISSET( s, &_signaled_fds[read_fd  ] ) ||
           FD_ISSET( s, &_signaled_fds[write_fd ] ) ||
           FD_ISSET( s, &_signaled_fds[except_fd] );   
}

#endif
//------------------------------------------------------------Socket_manager::clear_socket_signaled
#ifndef Z_WINDOWS

void Socket_manager::clear_socket_signaled( SOCKET s )
{ 
    if( s >= FD_SETSIZE )  z::throw_xc( "Z-ASYNC-SOCKET-001", s, FD_SETSIZE, Z_FUNCTION );

    FD_CLR( s, &_signaled_fds[read_fd] );
    FD_CLR( s, &_signaled_fds[write_fd] );
    FD_CLR( s, &_signaled_fds[except_fd] ); 
}

#endif
//-----------------------------------------------------------Socket_manager::string_from_operations

string Socket_manager::string_from_operations( const string& separator )
{
    S result;

    #ifdef Z_UNIX
        result << "Socket_manager(";
        print_fd_sets( &result, &_fds[read_fd], &_fds[write_fd], NULL, _n );
        result << ")" << separator;
    #endif

    result << Event_manager::string_from_operations( separator );

    return result;
}

//-------------------------------------------------------------------Socket_manager::bind_free_port
// Used_ports: Es gibt unter Windows interne Firewalls, die Verbindungsaufbau zu einem Port
// verweigern, der bereits für eine andere Verbindung belegt ist, obwohl bind() und listen() möglich waren.
// Zone Labs Integrity Agent Version 5.1.556.199, TrueVector security engine version 5.1.556.199, Driver version 5.1.556.199 

// Das ist keine zuverlässige Implementierung, denn ein Port kann von einer anderen Anwendung in Gebrauch sein,
// was wir hier nicht bemerken würden.

int Socket_manager::Used_ports::bind_free_port( SOCKET s, sockaddr_in* address )
{
    int errn = EINVAL;

    for( int port = address->sin_port? ntohs( address->sin_port ) : 59999; port > 1024; port-- )
    {
        if( _used_ports.find( port ) == _used_ports.end() )
        {
            address->sin_port = htons( port );

            int error = ::bind( s, (sockaddr*)address, sizeof *address );
            errn = error? socket_errno() : 0;

            if( Log_ptr log = "socket.bind" )
            {
                *log << "bind(" << s << "," << Host_and_port( *address ) << ")";
                if( error )  *log << "  ERRNO-" << errn;
                       else  *log << "  OK";
                *log << "\n" << flush;
            }
            
            if( !error )  
            {
                register_port( port );
                break;
            }
        }
    }

    if( errn )  address->sin_port = 0;

    return errn;
}

//-----------------------------------------------------------------------Socket_stream::connect_tcp

void Socket_stream::connect_tcp( const string& address )
{
    set_blocking( true );
    connect__start( address );
    async_finish();
}

//-----------------------------------------------------------------------Socket_stream::write_bytes

void Socket_stream::write_bytes( const io::Byte_sequence& bytes )
{
    send__start( string( (const char*)bytes.ptr(), bytes.length() ) );
    async_finish();
}

//------------------------------------------------------------------------Socket_stream::read_bytes

string Socket_stream::read_bytes( size_t maximum )
{
    char buffer [ read_bytes_maximum ];
    int length = call_recv( buffer, min( maximum, NO_OF( buffer ) ) );
    return string( buffer, length );
}

//-----------------------------------------------------------------------------Socket_stream::flush

void Socket_stream::flush() 
{
}

//------------------------------------------------------Socket_async_output_stream::try_write_bytes

//int Socket_async_output_stream::try_write_bytes( const io::Byte_sequence& s )
//{
//    const Byte* p     = s.ptr();
//    const Byte* p_end = s.ptr() + s.length();
//
//    while( p < p_end )
//    {
//        int c   = min( p_end - p, _socket_send_buffer_size );
//        int err = 0;
//
//        int len = _write_socket == STDOUT_FILENO? write ( _write_socket, p, c )
//                                                : ::send( _write_socket, p, c, 0 );
//        err = len < 0? socket_errno() : 0;
//        Z_LOG2( "socket.send", "send/write(" << _write_socket << "," << c << " bytes) ==> " << len << "  errno=" << err << "\n" );
//
//        if( len < 0 ) 
//        {
//            if( err == Z_EWOULDBLOCK )  break;
//            throw_socket( err, "send", _peer_host_and_port.as_string().c_str() );
//        }
//        else 
//        {
//            p += len;
//            _send_total_byte_count += len;
//        }
//    }
//
//    //if( p < p_end )  _socket_manager->set_fd  ( Socket_manager::write_fd, _write_socket );
//    //           else  
//                   _socket_manager->clear_fd( Socket_manager::write_fd, _write_socket );
//
//    return p_end - p;
//}

//-------------------------------------------------Socket_async_output_stream::request_notification

//void Socket_async_output_stream::request_notification()
//{
//    _socket_manager->set_fd( Socket_manager::write_fd, _write_socket );
//}

//----------------------------------------------------------------Socket_async_output_stream::close

void Socket_async_output_stream::close()
{
    //shutdown( _write_socket, SHUT_WR );
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
