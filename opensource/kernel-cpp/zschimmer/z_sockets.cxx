// $Id: z_sockets.cxx 14085 2010-10-13 18:18:48Z jz $

#include "zschimmer.h"
#include "log.h"
#include "z_sockets.h"


using namespace std;

//--------------------------------------------------------------------------------------------const

#ifdef Z_WINDOWS
    const extern int STDIN_FILENO  = 0;     //(int)GetStdHandle( STD_INPUT_HANDLE );    // 0;
    const extern int STDOUT_FILENO = 1;     //(int)GetStdHandle( STD_OUTPUT_HANDLE );   // 1;
#endif

//-------------------------------------------------------------------------------------------------
    
namespace zschimmer {

//--------------------------------------------------------------------------------------error_codes
    
static Message_code_text error_codes[] =
{
    { "Z-SOCKET-001", "Hostname $1 hat keine eindeutige IP-Nummer: $2" },
    { "Z-SOCKET-002", "Unerwartet Daten über TCP empfangen" },
    { NULL }
};

//--------------------------------------------------------------------------------------------const

const Ip_address Ip_address::localhost ( 127, 0, 0, 1 );

//--------------------------------------------------------------------------------------------_INIT

Z_INIT( z_sockets )
{
    add_message_code_texts( error_codes ); 
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

//-----------------------------------------------------------------------------------bind_free_port

int bind_free_port( SOCKET s, sockaddr_in* address )
{
    int errn = EINVAL;

    for( int port = address->sin_port? ntohs( address->sin_port ) : 59999; port > 1024; port-- )
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
        
        if( !error )  break;
    }

    if( errn )  address->sin_port = 0;

    return errn;
}

//-----------------------------------------------------------------------set_socket_not_inheritable

void set_socket_not_inheritable( SOCKET s )
{
#   ifdef Z_WINDOWS

        BOOL ok = SetHandleInformation( (HANDLE)s, HANDLE_FLAG_INHERIT, 0 );
        if( !ok )  throw_mswin( "SetHandleInformation", "INHERIT=0" );

#   endif
}

//----------------------------------------------------------------------------------z_gethostbyname

hostent* z_gethostbyname( const char* name )
{
    Z_LOG( "gethostbyname(\"" << name << "\")\n" );

    hostent* h = gethostbyname( name );     // In Unix nicht thread-sicher?
    if( !h )
    {
#       ifdef Z_WINDOWS
            throw_socket( socket_errno(), "gethostbyname", name );
#        else
            int error = h_errno;
            string error_code = "H_ERRNO-" + as_string( error );
            string error_text;
            switch( h_errno )
            {
                case HOST_NOT_FOUND:    error_text = "HOST_NOT_FOUND";  break;
                case TRY_AGAIN:         error_text = "TRY_AGAIN";       break;
                case NO_RECOVERY:       error_text = "NO_RECOVERY";     break;
                case NO_ADDRESS:        error_text = "NO_ADDRESS";      break;
                default: ;
            }
            throw_xc( error_code.c_str(), "gethostbyname", error_text, name );
#       endif
    }

    if( Log_ptr log = "" )
    {
        log << "gethostbyname(\"" << name << "\") ==> ";
        for( uint32** p = (uint32**)h->h_addr_list; *p; p++ )  log << Ip_address( **p ) << " ";
        for( char**   p =           h->h_aliases  ; *p; p++ )  log << *p << " ";
        log << "\n";
    }

    return h;
}

//-------------------------------------------------------------------------------------z_socketpair
/*
int z_socketpair( int domain, int type, int protocol, SOCKET sockets[2] )
{
#   ifdef Z_UNIX

        return socketpair( domain, type, protocol, sockets );

#   else

        if( domain == PF_UNIX )  domain = PF_INET;
        assert( domain == PF_INET );

        SOCKET listen_socket = SOCKET_ERROR;
        SOCKET a             = SOCKET_ERROR;
        SOCKET b             = SOCKET_ERROR;

        
        try
        {
            int error;

            if( domain != PF_INET )  throw Z_EAFNOSUPPORT;


            // LISTEN

            listen_socket = socket( domain, type, protocol );
            if( listen_socket == SOCKET_ERROR )  throw socket_errno();

            SetHandleInformation( (HANDLE)listen_socket, HANDLE_FLAG_INHERIT, 0 );

            sockaddr_in listen_address;
            memset( &listen_address, 0, sizeof listen_address );
            listen_address.sin_family = AF_INET;
            listen_address.sin_addr   = Ip_address( 127, 0, 0, 1 ).as_in_addr();
            listen_address.sin_port   = htons( 59999 );

            int errn = bind_free_port( listen_socket, &listen_address );
            if( errn )  throw errn;

            error = listen( listen_socket, 1 );
            if( error )  throw socket_errno();


            // CONNECT

            a = socket( domain, type, protocol );
            SetHandleInformation( (HANDLE)a, HANDLE_FLAG_INHERIT, 0 );

            Z_LOG2( "socket.connect", Z_FUNCTION << " connect(" << a << ")\n" );
            error = connect( a, (sockaddr*)&listen_address, sizeof listen_address );
            if( error )  throw socket_errno();


            // ACCEPT

            sockaddr_in   accepted_address;
            sockaddrlen_t accepted_address_length = sizeof accepted_address;
            memset( &accepted_address, 0, sizeof accepted_address );

            Z_LOG2( "socket.accept", Z_FUNCTION << " accept(" << a << ")\n" );
            b = accept( listen_socket, (sockaddr*)&accepted_address, &accepted_address_length );
            if( b == SOCKET_ERROR )  throw socket_errno();

            SetHandleInformation( (HANDLE)b, HANDLE_FLAG_INHERIT, 0 );

            closesocket( listen_socket );
            listen_socket = SOCKET_ERROR;


            // Kein Anderer darf die Verbindung aufgebaut haben!

            sockaddr_in   a_address;
            sockaddrlen_t a_address_length = sizeof a_address;
            getsockname( a, (sockaddr*)&a_address, &a_address_length );

            if( accepted_address.sin_addr.S_un.S_addr != a_address.sin_addr.S_un.S_addr )  throw EACCES;
            if( accepted_address.sin_port             != a_address.sin_port             )  throw EACCES;


            // Rückgabe

            sockets[ 0 ] = a;
            sockets[ 1 ] = b;
            return 0;
        }
        catch( int errn )
        {
            if( listen_socket != SOCKET_ERROR )  closesocket( listen_socket ), listen_socket = SOCKET_ERROR;
            if( a != SOCKET_ERROR )  closesocket( a ), a = SOCKET_ERROR;
            if( b != SOCKET_ERROR )  closesocket( b ), b = SOCKET_ERROR;

            Z_LOG( Z_FUNCTION << "ERROR ERRNO-" << errn << "  " << z_strerror( errn ) << "\n" );

            errno = errn;  errno darf unter Windows nicht gesetzt werden, weil socket_errno() benutzt wird...
            return -1;
        }
#   endif
}
*/
//---------------------------------------------------------------------------Ip_address::operator =

void Ip_address::operator = ( const string& name )
{
    ulong ip = inet_addr( name.c_str() );
    
    if( ip == INADDR_NONE )
    {
        if( rtrim( name ) == "" )  
        {
            ip = 0;
        }
        else
        {
            // Besser auslagern in Host::resolve_name(), weil gethostbyname() einige Sekunden blockieren kann
            // Host kann ip_adresse oder Name zugewiesen werden, ohne dass gethostbyname() oder gethostbyaddr() gerufen würde.
            // Ip_address kann dann kein Name zugewiesen werden, nur explizit über resolve_name( name );

            hostent* h = z_gethostbyname( name.c_str() );
            uint32*  p = *(uint32**)h->h_addr_list;
            ip = p[ 0 ];
        }
    }

    _ip.s_addr = ip;
}

//------------------------------------------------------------------------------Ip_address::netmask

uint32 Ip_address::netmask() const
{ 
    uint32 net_nr = ntohl(_ip.s_addr);
    uint32 netmask_nr;
    
    // In Linux macht das inet_netof()
    if( ( net_nr >> 24 ) >= 192 )  netmask_nr = 0xFFFFFF00;
    else
    if( ( net_nr >> 24 ) >= 128 )  netmask_nr = 0xFFFF0000;
    else
    if(   net_nr         >  0   )  netmask_nr = 0xFF000000;
    else
                                   netmask_nr = 0x00000000;

    return htonl(netmask_nr);
}

//----------------------------------------------------------------------------------Ip_address::net

Ip_address Ip_address::net() const
{
    return Ip_address( _ip.s_addr & netmask() );
}

//-----------------------------------------------------------------Ip_address::get_host_set_by_name

set<Ip_address> Ip_address::get_host_set_by_name( const string& name )
{
    set<Ip_address> host_set;

    uint32 ip = inet_addr( name.c_str() );

    if( ip != INADDR_NONE )
    {
        host_set.insert( ip );
    }
    else
    {
        hostent* h = z_gethostbyname( name.c_str() );     // In Unix nicht thread-sicher?
        uint32** p = (uint32**)h->h_addr_list;
        while( *p )  host_set.insert( **p ), p++;
    }        

    return host_set;
}

//----------------------------------------------------------------------------Ip_address::ip_string

string Ip_address::ip_string() const
{
#   ifdef Z_WINDOWS

        return inet_ntoa( _ip );        // inet_ntoa() von Windows ist thread-sicher!

#   else
        char buffer [ INET_ADDRSTRLEN ];

        const char* result = inet_ntop( AF_INET, &_ip, buffer, sizeof buffer );
        if( !result )  throw_errno( errno, "inet_ntop" );

        return result;

#   endif

    /*
    return S() << (int)_ip.S_un.S_un_b.s_b1 << '.'
               << (int)_ip.S_un.S_un_b.s_b2 << '.'
               << (int)_ip.S_un.S_un_b.s_b3 << '.'
               << (int)_ip.S_un.S_un_b.s_b4;
    */
}

//------------------------------------------------------------------------------Ip_address::compare
/*
int Ip_address::compare( const Ip_address& h ) const
{
    return _ip.s_addr == h._ip.s_addr? 0 : 
           ntohl( _ip.s_addr ) < ntohl( h._ip.s_addr )? -1 : +1;
}
*/

//-------------------------------------------------------------------------------------------------

void Host::operator = ( const string& name )
{
    ulong ip = inet_addr( name.c_str() );
    
    if( ip == INADDR_NONE )
    {
        *(Ip_address*)this = name;
        _name = name;
        _is_name_set = true;
    }
    else
    {
        set_ip( ip );
    }
}

//-------------------------------------------------------------------------------Host::resolve_name

void Host::resolve_name()
{
    if( !_is_name_set )
    {
        _is_name_set = true;

        Z_LOG( Z_FUNCTION << " gethostbyaddr " << ip_string() << '\n' );
        hostent* h = gethostbyaddr( (char*)&_ip, sizeof _ip, AF_INET );
        if( !h )  return;

        _name = h->h_name;
    }
}

//---------------------------------------------------------------------------------Host::name_or_ip

string Host::name_or_ip() const
{
    return _name != "" || is_empty()? _name 
                                    : ip_string();
}

//----------------------------------------------------------------------------------Host::as_string

string Host::as_string() const
{ 
    if( _ip.s_addr != 0 )
    {
        if( _is_name_set )
        {
            string ip = Ip_address::as_string();
            S result;
            result << ip;
            if( _name != "" && _name != ip )  result << '/' << _name;
            return result;
        }
        else
            return Ip_address::as_string();
    }
    else
    {
        return _name;
    }
}

//-----------------------------------------------------------------------Host_and_port::operator ==

bool Host_and_port::operator == ( const Host_and_port& o ) const
{ 
    return _host == o._host  &&  _port == o._port;
}

//------------------------------------------------------------------------Host_and_port::operator <

bool Host_and_port::operator < ( const Host_and_port& o ) const
{ 
    return _host != o._host? _host < o._host
                           : _port < o._port;
}

//------------------------------------------------------------------------Host_and_port::operator <
/*
bool Host_and_port::operator < ( const Host_and_port& o ) const
{ 
    int h = _host.compare( o._host );

    return h < 0? -1
           h > 0? +1
                : _port < o._port? -1
                  _port > o._port? +1
                                 : 0;
}
*/

//------------------------------------------------------------------------Host_and_port::operator =

void Host_and_port::operator = ( const string& str )
{
    vector<string> host_port = vector_split( ":", str, 2 );

    _host = host_port.size() > 0? host_port[ 0 ] : "";
    _port = host_port.size() > 1? as_int( host_port[ 1 ] ) : 0;
}

//--------------------------------------------------------------------Host_and_port::as_sockaddr_in

struct sockaddr_in Host_and_port::as_sockaddr_in() const
{
    struct sockaddr_in result;

    memset( &result, 0, sizeof result );

    result.sin_family = AF_INET;
    result.sin_addr   = _host.as_in_addr();
    result.sin_port   = htons( _port );

    return result;
}

//-----------------------------------------------------------------------Host_and_port::string_port

string Host_and_port::string_port() const
{
    S result;
    if( _port )  result << _port;
    return result;
}

//-------------------------------------------------------------------------Host_and_port::as_string

string Host_and_port::as_string() const
{ 
    S result;

    if( !is_empty() )
    {
        result << _host;
        result << ":";
        result << _port;
    }

    return result;
}

//----------------------------------------------------------------------------------Socket::~Socket

Socket::~Socket()
{
    if( _socket != SOCKET_ERROR )
    {
        Z_LOG2( "socket", "closesocket(" << _socket << ")\n" );
        closesocket( _socket );
    }
}

//------------------------------------------------------------------------------------Socket::close

void Socket::close()
{
    if( _socket != SOCKET_ERROR )
    {
        Z_LOG2( "socket", "closesocket(" << _socket << ")\n" );
        int err = closesocket( _socket );
        _socket = SOCKET_ERROR;

        if( err )  throw_errno( socket_errno(), "closesocket" );
    }
}

//---------------------------------------------------------------------------------send_udp_message

void send_udp_message( const Host_and_port& host_and_port, const string& message )
{
    struct sockaddr_in  sa;
    SOCKET              udp_socket = socket( AF_INET, SOCK_DGRAM, 0 );
    int                 ret;

    if( udp_socket == SOCKET_ERROR )  throw_socket( socket_errno(), "socket" );

    set_socket_not_inheritable( udp_socket );


    sa.sin_family = AF_INET;
    sa.sin_addr   = host_and_port.host().as_in_addr();
    sa.sin_port   = htons( host_and_port.port() );

    ulong on = 1;
    ret = ioctlsocket( udp_socket, FIONBIO, &on );      // non-blocking I/O
    if( ret == SOCKET_ERROR )
    {
        int errn = socket_errno();
        Z_LOG2( "socket", "closesocket(" << udp_socket << ")\n" );
        closesocket( udp_socket );
        throw_socket( errn, "ioctl(FIONBIO)" );
    }

    ret = sendto( udp_socket, message.data(), message.length(), 0, (struct sockaddr*)&sa, sizeof sa );
    if( ret != message.length() )
    {
        int errn = socket_errno();
        Z_LOG2( "socket", "closesocket(" << udp_socket << ")\n" );
        closesocket( udp_socket );
        if( ret != -1 )  throw_socket( errn, "sentto", host_and_port.as_string().c_str() );
                   else  z::throw_xc( "UDP-SEND", "message truncated", ret );
    }

    Z_LOG2( "socket", "closesocket(" << udp_socket << ")\n" );
    closesocket( udp_socket );
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
