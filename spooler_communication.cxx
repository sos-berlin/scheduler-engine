// $Id: spooler_communication.cxx,v 1.1 2001/01/03 22:16:23 jz Exp $

//#include <precomp.h>

#include <ctype.h>
#include "../kram/sos.h"
#include "../kram/sleep.h"
#include "spooler.h"


using namespace std;

namespace sos {
namespace spooler {


enum Tok { cdata_tok, comment_tok };
const int token_count = 2;

//----------------------------------------------------------------------------------------Tok_entry

struct Tok_entry
{
                                Tok_entry() : _index(0),_active(false) {}

    void                        reset                   ()              { _index = 0; }
    bool                        step_begin              ( char );
    void                        step_end                ( char );

    int                        _index;
    bool                       _active;
    const char*                _begin;
    const char*                _end;
};

//----------------------------------------------------------------------------Tok_entry::step_begin

bool Tok_entry::step_begin( char c )      
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

//------------------------------------------------------------------------------Tok_entry::step_end

void Tok_entry::step_end( char c )      
{ 
    if( c == _end[_index] )
    {
        _index++; 
        if( _end[_index] == '\0' )  _index = 0, _active = false;
    }
    else  
        _index = 0; 
}

//------------------------------------------------------Communication_channel::Communication_channel

Communication_channel::Communication_channel( Spooler* spooler )
: 
    _zero_(this+1), 
    _spooler(spooler)
{
}

//-----------------------------------------------------Communication_channel::~Communication_channel

Communication_channel::~Communication_channel()
{
    if( _thread )  CloseHandle( _thread );

    closesocket( _listen_socket );
    closesocket( _socket );
}

//-------------------------------------------------------------------Communication_channel::recv_xml

string Communication_channel::recv_xml()
{
    string      xml;

    bool        start           = true;     // Weiﬂe Zeichen am Anfang wegwerfen, damit's XML-konform wird.

    int         open_elements   = 0;        // Anzahl der offenen Elemente (ohne <?..?> und <!..>)
    bool        at_start_tag    = false;    // Letztes Zeichen war '<'
    bool        in_special_tag  = false;    // <?, <!
    bool        in_tag          = false;
    bool        in_end_tag      = false;
    bool        xml_complete    = false;
    char        last_char       = '\0';
    Tok_entry   tok             [token_count];
    
    tok[cdata_tok    ]._begin = "<![CDATA[";
    tok[cdata_tok    ]._end   = "]]>";
    tok[comment_tok  ]._begin = "<!--";
    tok[comment_tok  ]._end   = "-->";


    while( !xml_complete )
    {
        char buffer [ 4096 ];

        int len = recv( _socket, buffer, sizeof buffer, 0 );
        if( len < 0 )  throw_sos_socket_error( "recv" );
        if( len == 0 )  throw_eof_error();

        const char* p = buffer;

        if( start ) {
            while( p < buffer+len  &&  isspace( (Byte)*p ) )  p++;
            if( p < buffer+len )  start = false;
        }

        while( p < buffer+len )
        {
            if( *p != '\0' ) 
            {
                if( tok[cdata_tok  ]._active )  tok[cdata_tok  ].step_end( *p );
                else
                if( tok[comment_tok]._active )  tok[comment_tok].step_end( *p );
                else
                if( in_tag )
                {
                    if( at_start_tag ) 
                    {
                        if( *p == '/' )  in_end_tag = true;               // "</"
                        else
                        if( !isalpha(*p) )  in_special_tag = true;
                        at_start_tag = false;
                    }
                    else
                    if( *p == '>' ) 
                    {
                        in_tag = false;
                        if( in_end_tag ) {
                            open_elements--;
                            if( open_elements == 0 )  xml_complete = true;
                        }
                        else
                        if( !in_special_tag )
                        {
                            if( last_char != '/' )  open_elements++;
                            else 
                            if( open_elements == 0 )  xml_complete = true;      // Das Dokument ist nur ein leeres XML-Element
                        }
                        in_special_tag = false;
                    }
                }
                else
                {
                    bool in_something = false;
                    in_something |= tok[cdata_tok  ].step_begin( *p );    
                    in_something |= tok[comment_tok].step_begin( *p );

                    if( in_something )  at_start_tag = false, in_tag = false, in_special_tag = false;
                    else
                    if( *p == '<' )  in_tag = true, at_start_tag = true;
                }
            }

            last_char = *p;
            p++;
        }

        xml.append( buffer, len );
    }

    return xml;
}

//-----------------------------------------------------------------Communication_channel::send_text

void Communication_channel::send_text( const string& text )
{
    int len = send( _socket, text.c_str(), text.length(), 0 );
    if( len < text.length() )  throw_sos_socket_error( "send" );
}

//-------------------------------------------------------------------Communication_channel::execute
/*
void Communication_channel::execute()
{
    lock();
    
    try {
        ...;
    }
    catch( const Xc& x )
    {
    }

    unlock();
}
*/

//-------------------------------------------------------Communication_channel::wait_for_connection

void Communication_channel::wait_for_connection()
{
    struct sockaddr_in  peer_addr;
    int                 peer_addr_len = sizeof peer_addr;
    struct sockaddr_in  sa;
    int                 ret;

#   ifdef SYSTEM_WIN
        WSADATA wsa_data;
        ret = WSAStartup( 0x0101, &wsa_data );
        if( ret )  throw_sos_socket_error( ret, "WSAStartup" );
#   endif

    _listen_socket = socket( AF_INET, SOCK_STREAM, 0 );
    if( _listen_socket == SOCKET_ERROR )  throw_sos_socket_error( "socket" );


    BOOL true_ = 1;
    setsockopt( _listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&true_, sizeof true_ );


    sa.sin_port        = htons( 4444 );
    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = 0; /* INADDR_ANY */

    ret = bind( _listen_socket, (struct sockaddr*)&sa, sizeof sa );
    if( ret == SOCKET_ERROR )  throw_sos_socket_error( "bind" );


    ret = listen( _listen_socket, 5 );
    if( ret == SOCKET_ERROR )  throw_errno( errno, "listen" );

    _socket = accept( _listen_socket, (struct sockaddr*)&peer_addr, &peer_addr_len );
    if( _socket == SOCKET_ERROR )  throw_sos_socket_error( "accept" );

    struct linger l;
    l.l_onoff  = 0;
    l.l_linger = 0;
    setsockopt( _socket, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof l );
}

//-----------------------------------------------------------------------Communication_channel::run

int Communication_channel::run()
{
    wait_for_connection();

    while(1) 
    {
        send_text( _spooler->_command_processor.execute( recv_xml() ) );
    }

    closesocket( _socket );
    _socket = SOCKET_ERROR;

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

        result = ((Communication_channel*)param)->run();
    }
    catch( const Xc& x )
    {
        cerr << "Communication_channel::thread:  " << x << '\n';
        result = 1;
    }

    CoUninitialize();
    return result;
}

//---------------------------------------------------------------Communication_channel::start_thread

void Communication_channel::start_thread()
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


