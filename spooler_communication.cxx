// $Id: spooler_communication.cxx,v 1.6 2001/01/10 12:43:24 jz Exp $

//#include <precomp.h>

#include <ctype.h>
#include "../kram/sos.h"
#include "../kram/sleep.h"
#include "spooler.h"


using namespace std;

namespace sos {
namespace spooler {

//---------------------------------------------------------------------------------------get_errno

int get_errno() 
{
#   ifdef SYSTEM_WIN
        return WSAGetLastError();
#    else
        return errno;
#   endif
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
                        if( _open_elements == 0 )  _xml_is_complete = true;
                    }
                    else
                    if( !_in_special_tag )
                    {
                        if( _last_char != '/' )  _open_elements++;
                        else 
                        if( _open_elements == 0 )  _xml_is_complete = true;      // Das Dokument ist nur ein leeres XML-Element
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
            }
        }

        _last_char = *p;
        p++;
    }

    return _xml_is_complete;
}

//------------------------------------------------------------------Communication::Channel::Channel

Communication::Channel::Channel()
:
    _zero_(this+1),
    _socket( SOCKET_ERROR ),
    _send_is_complete( true )
{
}

//-----------------------------------------------------------------Communication::Channel::~Channel

Communication::Channel::~Channel()
{
    if( _socket != SOCKET_ERROR )  closesocket( _socket );
}

//----------------------------------------------------------------Communication::Channel::do_accept

void Communication::Channel::do_accept( SOCKET listen_socket )
{
    int peer_addr_len = sizeof _peer_addr;

    _socket = accept( listen_socket, (struct sockaddr*)&_peer_addr, &peer_addr_len );
    if( _socket == SOCKET_ERROR )  throw_sos_socket_error( "accept" );

    struct linger l; l.l_onoff=0; l.l_linger=0;
    setsockopt( _socket, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof l );
}

//-----------------------------------------------------------------Communication::Channel::do_close

void Communication::Channel::do_close()
{
    closesocket( _socket );
    _socket = SOCKET_ERROR;
}

//------------------------------------------------------------------Communication::Channel::do_recv

void Communication::Channel::do_recv()
{
    char buffer [ 4096 ];

    if( _receive_is_complete )         // Neuer Anfang?
    {
        _receive_at_start = true; 
        _receive_is_complete = false;
        _text = "";
        _xml_end_finder = Xml_end_finder();
    }


    int len = recv( _socket, buffer, sizeof buffer, 0 );
    if( len <= 0 ) {
        if( len == 0 )  { _eof = true;  return; }
        if( get_errno() == EAGAIN )  return; 
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

//------------------------------------------------------------------Communication::Channel::do_send

void Communication::Channel::do_send()
{
    if( _send_is_complete )  _send_progress = 0, _send_is_complete = false;     // Am Anfang

    int len = send( _socket, _text.c_str() + _send_progress, _text.length() - _send_progress, 0 );
    if( len < 0 ) {
        if( get_errno() == EAGAIN )  return;
        throw_sos_socket_error( "send" );
    }

    _send_progress += len;

    if( _send_progress == _text.length() )
    {
        _send_is_complete = true;
        _text = "";
    }
}

//--------------------------------------------------------------------Communication::Communication

Communication::Communication( Spooler* spooler )
: 
    _zero_(this+1), 
    _spooler(spooler)
{
}

//--------------------------------------------------Communication::Channel::~Communication::Channel

Communication::~Communication()
{
    {
        Thread_semaphore::Guard guard = &_semaphore;

        _channel_list.clear();
        closesocket( _listen_socket );

        _terminate = true;
    }


    if( _thread ) 
    {
        WaitForSingleObject( _thread, 1000 );
        CloseHandle( _thread );
    }
}

//-----------------------------------------------------------------------------Communication::start

void Communication::start()
{
    struct sockaddr_in  sa;
    int                 ret;
    unsigned long       on = 1;
    BOOL                true_ = 1;


#   ifdef SYSTEM_WIN
        WSADATA wsa_data;
        ret = WSAStartup( 0x0101, &wsa_data );
        if( ret )  throw_sos_socket_error( ret, "WSAStartup" );
#   endif


    // UDP:

    _udp_socket = socket( AF_INET, SOCK_DGRAM, 0 );
    if( _udp_socket == SOCKET_ERROR )  throw_sos_socket_error( "socket" );
    if( _udp_socket >= _nfds )  _nfds = _udp_socket + 1;

    setsockopt( _udp_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&true_, sizeof true_ );

    sa.sin_port        = htons( _spooler->_udp_port );
    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = 0; // INADDR_ANY

    ret = bind( _udp_socket, (struct sockaddr*)&sa, sizeof sa );
    if( ret == SOCKET_ERROR )  throw_sos_socket_error( "udp-bind" );

    ret = ioctlsocket( _udp_socket, FIONBIO, &on );
    if( ret == SOCKET_ERROR )  throw_sos_socket_error( "ioctl(FIONBIO)" );


    // TCP: 

    _listen_socket = socket( AF_INET, SOCK_STREAM, 0 );
    if( _listen_socket == SOCKET_ERROR )  throw_sos_socket_error( "socket" );
    if( _listen_socket >= _nfds )  _nfds = _listen_socket + 1;


    setsockopt( _listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&true_, sizeof true_ );

    sa.sin_port        = htons( _spooler->_tcp_port );
    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = 0; // INADDR_ANY

    ret = bind( _listen_socket, (struct sockaddr*)&sa, sizeof sa );
    if( ret == SOCKET_ERROR )  throw_sos_socket_error( "tcp-bind" );

    ret = listen( _listen_socket, 5 );
    if( ret == SOCKET_ERROR )  throw_errno( get_errno(), "listen" );

    ret = ioctlsocket( _listen_socket, FIONBIO, &on );
    if( ret == SOCKET_ERROR )  throw_sos_socket_error( "ioctl(FIONBIO)" );
}

//---------------------------------------------------------------------Communication::handle_socket

bool Communication::handle_socket( Channel* channel )
{
    try
    {
        if( FD_ISSET( channel->_socket, &_write_fds ) ) 
        {
            FD_CLR( channel->_socket, &_write_fds );

            channel->do_send();
        }

        if( FD_ISSET( channel->_socket, &_read_fds ) )
        {
            FD_CLR( channel->_socket, &_read_fds );

            channel->do_recv();
            
            if( channel->_receive_is_complete ) 
            {
                channel->_receive_is_complete = false;
                channel->_text = _spooler->_command_processor.execute( channel->_text );
                channel->do_send();
            }

            if( channel->_eof && channel->_send_is_complete )
            {
                channel->do_close();
                return false;
            }
        }
    }
    catch( const Xc& x )
    {
        _spooler->_log.error( "FEHLER bei einer Verbindung: " + x.what() );
        return false;
    }

    return true;
}

//-------------------------------------------------------------------------------Communication::run

int Communication::run()
{
    start();

    FD_ZERO( &_read_fds );      
    FD_ZERO( &_write_fds );


    while(1) 
    {
        FD_SET( _udp_socket, &_read_fds );
        FD_SET( _listen_socket, &_read_fds );

        int n = ::select( _nfds, &_read_fds, &_write_fds, NULL, NULL );
        if( n < 0 )  throw_sos_socket_error( get_errno(), "select" );

        {
            Thread_semaphore::Guard guard = &_semaphore;

            if( _terminate )  break;

            // UDP
            if( FD_ISSET( _udp_socket, &_read_fds ) )
            {
                char buffer [4096];
                int len = recv( _udp_socket, buffer, sizeof buffer, 0 );
                if( len > 0 ) 
                {
                    _spooler->_command_processor.execute( as_string( buffer, len ) );
                }
            }

            // Neue TCP-Verbindung
            if( FD_ISSET( _listen_socket, &_read_fds ) )
            {
                Sos_ptr<Channel> new_channel = SOS_NEW( Channel );
            
                new_channel->do_accept( _listen_socket );
                if( new_channel->_socket == SOCKET_ERROR )  return true;
                if( new_channel->_socket >= _nfds )  _nfds = new_channel->_socket + 1;

                _channel_list.push_back( new_channel );

                _spooler->_log.msg( "TCP-Verbindung angenommen" );
            }


            // TCP-Nachricht
            FOR_EACH( Channel_list, _channel_list, it )
            {
                Channel* channel = *it;

                bool ok = handle_socket( channel );

                if( !ok ) { it = _channel_list.erase( it );  continue; }

                if( channel->_send_is_complete    )  FD_SET( channel->_socket, &_read_fds );
                if( channel->_receive_is_complete )  FD_SET( channel->_socket, &_write_fds  );
            }
        }
    }

    closesocket( _listen_socket );

    return 0;
}

//-------------------------------------------------------------------------------------------thread

static ulong __stdcall thread( void* param )
{
    ulong result;

    try 
    {
        HRESULT hr = CoInitialize(NULL);
        if( FAILED(hr) )  throw_ole( hr, "CoInitialize" );

        result = ((Communication*)param)->run();
    }
    catch( const Xc& x )
    {
        ((Communication*)param)->_spooler->_log.msg( "Communication::thread:  " + x.what() );
        result = 1;
    }

    CoUninitialize();
    return result;
}

//----------------------------------------------------------------------Communication::start_thread

void Communication::start_thread()
{
    DWORD thread_id;

    _thread = CreateThread( NULL,                        // no security attributes 
                            0,                           // use default stack size  
                            thread,                      // thread function 
                            this,                        // argument to thread function 
                            0,                           // use default creation flags 
                            &thread_id );                // returns the thread identifier 
 
   if( !_thread )  throw_mswin_error( "CreateThread" );
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos


