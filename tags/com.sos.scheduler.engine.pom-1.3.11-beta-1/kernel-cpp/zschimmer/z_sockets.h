// $Id$

#ifndef __Z_SOCKETS_H
#define __Z_SOCKETS_H

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef Z_WINDOWS

#   include "z_windows.h"
#else
#   include "sys/socket.h"

#   ifndef Z_HPUX
#       include <sys/select.h>
#   endif

#   include <sys/ioctl.h>
#   include <sys/types.h>
#   include <sys/time.h>
#   include <sys/wait.h>
#   include <netdb.h>
#   include <netinet/in.h>   // gethostbyname()
#   include <arpa/inet.h>    // inet_addr()
#   include <unistd.h>       // close()
#endif

#ifdef Z_SOLARIS
#   include <stropts.h>
#   include <sys/filio.h>       // ioctl( , FIONBIO )

#   ifndef INADDR_NONE
#       define INADDR_NONE (-1)
#   endif
#endif


#ifdef Z_WINDOWS
    const int Z_EWOULDBLOCK  = WSAEWOULDBLOCK;
    const int Z_ECONNABORTED = WSAECONNABORTED;
    const int Z_ECONNRESET   = WSAECONNRESET;
    const int Z_EINPROGRESS  = WSAEINPROGRESS;
    const int Z_EALREADY     = WSAEALREADY;
    const int Z_EISCONN      = WSAEISCONN;
    const int Z_EAFNOSUPPORT = WSAEAFNOSUPPORT;
    const int SHUT_WR      = SD_SEND;      // Für shutdown()
    const int MSG_NOSIGNAL = 0;

    const extern int STDIN_FILENO;
    const extern int STDOUT_FILENO;

    typedef int socklen_t;
    typedef int sockaddrlen_t;

    inline int socket_errno() { return WSAGetLastError(); }

# else

    const int Z_EWOULDBLOCK  = EWOULDBLOCK;
    const int Z_ECONNABORTED = ECONNABORTED;
    const int Z_ECONNRESET   = ECONNRESET;
    const int Z_EINPROGRESS  = EINPROGRESS;
    const int Z_EALREADY     = EALREADY;
    const int Z_EISCONN      = EISCONN;
    const int Z_EAFNOSUPPORT = EAFNOSUPPORT;

    typedef int                 SOCKET;
    const SOCKET                SOCKET_ERROR = (SOCKET)-1;

#   ifndef __SOS_SOSSOCK_H
        inline int socket_errno()           { return errno; }
#   endif
#   ifndef __SOS_SOSSOCK1_H
        inline int closesocket( SOCKET s )  { return close(s); }
        inline int ioctlsocket( SOCKET s, long a, unsigned long* b )  { return ioctl( s, a, b ); }
#   endif

#   if defined Z_HPUX || defined Z_SOLARIS
        const int MSG_NOSIGNAL = 0;    // HP-UX kennt diese Option fuer send() nicht. Wir muessen SIGPIPE ignorieren (im jeweiligen Hauptprogramm)
#   endif

#   if defined Z_HPUX  &&  defined __IA64__
        typedef int             sockaddrlen_t;
#    else
        typedef socklen_t       sockaddrlen_t;
#   endif

#endif


//-------------------------------------------------------------------------------------------------

namespace zschimmer {

struct Host_and_port;

//-------------------------------------------------------------------------------------------------

//inline ostream& operator << ( ostream& s, const fd_set& fds )
//{
//    s << '{';
//    for( int i = 0; i < FD_SETSIZE; i++ )  if( FD_ISSET( i, &fds ) )  s << i << " ";
//    s << '}';
//    return s;
//}

//-------------------------------------------------------------------------------------------------

void                            set_socket_not_inheritable  ( SOCKET  );
int                             bind_free_port              ( SOCKET, sockaddr_in* );
int                             z_socketpair                ( int domain, int type, int protocol, SOCKET sockets[2] );
void                            send_udp_message            ( const Host_and_port&, const string& );

//---------------------------------------------------------------------------------------Ip_address

struct Ip_address
{
                                Ip_address                  ()                                      { _ip.s_addr = 0; }
                                Ip_address                  ( const in_addr& ip )                   { _ip = ip; }
                                Ip_address                  ( uint32 ip )                           { _ip.s_addr = ip; }
                                Ip_address                  ( unsigned char a, unsigned char b, unsigned char c, unsigned char d )
                                                                                                    { _ip.s_addr = htonl( (uint32)a << 24 | (uint32)b << 16 | (uint32)c << 8 | d ); }

    void                        operator =                  ( const in_addr& ip )                   { set_ip(ip.s_addr); }
    void                        operator =                  ( const string& );
                              //operator in_addr            () const                                { return _ip; }

    bool                        operator ==                 ( const Ip_address& h ) const           { return _ip.s_addr == h._ip.s_addr; }
    bool                        operator !=                 ( const Ip_address& h ) const           { return _ip.s_addr != h._ip.s_addr; }
    bool                        operator <                  ( const Ip_address& h ) const           { return ntohl( _ip.s_addr ) <  ntohl( h._ip.s_addr ); }  // Für map<>
  //int                         compare                     ( const Ip_address& h ) const;

                                operator bool               () const                                { return _ip.s_addr != 0; }
    bool                        operator !                  () const                                { return _ip.s_addr == 0; }
                              //operator in_addr            () const                                { return as_in_addr(); }

    uint32                      netmask                     () const;                               // Network byte order
    Ip_address                  net                         () const;

    in_addr                     as_in_addr                  () const                                { return _ip; }
    virtual string              as_string                   () const                                { return ip_string(); }
    size_t                      hash_value                  () const                                { return _ip.s_addr; }
    friend ostream&             operator <<                 ( ostream& s, const Ip_address& o )     { s << o.as_string(); return s; }

    void                    set_ip                          ( uint32 ip )                           { _ip.s_addr = ip; }
    string                      ip_string                   () const;
    bool                        is_empty                    () const                                {  return _ip.s_addr == 0; }
    virtual void                resolve_name                ()                                      {}

    static std::set<Ip_address> get_host_set_by_name        ( const string& name );

  protected:
    in_addr                    _ip;                         // IP-Nummer. Muss vielleicht für IPv6 angepasst werden?


  public:
    static const Ip_address     localhost;
};

//---------------------------------------------------------------------------------------------Host

struct Host : Ip_address
{
                                Host                        ()                                      : _zero_(this+1) {}
                                Host                        ( const in_addr& ip )                   : _zero_(this+1) { set_ip(ip.s_addr); }
                                Host                        ( uint32 ip )                           : _zero_(this+1) { set_ip(ip); }
                                Host                        ( const Ip_address& ip )                : _zero_(this+1), Ip_address( ip ) {}
                                Host                        ( const string& name )                  : _zero_(this+1) { *this = name; }
    virtual                    ~Host                        ()                                      {}                      // gcc 3.2 zuliebe

    void                        operator =                  ( const in_addr& ip )                   { set_ip(ip.s_addr); }
    void                        operator =                  ( const string& );

    void                        resolve_name                ();
    string                      get_name                    ()                                      { resolve_name();  return _name; }
    string                      name                        () const                                { return _name; }
    string                      name_or_ip                  () const;
    bool                        is_empty                    () const                                { return _name == ""  &&  Ip_address::is_empty(); }
    
    string                      as_string                   () const;

  protected:
    Fill_zero                  _zero_;
    bool                       _is_name_set;
    string                     _name;
};

//------------------------------------------------------------------------------------Host_and_port

struct Host_and_port
{
                                Host_and_port               ()                                      : _port(0) {}
                                Host_and_port               ( const string& str )                   { *this = str; }
                                Host_and_port               ( const Host& h, int port )             : _host(h), _port(port) {}
                                Host_and_port               ( const struct sockaddr_in& a )         { *this = a; }
    virtual                    ~Host_and_port               ()                                      {}


    void                        operator =                  ( const string& str );
    Host_and_port&              operator =                  ( const struct sockaddr_in& a )         { _host = a.sin_addr;  _port = ntohs(a.sin_port);  return *this; }

    bool                        operator ==                 ( const Host_and_port& o ) const;
    bool                        operator !=                 ( const Host_and_port& o ) const        { return !( *this == o ); }
    bool                        operator <                  ( const Host_and_port& o ) const;       // Für map<>
                                operator bool               () const                                { return !is_empty(); }
    bool                        operator !                  () const                                { return is_empty(); }

    bool                        is_empty                    () const                                { return _host.is_empty()  &&  _port == 0; }

                              //operator sockaddr_in        () const                                { return as_sockaddr_in(); }

    sockaddr_in                 as_sockaddr_in              () const;
    virtual string              as_string                   () const;
    friend ostream&             operator <<                 ( ostream& s, const Host_and_port& o )  { s << o.as_string();  return s; }

    Host&                       host                        ()                                      { return _host; }
    const Host&                 host                        () const                                { return _host; }
    Ip_address                  ip                          () const                                { return (Ip_address)_host; }
    int                         port                        () const                                { return _port; }
    string                      string_port                 () const;                               // "", wenn port fehlt

    size_t                      hash_value                  () const                                { return _host.hash_value() ^ _port; }


    Host                       _host;
    int                        _port;
};

inline size_t                   hash_value                  ( const Host_and_port& hp )             { return hp.hash_value(); }

//-------------------------------------------------------------------------------------------Socket

struct Socket : Non_cloneable
{
                                Socket                      ( int s = SOCKET_ERROR )                : _socket( s ) {}
    virtual                    ~Socket                      ();


    Socket&                     operator =                  ( SOCKET s )                            { assign( s );  return *this; }

    void                        close                       ();
    void                        assign                      ( SOCKET s )                            { close();  _socket = s; }
                                operator SOCKET             () const                                { return _socket; }


    SOCKET                     _socket;
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer


Z_DEFINE_GNU_HASH_VALUE( zschimmer, Host_and_port )


#endif
