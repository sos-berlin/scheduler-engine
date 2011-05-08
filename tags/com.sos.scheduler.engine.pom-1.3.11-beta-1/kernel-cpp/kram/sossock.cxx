// $Id$

#include "precomp.h"
#include "sysdep.h"
//#define MODULE_NAME "sossock"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"


#if defined WIN16  ||  defined __BORLANDC__ //SYSTEM_WIN16
//jz 15.4.98 #if defined _WIN32 || defined WIN16  ||  defined __BORLANDC__ //SYSTEM_WIN16
#   define SOSSOCK_WND            // asynchrone I/O über Windows-Botschaften (in Windows)
#endif

#include <time.h>               // egcs setzt FD_SETSIZE, also vor #define FD_SETSIZE einziehen! jz 1.11.98

#ifdef SYSTEM_LINUX             // Ab Suse 8 nicht mehr nötig
#   define __need_timeval
#   include <bits/time.h>
#endif
//Bis gcc 3.0.4: #ifdef SYSTEM_LINUX             // Wo wird denn in Linux timeval definiert???
//	struct timeval {
//		time_t      tv_sec;     /* seconds */
//		int         tv_usec;    /* microseconds */
//	};
//#endif

#include <stdlib.h>

#ifndef SYSTEM_LINUX    //jz 20.9.00 
//#   define FD_SETSIZE 2000        // max. Anzahl Sockets, wird in sysdep.h vor #include <windows.h> gesetzt
#endif

#include <limits.h>
#include <stdio.h>

#include "../kram/sosstrg0.h"
#include "../kram/sosstrng.h"           // MFC zieht selbst windows.h
#include "../kram/sysdep.h"

#include "../kram/sossock1.h"

#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/xception.h"
#include "../kram/soswin.h"
//#if defined SYSTEM_WIN
//#   include "../kram/sosmswin.h"
//#endif
#include "../kram/sosarray.h"
#include "../kram/sosobj.h"
#include "../kram/sosmsg.h"
#include "../kram/sosfact.h"
#include "../kram/sosopt.h"
#include "../kram/sosstat.h"
#include "../kram/sossock.h"
#include "../kram/sosdll.h"
#include "../kram/sostimer.h"
#include "../kram/sosprof.h" // wg. read_profile_string( "", "fileclient", "myhostname" )

#if defined SYSTEM_WIN

    extern HINSTANCE _hinstance;

#endif

#if defined Z_WINDOWS  ||  defined Z_HPUX  &&  defined __IA64__
    typedef int             sockaddrlen_t;
#else
    typedef socklen_t       sockaddrlen_t;
#endif

using namespace std;
namespace sos {


bool sossock_reschedule    = false;     // Setzt eine Applikation, die mit rescheduling klar kommt
bool sossock_in_select     = false;

#ifdef SOSSOCK_WND
    bool sossock_dispatch_self = false;     // Wird vom Windows-FS gesetzt
    bool sossock_scheduling    = false;     // "GetMessage, DispatchMessage in sossock läuft"
#endif


#if defined SYSTEM_WIN32
#   pragma comment( lib, "wsock32.lib" )
#endif

#if !defined SYSTEM_LINUX && !defined Z_SOLARIS
    typedef int socklen_t;
#endif


#if defined SYSTEM_WIN

    const int   socket_msg  = WM_USER + 0x101;
//  const int   socket_msg2 = WM_USER + 0x102;          // Für verzögertes closesocket
    const char  mswin_socket_class_name [] = "Sos_socket_manager";

#else

    const int WSAEINTR           = EINTR;
    const int WSAEBADF           = EBADF;
    const int WSAEACCES          = EACCES;
    const int WSAEFAULT          = EFAULT;
    const int WSAEINVAL          = EINVAL;
    const int WSAEMFILE          = EMFILE;
    const int WSAEWOULDBLOCK     = EWOULDBLOCK;
    const int WSAEINPROGRESS     = EINPROGRESS;
    const int WSAEALREADY        = EALREADY;
    const int WSAENOTSOCK        = ENOTSOCK;
    const int WSAEDESTADDRREQ    = EDESTADDRREQ;
    const int WSAEMSGSIZE        = EMSGSIZE;
    const int WSAEPROTOTYPE      = EPROTOTYPE;
    const int WSAENOPROTOOPT     = ENOPROTOOPT;
    const int WSAEPROTONOSUPPORT = EPROTONOSUPPORT;
    const int WSAESOCKTNOSUPPORT = ESOCKTNOSUPPORT;
    const int WSAEOPNOTSUPP      = EOPNOTSUPP;
    const int WSAEPFNOSUPPORT    = EPFNOSUPPORT;
    const int WSAEAFNOSUPPORT    = EAFNOSUPPORT;
    const int WSAEADDRINUSE      = EADDRINUSE;
    const int WSAEADDRNOTAVAIL   = EADDRNOTAVAIL;
    const int WSAENETDOWN        = ENETDOWN;
    const int WSAENETUNREACH     = ENETUNREACH;
    const int WSAENETRESET       = ENETRESET;
    const int WSAECONNABORTED    = ECONNABORTED;
    const int WSAECONNRESET      = ECONNRESET;
    const int WSAENOBUFS         = ENOBUFS;
    const int WSAEISCONN         = EISCONN;
    const int WSAENOTCONN        = ENOTCONN;
    const int WSAESHUTDOWN       = ESHUTDOWN;
    const int WSAETOOMANYREFS    = ETOOMANYREFS;
    const int WSAETIMEDOUT       = ETIMEDOUT;
    const int WSAECONNREFUSED    = ECONNREFUSED;
    const int WSAELOOP           = ELOOP;
    const int WSAENAMETOOLONG    = ENAMETOOLONG;
    const int WSAEHOSTDOWN       = EHOSTDOWN;
    const int WSAEHOSTUNREACH    = EHOSTUNREACH;
    const int WSAENOTEMPTY       = ENOTEMPTY;
  //const int WSAEPROCLIM        = EPROCLIM;
    const int WSAEUSERS          = EUSERS;
  //const int WSAEDQUOT          = EDQUOT;
    const int WSAESTALE          = ESTALE;
    const int WSAEREMOTE         = EREMOTE;

    typedef int BOOL;                // für setsockopt() und getsockopt()

    const SOCKET INVALID_SOCKET = (SOCKET)-1;   // von socket()

#   ifndef SOCKET_ERROR
        const SOCKET SOCKET_ERROR   = (SOCKET)-1;   // von send(), recv(), ...
#   endif

//#   if !defined SYSTEM_LINUX
#   if (defined SYSTEM_SOLARIS  && ! defined INADDR_NONE) || defined SYSTEM_WIN
        const int    INADDR_NONE = -1;          // von inet_addr()
#   endif

    int socket_errno()
    {
        return errno;
    }

#endif

//-----------------------------------------------------------------------------Sos_socket_descr

struct Sos_socket_descr : Sos_object_descr
{
    BASE_CLASS( Sos_object_descr )

    virtual const char* name() const  { return "socket"; }


    int is_my_name( const char* name ) const
    {
        ASSERT_VIRTUAL( is_my_name );

        /*
         * Address families.
        #define AF_UNSPEC       0               // unspecified
        #define AF_UNIX         1               // local to host (pipes, portals)
        #define AF_INET         2               // internetwork: UDP, TCP, etc.
        #define AF_IMPLINK      3               // arpanet imp addresses
        #define AF_PUP          4               // pup protocols: e.g. BSP
        #define AF_CHAOS        5               // mit CHAOS protocols
        #define AF_IPX          6               // IPX and SPX
        #define AF_NS           6               // XEROX NS protocols
        #define AF_ISO          7               // ISO protocols
        #define AF_OSI          AF_ISO          // OSI is ISO
        #define AF_ECMA         8               // european computer manufacturers
        #define AF_DATAKIT      9               // datakit protocols
        #define AF_CCITT        10              // CCITT protocols, X.25 etc
        #define AF_SNA          11              // IBM SNA
        #define AF_DECnet       12              // DECnet
        #define AF_DLI          13              // Direct data link interface
        #define AF_LAT          14              // LAT
        #define AF_HYLINK       15              // NSC Hyperchannel
        #define AF_APPLETALK    16              // AppleTalk
        #define AF_NETBIOS      17              // NetBios-style addresses
         */

        if( strcmp( name, "tcp"       ) == 0 )  return SOCK_STREAM;
        if( strcmp( name, "udp"       ) == 0 )  return SOCK_DGRAM;
      //if( strcmp( name, "raw"       ) == 0 )  return SOCK_RAW;
      //if( strcmp( name, "rdm"       ) == 0 )  return SOCK_RDM;       // reliably-delivered message
      //if( strcmp( name, "seqpacket" ) == 0 )  return SOCK_SEQPACKET; // sequenced packet stream
        return 0;
    }

    Sos_object_ptr create( Subtype_code socket_type ) const
    {
        Sos_ptr<Sos_socket> s = SOS_NEW_PTR( Sos_socket( socket_type ) );
        return +s;
    }
};

       const Sos_socket_descr  _sos_socket_descr;
extern const Sos_object_descr&  sos_socket_descr = _sos_socket_descr;
//extern const Sos_socket_descr& sos_socket_descr = Sos_socket_descr();

//-------------------------------------------------------------------------------------------------


//typedef Sos_simple_list_node< Sos_socket* >  Sos_socket_list_node;

struct Sos_socket_manager : Sos_object
{
                                Sos_socket_manager          ();
                               ~Sos_socket_manager          ();

    void                        init                        ();
    void                        add                         ( Sos_socket* );
    void                        del                         ( Sos_socket* );
    void                        status                      ( SOCKET, Sos_socket::Status );
    void                        clear_status                ( SOCKET, Sos_socket::Status );
    Sos_socket*                 sos_socket_ptr              ( SOCKET ) const;
    void                        listen                      ( int port );
    void                        setsockopt_std              ( SOCKET );
    void                        set_linger                  ( SOCKET, int, int );
    void                        ioctl_fionbio               ( SOCKET );

#   if defined SOSSOCK_WND
      //int                     mswin_message_event         ( WORD msg, WORD wParam, DWORD lParam, LONG* ret );
        HWND                    hwnd                        () const            { return _hwnd; }
#   endif

    void                        select                      ();

    friend struct               Sos_socket;

    Fill_zero                  _zero_;
    int                        _open_event_count;

//private:

    Sos_socket_manager*        _socket_manager_ptr;         // Für SOCKET_LIB
    Bool                       _initialized;
    int                        _nfds;                       // für select()
    int                        _socket_count;
    Sos_simple_array<Sos_socket*>  _socket_array;
    SOCKET                     _listen_socket;
  //Sos_socket_list_node*      _socket_list;

#   if defined SOSSOCK_WND
        HWND                   _hwnd;           // Window-Handle für spezielles Socket-msg-window
        Bool                   _reschedule;
        int                    _in_event_handler;
#    else
        fd_set                 _read_fds;
        fd_set                 _write_fds;
#   endif

#   if defined SYSTEM_WIN
        WSADATA                _wsa_data;
      //const WORD             _msg;
#   endif

};

//---------------------------------------------------------------operator<< ( ostream, fd_set )
//#if !defined SOSSOCK_WND
//
//ostream& operator<< ( ostream& s, const fd_set& st )
//{
//    s << '{';
//    for( int i = 0; i < (FD_SETSIZE); i++ )  if( FD_ISSET( i, &st ) )  s << ' ' << i;
//    s << '}';
//    return s;
//}
//
//#endif
//----------------------------------------------Sos_static_ptr<Sos_socket_manager>::sos_pointer

DEFINE_SOS_STATIC_PTR( Sos_socket_manager )

//---------------------------------------------------------------------------socket_errno (WIN)

#if defined SYSTEM_WIN
    int socket_errno()
    {
#       if defined SYSTEM_WIN16
            return sos_static_ptr()->_socket_manager_ptr->_functions.WSAGetLastError();
#       else
            return WSAGetLastError();
#       endif
    }
#endif

//-----------------------------------------------------------------------sos_socket_manager_ptr

inline Sos_socket_manager* sos_socket_manager_ptr()
{
    return (Sos_socket_manager*)+sos_static_ptr()->_socket_manager_ptr;
}

//-----------------------------------------------------------------------------Sos_socket_error

struct Sos_socket_error : Xc
{
                                Sos_socket_error        ( const char* insertion = 0 );
                                Sos_socket_error        ( int errno_, const char* insertion = 0 );
};

Sos_socket_error::Sos_socket_error( int errno_, const char* insertion )
:
    Xc( Msg_code( "SOCKET-", Sos_socket::normalized_error( errno_ ) ),
        Msg_insertions( insertion ) )
{
}

Sos_socket_error::Sos_socket_error( const char* insertion )
:
    Xc( Msg_code( "SOCKET-", Sos_socket::normalized_error( socket_errno() ) ),
        Msg_insertions( insertion ) )
{
}

//-----------------------------------------------------------------------throw_sos_socket_error

void throw_sos_socket_error( int errno_, const char* insertion )
{
    throw Sos_socket_error ( errno_, insertion );
}

//-----------------------------------------------------------------------throw_sos_socket_error

void throw_sos_socket_error( const char* insertion, const Sos_object_base* o )
{
    Sos_socket_error x ( insertion );
    x.insert( o );
    throw x;
}

//-----------------------------------------------------------------------throw_sos_socket_error

void throw_sos_socket_error( const char* insertion )
{
    Sos_socket_error x ( insertion );
    throw x;
}

//-----------------------------------------------------------------------throw_sos_socket_error

void throw_sos_socket_error( const char* insertion, const char* ins2 )
{
    Sos_socket_error x ( insertion );
    x.insert( ins2 );
    throw x;
}

//---------------------------------------------------------------------------throw_socket_error

static void throw_socket_error( const char* insertion = 0 )
{
    int e = socket_errno();
    int ne = Sos_socket::normalized_error( e );

    switch( e )
    {
        case WSAEMSGSIZE: { Too_long_error x ( Msg_code( "SOCKET-", ne ) ); x.insert( insertion ); throw x; }
#       if !defined SYSTEM_WIN
            case EPIPE:
#       endif
        case WSAENETDOWN:
        case WSAENETRESET:
        case WSAECONNABORTED:
        case WSAECONNRESET:
        case WSAESHUTDOWN:
        case WSAETIMEDOUT:
        case WSAEHOSTDOWN: { Connection_lost_error x ( Msg_code( "SOCKET-", ne ) ); x.insert( insertion ); throw x; }
        default       : throw_sos_socket_error( e, insertion );
    }
}

//---------------------------------------------------------------------------------socket_error_tab

const struct
{
    int2    wsa_error_code;
    uint2   sos_error_code;
    Bool    socket_unuseable;
}
socket_error_tab[] =
{
    { WSAEINTR           /*10004*/, (uint2) Sos_socket::eintr           , false },   // As in standard C
    { WSAEBADF           /*10009*/, (uint2) Sos_socket::ebadf           , false },   // As in standard C
    { WSAEACCES          /*10013*/, (uint2) Sos_socket::eacces          , false },   // As in standard C
    { WSAEFAULT          /*10014*/, (uint2) Sos_socket::efault          , false },   // As in standard C
    { WSAEINVAL          /*10022*/, (uint2) Sos_socket::einval          , false },   // As in standard C
    { WSAEMFILE          /*10024*/, (uint2) Sos_socket::emfile          , false },   // As in standard C
    { WSAEWOULDBLOCK     /*10035*/, (uint2) Sos_socket::ewouldblock     , false },   // As in BSD
    { WSAEINPROGRESS     /*10036*/, (uint2) Sos_socket::einprogress     , false },   // This error is returned if anyWindows Sockets API function is called while a blocking function is in progress.
    { WSAEALREADY        /*10037*/, (uint2) Sos_socket::ealready        , false },   // As in BSD
    { WSAENOTSOCK        /*10038*/, (uint2) Sos_socket::enotsock        , false },   // As in BSD
    { WSAEDESTADDRREQ    /*10039*/, (uint2) Sos_socket::edestaddrreq    , false },   // As in BSD
    { WSAEMSGSIZE        /*10040*/, (uint2) Sos_socket::emsgsize        , false },   // As in BSD
    { WSAEPROTOTYPE      /*10041*/, (uint2) Sos_socket::eprototype      , false },   // As in BSD
    { WSAENOPROTOOPT     /*10042*/, (uint2) Sos_socket::enoprotoopt     , false },   // As in BSD
    { WSAEPROTONOSUPPORT /*10043*/, (uint2) Sos_socket::protonosupport  , false },   // As in BSD
    { WSAESOCKTNOSUPPORT /*10044*/, (uint2) Sos_socket::esocktnosupport , false },   // As in BSD
    { WSAEOPNOTSUPP      /*10045*/, (uint2) Sos_socket::eopnotsupp      , false },   // As in BSD
    { WSAEPFNOSUPPORT    /*10046*/, (uint2) Sos_socket::epfnosupport    , false },   // As in BSD
    { WSAEAFNOSUPPORT    /*10047*/, (uint2) Sos_socket::eafnosupport    , false },   // As in BSD
    { WSAEADDRINUSE      /*10048*/, (uint2) Sos_socket::eaddrinuse      , false },   // As in BSD
    { WSAEADDRNOTAVAIL   /*10049*/, (uint2) Sos_socket::eaddrnotavail   , false },   // As in BSD
    { WSAENETDOWN        /*10050*/, (uint2) Sos_socket::enetdown        , true  },   // As in BSD.  This error may be reported at any time if the Windows Sockets implementation detects an underlying failure.
    { WSAENETUNREACH     /*10051*/, (uint2) Sos_socket::enetunreach     , true  },   // As in BSD
    { WSAENETRESET       /*10052*/, (uint2) Sos_socket::enetreset       , true  },   // As in BSD
    { WSAECONNABORTED    /*10053*/, (uint2) Sos_socket::econnaborted    , true  },   // As in BSD
    { WSAECONNRESET      /*10054*/, (uint2) Sos_socket::econnreset      , true  },   // As in BSD
    { WSAENOBUFS         /*10055*/, (uint2) Sos_socket::enobufs         , false },   // As in BSD
    { WSAEISCONN         /*10056*/, (uint2) Sos_socket::eisconn         , false },   // As in BSD
    { WSAENOTCONN        /*10057*/, (uint2) Sos_socket::enotconn        , true  },   // As in BSD
    { WSAESHUTDOWN       /*10058*/, (uint2) Sos_socket::eshutdown       , true  },   // As in BSD
    { WSAETOOMANYREFS    /*10059*/, (uint2) Sos_socket::etoomanyrefs    , false },   // As in BSD
    { WSAETIMEDOUT       /*10060*/, (uint2) Sos_socket::etimedout       , false },   // As in BSD
    { WSAECONNREFUSED    /*10061*/, (uint2) Sos_socket::econnrefused    , true  },   // As in BSD
    { WSAELOOP           /*10062*/, (uint2) Sos_socket::eloop           , false },   // As in BSD
    { WSAENAMETOOLONG    /*10063*/, (uint2) Sos_socket::enametoolong    , false },   // As in BSD
    { WSAEHOSTDOWN       /*10064*/, (uint2) Sos_socket::ehostdown       , true  },   // As in BSD
    { WSAEHOSTUNREACH    /*10065*/, (uint2) Sos_socket::ehostunreach    , false }    // As in BSD

# if defined SYSTEM_WIN   // Gibt's diese Fehlercodes wirklich nur in MS-Windows?
   ,{ WSASYSNOTREADY     /*10091*/, (uint2) Sos_socket::sysnotready     , false },   // Returned by WSAStartup() indicating that the network subsystem is unusable.
    { WSAVERNOTSUPPORTED /*10092*/, (uint2) Sos_socket::vernotsupported , false },   // Returned by WSAStartup()indicating that the Windows Sockets DLL cannot support this app.
    { WSANOTINITIALISED  /*10093*/, (uint2) Sos_socket::notinitialised  , false },   // Returned by any function except WSAStartup() indicating that a successful WSAStartup() has not yet been performed.
    { WSAHOST_NOT_FOUND  /*11001*/, (uint2) Sos_socket::host_not_found  , false },   // As in BSD.
    { WSATRY_AGAIN       /*11002*/, (uint2) Sos_socket::try_again       , false },   // As in BSD
    { WSANO_RECOVERY     /*11003*/, (uint2) Sos_socket::no_recovery     , false },   // As in BSD
    { WSANO_DATA         /*11004*/, (uint2) Sos_socket::no_data         , false }    // As in BSD
# endif
};

//------------------------------------------------------------------------mswin_message_handler
#if defined SOSSOCK_WND

#if defined SYSTEM_WIN32
LRESULT CALLBACK
#else
LRESULT CALLBACK __export __pascal
#endif
mswin_message_handler( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    //LOGI( "Sos_socket.mswin_window_message( " << hex << msg << ", " << wParam << ", " << lParam << ")\n" << dec );

    if( msg == socket_msg )
    {
        Sos_socket_manager* socket_manager_ptr = sos_socket_manager_ptr();

        if( socket_manager_ptr )
        {
            Sos_socket* s = socket_manager_ptr->sos_socket_ptr( wParam );
            if( !s ) {
                // Sos_socket::close() wartet FD_CLOSE nicht ab, daher:
                LOG( "Sos_socket::_socket_event: Unbekannter Socket " << wParam << ", Ereignis " << WSAGETSELECTEVENT( lParam ) << '\n' );
                return 1;
            }

            LRESULT result = s->mswin_message_event( msg, wParam, lParam );

            return result;
        }
        else
        {
            //LOG( "socket_msg ohne Sockets?\n" );
            return 0;
        }
    }
/*
    else
    if( msg == socket_msg2 ) {
        LOG( "mswin_message_handler: verzögertes closesocket(" << wParam << ")\n" );
        int rc = sos_socket_manager_ptr()->_functions.closesocket( wParam );
        if( !rc )  LOG( "error " << socket_errno << "\n" );
    }
*/
    else
    {
        return DefWindowProc( hwnd, msg, wParam, lParam );
    }
}

#endif
//--------------------------------------------------------------------------------blocking_hook
#if defined SOSSOCK_WND


static BOOL
#if defined WIN32
    __stdcall
#endif
#if defined SYSTEM_WIN16
    __pascal __export/*jz 5.2.97*/
#endif
blocking_hook()
{
/*  Original von Microsoft:
    MSG msg;
    BOOL ret;

    // get the next message, if any
    ret = (BOOL) PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);

    // if we got one, process it
    if (ret) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // TRUE if we got a message
    return ret;
*/



    sossock_scheduling = true;          // Globale Variable

    MSG  msg;
    int  ok;
    uint timer_id     = 1;
    uint timer_handle = 0;
    Sos_socket_manager* man = sos_static_ptr()->_socket_manager_ptr;

    double wait = sos_static_ptr()->_timer_manager->wait_time();

    if( wait < INT4_MAX - 2 ) {
        timer_handle = SetTimer( sos_static_ptr()->_socket_manager_ptr->_hwnd, timer_id,
                                 int( MIN( wait, (double)( UINT_MAX / 1000 ) ) * 1000.0 ),
                                 NULL );
    }


    while(1)   // Bis eine akzeptierte Botschaft eintrifft
    {
        ok = man->_reschedule || sossock_reschedule? PeekMessage( &msg, 0, 0, 0, PM_REMOVE )
                                                   : PeekMessage( &msg, man->_hwnd, socket_msg, socket_msg, PM_REMOVE );
        //if( !ok )  break;
        if( !ok )  return FALSE;

        TranslateMessage(&msg);

        if( sossock_reschedule )  break;    // Die Applikation hat's erlaubt

        //if( msg.message >= WM_KEYFIRST    &&  msg.message <= WM_KEYLAST
        // || msg.message >= WM_MOUSEFIRST  &&  msg.message <= WM_MOUSELAST )
        //{
        //    MessageBeep( MB_ICONHAND );
        //}
        //else break;
        if( msg.message == socket_msg )  break;
        if( msg.message == WM_PAINT   )  break;
        if( msg.message == WM_TIMER   )  break;

        if( msg.message == 0 )  break;             // endlos in dbWeb 1.1 beim zweiten Aufruf. Was ist das?

        if(/* msg.message != 0                     // endlos in dbWeb 1.1 beim zweiten Aufruf. Was ist das?
         &&*/ msg.message != WM_KEYDOWN
         && msg.message != WM_KEYUP
         && msg.message != WM_MOUSEMOVE
         && msg.message != WM_NCMOUSEMOVE     // im Fensterkopf
         && msg.message != WM_NCMOUSEMOVE
/*
         && msg.message != WM_NCLBUTTONDOWN
         && msg.message != WM_NCLBUTTONUP
         && msg.message != WM_NCLBUTTONDBLCLK
         && msg.message != WM_NCRBUTTONDOWN
         && msg.message != WM_NCRBUTTONUP
         && msg.message != WM_NCRBUTTONDBLCLK
         && msg.message != WM_NCMBUTTONDOWN
         && msg.message != WM_NCMBUTTONUP
         && msg.message != WM_NCMBUTTONDBLCLK
*/
        ){
            // Zu laut auf langsamen PC: MessageBeep( MB_ICONHAND );
            LOG( "Sos_socket_manager::select message " << msg.message << " refused\n" );
        }
    }

    //if( msg.message != socket_msg ) {
         //LOG( "Sos_socket_manager::select() Botschaft " << hex << msg.message << " hwnd=" << (uint)msg.hwnd << " wParam=" << msg.wParam << " lParam=" << msg.lParam << dec << "\n" );
    //}

    if( ok ) {
        //TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if( timer_handle )  KillTimer( man->_hwnd, timer_id );

    sossock_scheduling = false;

/* MessageBox kommt immer bei FS DISABLE nach Alt-F4:
    if( msg.message == WM_QUIT ) {
        int a = MessageBox( 0, "Eine Netzwerk-Transaktion ist noch nicht abgeschlossen."
                               " Trotzdem abbrechen?",
                            0, MB_YESNO );
        if( a == IDYES ) {
            PostQuitMessage(1);
            throw Abort_error();
        }
    }
*/
    return TRUE;
}

#endif

//-----------------------------------------------------------Sos_socket_manager::Sos_socket_manager

Sos_socket_manager::Sos_socket_manager()
:
    _zero_            ( this+1  ),
    _socket_manager_ptr( this   )
{
    _socket_array.obj_const_name( "Sos_socket_manager::_socket_array" );
    //init();
}

//----------------------------------------------------------Sos_socket_manager::~Sos_socket_manager

Sos_socket_manager::~Sos_socket_manager()
{
    //LOGI( "~Sos_socket_manager\n" );

  //socket_manager_ptr = 0;

    if( !_initialized )  return;

    if( _listen_socket ) {
        set_linger( _listen_socket, 0, 0 );
        LOG( "close(" << _listen_socket << ")\n" );
        int rc = ::closesocket( _listen_socket );
        if( rc == SOCKET_ERROR )  LOG( "~Sos_socket_manager: Fehler " << rc << " bei closesocket()\n" );
    }

#   if defined SYSTEM_WIN
        LOG( "~Sos_socket_manager: WSACleanup\n" );
#       if defined SYSTEM_WIN16
            int rc = _functions.WSACleanup();
#       else
            int rc = WSACleanup();
#       endif


        if( rc && socket_errno() == WSAEINPROGRESS ) {
            LOGI( "WSACancelBlockingCall()\n" );
#           if defined SYSTEM_WIN16
                _functions.WSACancelBlockingCall();
                rc = _functions.WSACleanup();
#           else
                WSACancelBlockingCall();
                rc = WSACleanup();
#           endif
        }

        if( rc ) {
            LOG( "sossock: WSACleanup() liefert Fehler " << socket_errno() << "\n" );
        }

#       if defined SOSSOCK_WND
            BOOL ok;
            //LOG( "~Sos_socket_manager: DestroyWindow()\n" );
            ok = DestroyWindow( _hwnd );
            if( !ok )  LOG( "~Sos_socket_manager: DestroyWindow() fehlerhaft\n" );

          //LOG( "~Sos_socket_manager: UnregisterClass()\n" );
          //ok = UnregisterClass( mswin_socket_class_name, Mswin::hinstance() );
          //if( !ok )  LOG( "~Sos_socket_manager: UnregisterClass() fehlerhaft\n" );
#       endif
#    endif

    _initialized = false;  //jz 17.8.98
}

//-------------------------------------------------------------------------Sos_socket_manager::init

#if defined SOSSOCK_WND
HWND _app_window_handle = HWND_DESKTOP;
#endif

void Sos_socket_manager::init()
{
    //jz 13.1.98 _socket_array.first_index( 1 );
    _socket_array.first_index( 0 );     // NDC Marathon TCP/IP 2.0 fängt mit 0 an.

#   if !defined SOSSOCK_WND
        FD_ZERO( &_read_fds );
        FD_ZERO( &_write_fds );
#   endif

#  if defined SYSTEM_WIN

    try {
        int         rc;

#       if defined SOSSOCK_WND
            WNDCLASS    wc;
            memset( &wc, 0, sizeof (WNDCLASS) );

            if( !_hinstance )  throw_xc( "SOS-HINSTANCE" );

            if( !GetClassInfo( _hinstance, mswin_socket_class_name, &wc ) ) {
                wc.style         = 0;
                wc.lpfnWndProc   = mswin_message_handler;
                wc.cbClsExtra    = 0;
                wc.cbWndExtra    = 0;
                wc.hInstance     = _hinstance;
                wc.hIcon         = NULL;
                wc.hCursor       = NULL;
                wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
                wc.lpszMenuName  = NULL;
                wc.lpszClassName = mswin_socket_class_name;
#               if defined SYSTEM_WIN16
                    //OutputDebugString( "RegisterClass\n");
                    //LOG( "RegisterClass( "<< (void*)_hinstance<<"  " <<(void*)mswin_message_handler << "  " <<(void*) mswin_socket_class_name<<"  "<<(void*)&wc<<"\n" );
#               endif
                rc = RegisterClass( &wc );
#               if defined __WIN32__
                    // rc ist ein ATOM
                    if( !rc )  throw_mswin_error( "RegisterClass", "Sos_socket" );
#              endif
            }

  //OutputDebugString( "CreateWindow\n");
            _hwnd = CreateWindow( mswin_socket_class_name,  // Klassenname
                                  "Sos_socket_manager",     // Fenstername
                                  DS_NOIDLEMSG | WS_DISABLED,// Style
                                  0,                        // x
                                  0,                        // y
                                  1,                        // Breite
                                  1,                        // Höhe
                                  _app_window_handle,       // parent window
//                                  HWND_DESKTOP,             // parent window
                                  NULL,                     // hmenu
                                //Mswin::hinstance(),       // hinst
                                  _hinstance,
                                  NULL );                   // lpvParam

            if( !_hwnd )  throw_mswin_error( "CreateWindow", "Sos_socket" );  // in win32 Absturz in ~Sos_socket ... jz 30.4.96
            //LOG( "Sos_socket_manager::_hwnd = " << hex << (uint)_hwnd << dec << '\n' );
#       endif


#       if defined __WIN32__
//            _functions.init( "WSOCK32.DLL" );       // dll laden
#        else
            _functions.init( "WINSOCK.DLL" );       // dll laden
#       endif


        memset( &_wsa_data, 0, sizeof _wsa_data );
#       if defined SYSTEM_WIN16
            rc = _functions.WSAStartup( 0x0101, &_wsa_data );  // Exception bei LoadLibrary() möglich
#       else
            rc = WSAStartup( 0x0101, &_wsa_data );  // Exception bei LoadLibrary() möglich
#       endif

        if( log_ptr ) {
            *log_ptr << "Sos_socket_manager::init(): "
#                       if defined SOSSOCK_WND
                            "_hwnd = " << hex << (uint)_hwnd << dec <<
#                       endif
                        ", WSAStartup() returns "
                        "version=" << hex << _wsa_data.wVersion << "..." << _wsa_data.wHighVersion << dec
                     << " iMaxSockets="   << _wsa_data.iMaxSockets
                     << "  \""            << _wsa_data.szDescription
                     << "\"\nstatus=\""   << _wsa_data.szSystemStatus << "\"\n" << flush;
        }

        if( rc )  throw_sos_socket_error( rc, "WSAStartup" );  // Nicht WSAGetLastError() rufen!

#       if defined SOSSOCK_WND
            _reschedule = read_profile_bool( "", "socket", "reschedule", false );

            LOG( "WSASetBlockingHook()\n" );  // PC-NFS bleibt hier hängen?
            void* ret = (void*)::WSASetBlockingHook( (FARPROC)blocking_hook );
            LOG( "WSASetBlockingHook() ok\n" );
            if( !ret )  throw_sos_socket_error( ::WSAGetLastError(), "WSASetBlockingHook" );
#       endif

        _initialized = true;
    }
    catch( const Xc& )
    {
#       if defined SOSSOCK_WND
            DestroyWindow( _hwnd );
            UnregisterClass( mswin_socket_class_name, _hinstance );
#       endif
        throw;
    }
#  endif
}

//--------------------------------------------------------------------------Sos_socket_manager::add

void Sos_socket_manager::add( Sos_socket* socket_ptr )
{
    //_socket_list = new Sos_socket_list_node( socket_ptr, _socket_list );
    if( _socket_array.last_index() < (int)socket_ptr->_socket ) {
        int i = _socket_array.last_index() + 1;
        _socket_array.last_index( socket_ptr->_socket );
        for( ; i < _socket_array.last_index(); i++ )  _socket_array[ i ] = 0;
    }

    _socket_array[ socket_ptr->_socket ] = socket_ptr;

    _nfds = max( _nfds, (int)socket_ptr->_socket + 1 );
}

//--------------------------------------------------------------------------Sos_socket_manager::del

void Sos_socket_manager::del( Sos_socket* socket_ptr )
{
    if( !socket_ptr )  return; //? e370 jz 22.11.96
  //delete_node( node_ptr_ptr( &_socket_list, socket_ptr ) );

    if( socket_ptr->_socket == INVALID_SOCKET ) {
        LOG( "Sos_socket_manager::del(): _socket==INVALID_SOCKET\n" );
		int i;
        for( i = _socket_array.first_index(); i <= _socket_array.last_index(); i++ ) {
            if( _socket_array[ i ] == socket_ptr )  break;
        }
        if( i <= _socket_array.last_index() ) {
            _socket_array[ i ] = 0;
        }
    }
    else
    {
        _socket_array[ socket_ptr->_socket ] = 0;
    }

    while( _nfds > 1  &&  !_socket_array[ _nfds - 1 ] )  _nfds--;

    if( _socket_count == 0 ) {
        SOS_DELETE( sos_static_ptr()->_socket_manager_ptr );     // Selbstzerstörung!
    }
    /// Objekt hat sich evtl. selbst zerstört ///
}

//---------------------------------------------------------------Sos_socket_manager::sos_socket_ptr

Sos_socket* Sos_socket_manager::sos_socket_ptr( SOCKET s ) const
{

/*
    Sos_socket_list_node* n = _socket_list;

    while( !empty( n ) ) {
       if( n->head()->socket() == s )  break;
       n = n->tail();
    }

    return empty( n )? 0 : n->head();
*/
    return _socket_array[ s ];
}

//-------------------------------------------------------------------Sos_socket_manager::status

inline void Sos_socket_manager::status( SOCKET s, Sos_socket::Status sta )
{
    _open_event_count++;

#   if !defined SOSSOCK_WND
        if( sta & (Sos_socket::sta_listening |Sos_socket::sta_receiving))  FD_SET( s, &_read_fds );
        if( sta & (Sos_socket::sta_connecting|Sos_socket::sta_sending  ))  FD_SET( s, &_write_fds );
#   endif
}

//-------------------------------------------------------------Sos_socket_manager::clear_status

inline void Sos_socket_manager::clear_status( SOCKET s, Sos_socket::Status sta )
{
    _open_event_count--;

#   if !defined SOSSOCK_WND
        if( sta & (Sos_socket::sta_listening |Sos_socket::sta_receiving))  FD_CLR( s, &_read_fds );
        if( sta & (Sos_socket::sta_connecting|Sos_socket::sta_sending  ))  FD_CLR( s, &_write_fds );
        //LOG( "Sos_socket_manager::status(): _read_fds =" << _read_fds << '\n' );
        //LOG( "Sos_socket_manager::status(): _write_fds=" << _write_fds << '\n' );
#   endif
}

//-------------------------------------------------------------------Sos_socket_manager::select

void Sos_socket_manager::select()
{
    sos_static_ptr()->_timer_manager->check();   // Abgelaufene Zeitgeber abarbeiten

#   if defined SOSSOCK_WND
#     if 1
        WaitMessage();
        blocking_hook();
#     else
        double wait = sos_static_ptr()->_timer_manager->wait_time();
        sossock_scheduling = true;
        MSG msg;
        int ok;
        uint timer_id     = 1;
        uint timer_handle = 0;

        if( wait < INT4_MAX - 2 ) {
                timer_handle = SetTimer( _hwnd, timer_id,
                                         int( MIN( wait, (double)( UINT_MAX / 1000 ) ) * 1000.0 ),
                                         NULL );
            }

            while(1) {
                ok = _reschedule || sossock_reschedule?
                         GetMessage( &msg, 0, 0, 0 )
                       : GetMessage( &msg, _hwnd, socket_msg, socket_msg );
                if( !ok )  break;

                TranslateMessage(&msg);

                if( sossock_reschedule )  break;    // Die Applikation hat's erlaubt

                //if( msg.message >= WM_KEYFIRST    &&  msg.message <= WM_KEYLAST
                // || msg.message >= WM_MOUSEFIRST  &&  msg.message <= WM_MOUSELAST )
                //{
                //    MessageBeep( MB_ICONHAND );
                //}
                //else break;
                if( msg.message == socket_msg )  break;
                if( msg.message == WM_PAINT   )  break;
                if( msg.message == WM_TIMER   )  break;

                if( msg.message == 0 )  break;             // endlos in dbWeb 1.1 beim zweiten Aufruf. Was ist das?

                if(/* msg.message != 0                     // endlos in dbWeb 1.1 beim zweiten Aufruf. Was ist das?
                 &&*/ msg.message != WM_KEYDOWN
                 && msg.message != WM_KEYUP
                 && msg.message != WM_MOUSEMOVE
                 && msg.message != WM_NCMOUSEMOVE     // im Fensterkopf
                 && msg.message != WM_NCMOUSEMOVE
/*
                 && msg.message != WM_NCLBUTTONDOWN
                 && msg.message != WM_NCLBUTTONUP
                 && msg.message != WM_NCLBUTTONDBLCLK
                 && msg.message != WM_NCRBUTTONDOWN
                 && msg.message != WM_NCRBUTTONUP
                 && msg.message != WM_NCRBUTTONDBLCLK
                 && msg.message != WM_NCMBUTTONDOWN
                 && msg.message != WM_NCMBUTTONUP
                 && msg.message != WM_NCMBUTTONDBLCLK
*/
                ){
                    // Zu laut auf langsamen PC: MessageBeep( MB_ICONHAND );
                    LOG( "Sos_socket_manager::select message " << msg.message << " refused\n" );
                }
            }

            if( msg.message != socket_msg ) {
                LOG( "Sos_socket_manager::select() Botschaft " << hex << msg.message << " hwnd=" << (uint)msg.hwnd << " wParam=" << msg.wParam << " lParam=" << msg.lParam << dec << "\n" );
            }
            if( ok ) {
              //TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if( timer_handle )  KillTimer( _hwnd, timer_id );

            sossock_scheduling = false;
/* MessageBox kommt immer bei FS DISABLE nach Alt-F4:
            if( msg.message == WM_QUIT ) {
                int a = MessageBox( 0, "Eine Netzwerk-Transaktion ist noch nicht abgeschlossen."
                                       " Trotzdem abbrechen?",
                                    0, MB_YESNO );
                if( a == IDYES ) {
                     PostQuitMessage(1);
                     throw Abort_error();
                }
            }
*/
        }
#      endif

#    else

        double wait = sos_static_ptr()->_timer_manager->wait_time();

        fd_set read_fds;
        fd_set write_fds;
#       if defined SYSTEM_WIN
        {
            FD_ZERO( &read_fds );
            FD_ZERO( &write_fds );

            for( int i = _socket_array.first_index(); i <= _socket_array.last_index(); i++ ) {
                Sos_socket* s = _socket_array[ i ];
                if( s ) {
                    SOCKET n = s->_socket;
                    if( n != INVALID_SOCKET ) {
                        if( FD_ISSET( n, &_read_fds  ) )  FD_SET( n, &read_fds  );
                        if( FD_ISSET( n, &_write_fds ) )  FD_SET( n, &write_fds );
                    }
                }
            }
        }
#       elif 1  //defined SYSTEM_SOLARIS
            memcpy( read_fds.fds_bits , _read_fds.fds_bits , sizeof read_fds.fds_bits  );
            memcpy( write_fds.fds_bits, _write_fds.fds_bits, sizeof write_fds.fds_bits );
#       else
            memcpy( read_fds.__fds_bits , _read_fds.__fds_bits , sizeof read_fds.__fds_bits  );
            memcpy( write_fds.__fds_bits, _write_fds.__fds_bits, sizeof write_fds.__fds_bits );
#       endif

        //LOG( "read_fds =" << read_fds << '\n' );
        //LOG( "write_fds=" << write_fds << '\n' );

        wait = MIN( wait, 3600.0 );   // select() verträgt keine große Zahl
        timeval t;        
        t.tv_sec  = time_t( wait );
        t.tv_usec = (unsigned int)( wait * 1e6 ) % 1000000;

//fprintf(stderr,"select timeval=%d.%06d\n",(int)t.tv_sec,(int)t.tv_usec);
        //LOG( "select( " << _nfds << ", read=" << read_fds << ", write=" << write_fds << ", wait=" << wait << " )\n" );
        sossock_in_select = true;
        int n = ::select( _nfds, &read_fds, &write_fds, 0/*except*/, &t );
        sossock_in_select = false;
        //LOG( "        liefert " << n << ", read=" << read_fds << ", write=" << write_fds << "\n" );

        if( n == -1 )  throw_socket_error();
        if( n > 0 )
        {
			int i;
            for( i = 0; i < _nfds; i++ )    // erst send() und connect() abschließen
            {
                if( FD_ISSET( i, &write_fds ) ) {
                    Sos_socket* s = sos_socket_ptr( i );
                    s->event( s->_status & Sos_socket::sta_connecting? Sos_socket::sta_connecting
                                                                     : Sos_socket::sta_sending );
                    if( --n == 0 )  goto FERTIG;
                    dispatch_waiting_msg();     // Antworten zurücksenden  jz 11.4.97
                }
            }

            for( i = 0; i < _nfds; i++ )        // dann recv() und listen()/accept()
            {
                if( FD_ISSET( i, &read_fds ) ) {
                    Sos_socket* s = sos_socket_ptr( i );
                    s->event( s->_status & Sos_socket::sta_listening? Sos_socket::sta_listening
                                                                    : Sos_socket::sta_receiving );
                    if( --n == 0 )  break;
                    dispatch_waiting_msg();     // Antworten zurücksenden  jz 11.4.97
                }
            }

            FERTIG: ;
        }

        if( n != 0 )  LOG( "Sos_socket::select(): n != 0: " << n << "\n" );
#   endif
}

//------------------------------------------------------------------Sos_socket_manager::set_linger

void Sos_socket_manager::set_linger( SOCKET socket, int onoff, int linger )
{
    struct linger l;
    l.l_onoff  = onoff;  /* 0: Nicht warten */
    l.l_linger = linger;
    LOG( "setsockopt(" << socket << ",SOL_SOCKET,SO_LINGER,onoff=" << onoff << ",linger=" << linger << ")\n" );
    ::setsockopt( socket, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof l );
    //Auf Fehlerprüfung verzichten wir. Wird auch von ~Sos_socket_manager gerufen.
}

//----------------------------------------------------------------Sos_socket_manager::ioctl_fionbio

void Sos_socket_manager::ioctl_fionbio( SOCKET socket )
{
    unsigned long on = 1;
    LOG( "ioctl(" << socket << ",FIONBIO,&1)\n" );
    int rc = ::ioctlsocket( socket, FIONBIO, &on );
    if( rc == SOCKET_ERROR )  throw_sos_socket_error( "ioctl(FIONBIO)", this );
}

//-----------------------------------------------------------------------Sos_socket_manager::listen

void Sos_socket_manager::listen( int port )
{
    int rc;

    LOG( "socket(AF_INET,SOCK_STREAM,0)\n" );
    SOCKET socket = ::socket( AF_INET, SOCK_STREAM, 0 );
    if( socket == (SOCKET)SOCKET_ERROR )  throw_sos_socket_error( "socket", this );

    setsockopt_std( socket );

    _listen_socket = socket;

#   if defined SOSSOCK_WND
        rc =
#       if defined SYSTEM_WIN16
                 _socket_manager_ptr->_functions.
#       endif
                 WSAAsyncSelect( _listen_socket, hwnd(), socket_msg,
                                 FD_ACCEPT | FD_CONNECT | FD_READ | FD_WRITE /*| FD_OOB */| FD_CLOSE );
        if( rc == SOCKET_ERROR )  throw_sos_socket_error( "WSAAsyncSelect", this );
#   else
        ioctl_fionbio( _socket_manager_ptr->_listen_socket );
#   endif

    struct sockaddr_in sa;
    //int                addrlen;

#   if defined SYSTEM_WIN
        sa.sin_port        = ::htons( port );
#    else
        sa.sin_port        = htons( port );
#   endif

    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = 0; /* INADDR_ANY */

    set_linger( _listen_socket, 0, 0 );

    BOOL true_ = 1;
    LOG( "setsockopt(" << _listen_socket<< ",SOL_SOCKET,SO_REUSEADDR,true)\n" );
    ::setsockopt( _listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&true_, sizeof true_ );

    if( sa.sin_port ) {
        LOG( "bind(" << _listen_socket << ",AF_INET " << sa.sin_port << ")\n" );
        rc = ::bind( _listen_socket, (struct sockaddr*)&sa, sizeof sa );
        if( rc == SOCKET_ERROR )  throw_sos_socket_error( "bind", this );
    }

    LOG( "listen(" << _listen_socket << ",5)\n" );
    rc = ::listen( _listen_socket, 5 );
    if( rc == SOCKET_ERROR )  throw_sos_socket_error( "listen", this );
}

//-----------------------------------------------------------Sos_socket_manager::setsockopt_std

void Sos_socket_manager::setsockopt_std( SOCKET socket )
{
    int err;
    int buffsize = (int)MIN( (long)INT_MAX, 35000L );

    LOG( "setsockopt(" << socket << ",SOL_SOCKET,SO_RCVBUF," << buffsize << ")\n" );
    err = :: setsockopt( socket, SOL_SOCKET, SO_RCVBUF, (char*)&buffsize, sizeof buffsize);
    if( err )  LOG( "setsockopt(SO_RCVBUF," << buffsize << ") FAILED\n" );

    LOG( "setsockopt(" << socket << ",SOL_SOCKET,SO_SNDBUF," << buffsize << ")\n" );
    :: setsockopt( socket, SOL_SOCKET, SO_SNDBUF, (char*)&buffsize, sizeof buffsize);
    if( err )  LOG( "setsockopt(SO_SNDBUF," << buffsize << ") FAILED\n" );
}

//-----------------------------------------------------------------Sos_socket_reverse::_obj_msg

void Sos_socket_reverse::_obj_msg( Sos_msg* m )
{
    if( m->source_ptr() == _sos_socket_ptr )
    {
        m->dest_ptr( _obj_client_ptr );
        _obj_client_ptr = 0;
        m->source_ptr( this );
        ::sos::send( m );
    }
    else
    {
        if( m->type() == msg_end ) {
            reply_ack_msg( m->source_ptr(), this );
        }
        else
        {
            if( _obj_client_ptr )  { obj_busy(); return; }      // get und put nicht gleichzeitig möglich!
            _obj_client_ptr = m->source_ptr();
            m->dest_ptr( _sos_socket_ptr );
            m->source_ptr( this );
            ::sos::send( m );
        }
    }
    //_sos_socket_ptr->_obj_msg( m );
}

//-----------------------------------------------------------------------Sos_socket::Sos_socket

Sos_socket::Sos_socket( int socket_type )
:
    _zero_ (this+1),
  //_ack_msg               ( 0, this ),
    _get_requestor_ptr     ( 0 ),
    _receive_area_ptr      ( 0 ),
    _receive_length        ( 0 ),
    _status                ( sta_none ),
    _socket_manager_ptr    ( 0 ),
    _socket_type           ( socket_type ),
    _reverse               ( this ),
    _continue              ( false ),
    _sam3                  ( false ),
    _collecting_length     ( false ),
    _buffer_size           ( 1536 )   // TCP-Blocksize
{
    obj_const_name( "Sos_socket" );

    _unuseable_errno    = 0;
    _socket             = INVALID_SOCKET;
}

//--------------------------------------------------------------------------Sos_socket::~Sos_socket

Sos_socket::~Sos_socket()
{
    if( !this )  return;  // e370 jz 22.11.96


    try {
#       if defined SOSSOCK_WND
            if( _socket != INVALID_SOCKET ) 
            {
#               if defined SYSTEM_WIN16
                    _socket_manager_ptr->_functions.
#               endif

                WSAAsyncSelect( _socket, 0, 0, 0 );
            }
#       endif

        if( _socket_manager_ptr )  {
            _socket_manager_ptr->del( this );
            _socket_manager_ptr = NULL;
        }
    }
    catch( const Xc& ) {}

    try {
        close( close_error );  // event?
    }
    catch( const Xc& ) {}

    _obj_output_ptr.del();        // Vor ~_reverse!
    _obj_reverse_filter.del();    // Verweis auf _reverse löschen vor ~_reverse
}

//-----------------------------------------------------------------------------sos_delete_array

DEFINE_SOS_DELETE_ARRAY( Sos_socket* )

//-----------------------------------------------------------------------------Sos_socket::init

void Sos_socket::init()
{
    if( !sos_static_ptr()->_socket_manager_ptr )  {
        Sos_ptr<Sos_socket_manager> p = SOS_NEW( Sos_socket_manager );
        p->init();
        sos_static_ptr()->_socket_manager_ptr = +p;
    }
}

//-----------------------------------------------------------------------Sos_socket::Sos_socket

void Sos_socket::_obj_open_msg( Open_msg* m )
{
    _obj_client_ptr = m->source_ptr();

    init();
    _socket_manager_ptr = +sos_static_ptr()->_socket_manager_ptr;

    int        rc;
    Bool       lsten = false;
    Sos_string param1, param2;
    Sos_string host;
    Sos_string port;

    {
        Sos_option_iterator o ( m->name() );
        o.max_params( 2 );

        for(; !o.end(); o.next() )
        {
            if( o.flag( "listen"   ) )  lsten = o.set();
            else
            if( o.flag( "continue" ) )  _continue = o.set();
            else
            if( o.flag( "sam3" ) )      _sam3 = o.set();       // 3 Bytes Satzlängenfeld
            else
            if( o.with_value( "buffer-size" ) )  _buffer_size = o.as_uintK();
            else
            if( o.flag( "nl" ) )        _nl = _send_nl = o.set();   // '\n' schließt Satz ab
            else
            if( o.flag( "send-nl" ) )   _send_nl = o.set();         // '\n' schließt Satz ab (nur senden)
            else
            if( o.param( 1 ) )  param1 = o.value();
            else
            if( o.param( 2 ) )  param2 = o.value();
            else throw_sos_option_error( o );
        }
    }

    if( param2 != "" ) {
        host = param1;
        port = param2;
    } else {
        const char* p = strchr( c_str( param1 ), '/' );      // host/port? (alte Syntax)
        if( p ) {
            host = as_string( c_str( param1 ), p - c_str( param1 ) );
            port = p + 1;
        } else {
            port = param1;
        }
    }

    if( lsten  &&  host != "" )  throw_xc( "Sos_socket", "Überflüssiger Parameter" );

    if( !lsten && !empty( host ) )
    {
        // Client
/*
        if(  p - name_ptr > sizeof host_name - 1
         ||  strlen( p + 1 ) > sizeof service_name - 1 )  { LOG( "Sos_socket::Sos_socket(\""<<m->name()<<"\"): "); Xc x ( "D104" ); x.insert( m->name() ); throw x; }

        memcpy( host_name, name_ptr, p - name_ptr );
        host_name [ p - name_ptr ] = '\0';
        truncate_spaces( host_name );

        strcpy( service_name, p + 1 );
        truncate_spaces( service_name );
*/
        make_socket( AF_INET, _socket_type );
      //debugging( true );

        if( _socket_type == SOCK_STREAM ) {
            BOOL bb = true;
            LOG( "setsockopt(" << _socket << ",SOL_SOCKET,SO_KEEPALIVE," << bb << ")\n" );
            ::setsockopt( _socket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&bb, sizeof bb );  //
          //buffer_size( 3/*Längenbytes*/ + file_spec.max_record_length() );
            _socket_manager_ptr->set_linger( _socket, 0, 0 );
        }

        uint4 ip_no = ::inet_addr( c_str( host ) );
        if( ip_no == INADDR_NONE ) {
            // In Solaris 2.3 stürzt gethostbyname() ab, wenn alle File handles verbraucht sind
            struct hostent* host_ptr = ::gethostbyname( c_str( host ) );      // Blockierend!
            if (host_ptr == NULL)  {
                throw_sos_socket_error( "gethostbyname", this );
            }
            ip_no = *(uint4*)host_ptr->h_addr;
        }

        //typedef Sos_socket_address::Port_no Port_no;  // Für gcc 2.5.8
        //Sos_socket_internet_address sockaddr ( ip_no, service_name );

        struct sockaddr_in sockaddr;
        sockaddr.sin_family = PF_INET;
        sockaddr.sin_addr   = *(in_addr*)&ip_no;
#       if defined SYSTEM_WIN
            sockaddr.sin_port   = :: htons( as_ushort( c_str( port ) ) );
#        else
            sockaddr.sin_port   = htons( as_int( c_str( port ) ) );
#       endif

        for( int i = 0; i < NO_OF( sockaddr.sin_zero ); i++ )  sockaddr.sin_zero[ i ] = 0;

        LOG( "connect(" << _socket << ")\n" );
        rc = ::connect( _socket, (struct sockaddr*)&sockaddr, sizeof sockaddr );

        if( rc != SOCKET_ERROR ) {
            connected();
            obj_created();
        } else {
            if( socket_errno() == WSAEWOULDBLOCK/*mswin*/  ||  socket_errno() == WSAEINPROGRESS/*unix*/ )  {
                status( sta_connecting );
            }
            else
            {
                obj_not_created( Sos_socket_error( "connect" ) );
                LOG( "close(" << _socket << ")\n" );
                ::closesocket( _socket );     // wenigstens bei WSAECONNREFUSED
            }
        }
    }
    else
    {
        // Server

        if( _socket_type != SOCK_STREAM )  throw_xc( "SOS-SERVER-ONLY-TCP" );    // UDP ist nicht als Server implementiert

        int port_no = as_int( c_str( port ) );

        if( port_no < 1024  &&  !lsten )   throw_xc( "SOS-1123", this );

        if( !_socket_manager_ptr->_listen_socket ) {
            _socket_manager_ptr->listen( port_no );
        }

        _socket = _socket_manager_ptr->_listen_socket;
        _socket_manager_ptr->_socket_count++;
        _socket_manager_ptr->add( this );


        SOCKET s = accept_connection();
        if( s != INVALID_SOCKET )
        {
            obj_created();
        }
        else
        {
            if( socket_errno() == WSAEWOULDBLOCK )  {
                status( sta_listening );
                if( _continue )  obj_created();   // open() ok, trotz fehlendem accept()
            }
            else throw_sos_socket_error( "accept", this );
        }
    }
}

//-------------------------------------------------------------------------Sos_socket::_obj_msg

SOS_BEGIN_MSG_DISPATCHER( Sos_socket )
    SOS_DISPATCH_MSG( data )
    SOS_DISPATCH_MSG( ack  )
    SOS_DISPATCH_MSG( get  )
    SOS_DISPATCH_MSG( run  )
    SOS_DISPATCH_MSG( end  )
    SOS_DISPATCH_MSG( open )
  //SOS_DISPATCH_MSG( get_port_name )
SOS_END_MSG_DISPATCHER

//-------------------------------------------------------Sos_socket::_obj_create_reverse_filter

Sos_ptr<Sos_msg_filter> Sos_socket::_obj_create_reverse_filter( Sos_msg_filter* )
{
    return &_reverse;
}

//---------------------------------------------------------------------------Sos_socket::status

void Sos_socket::status( Status sta )
{
    assert( !(_status & sta ) );

    _status = Status( _status | sta );
    _socket_manager_ptr->status( _socket, sta );

#   if defined SOSSOCK_WND
        //sos_application_ptr->add_open_event_count( 1 );
#   endif
}

//---------------------------------------------------------------------------Sos_socket::status

void Sos_socket::status_clear( Status sta )
{
    assert( ( _status & sta ) == sta );

    _status = Status( _status & ~sta );
    _socket_manager_ptr->clear_status( _socket, sta );

#   if defined SOSSOCK_WND
        //sos_application_ptr->add_open_event_count( -1 );
#   endif
}

//----------------------------------------------------------------------Sos_socket::make_socket

void Sos_socket::make_socket( int address_format, int socket_type )
{
    LOG( "socket(" << address_format << ',' << socket_type << ")\n" );
    SOCKET socket = ::socket( address_format, socket_type, 0 );
    if( socket == (SOCKET)SOCKET_ERROR )  throw_sos_socket_error( "socket", this );

    _socket_manager_ptr->setsockopt_std( socket );

    _socket = socket;

    _socket_manager_ptr->_socket_count++;
    _socket_manager_ptr->add( this );

//#   if !defined SYSTEM_WIN    // In Windows (nur 16bit?) immer asynchron, weil die
                              // Windows-Sockets alle Windows-Botschaften durchlassen.
                              // Das bekommt u.a. MS-Access schlecht.
    if( _socket_manager_ptr->_listen_socket )
//#   endif
    {
#       if defined SOSSOCK_WND
            int rc =
#               if defined SYSTEM_WIN16
                    _socket_manager_ptr->_functions.
#               endif
                    WSAAsyncSelect( _socket, _socket_manager_ptr->hwnd(),
                                     socket_msg,
                                     FD_ACCEPT | FD_CONNECT | FD_READ | FD_WRITE /*| FD_OOB */| FD_CLOSE );
            if( rc == SOCKET_ERROR )  throw_sos_socket_error( "WSAAsyncSelect", this );
#        else
            _socket_manager_ptr->ioctl_fionbio( _socket );
#       endif
    }
}

//-------------------------------------------------------------------------Sos_socket::save_peername

void Sos_socket::save_peername( struct sockaddr_in peer_addr )
{
    char buffer [ 50 ];     // "111.111.111.111 1234567890"

#   if defined SYSTEM_LINUX || defined SYSTEM_HPUX || defined Z_AIX
        sprintf( buffer, "%d.%d.%d.%d %u",
                (int)((Byte*)&peer_addr.sin_addr.s_addr) [0],
                (int)((Byte*)&peer_addr.sin_addr.s_addr) [1],
                (int)((Byte*)&peer_addr.sin_addr.s_addr) [2],
                (int)((Byte*)&peer_addr.sin_addr.s_addr) [3],
                (uint)peer_addr.sin_port );
#    else
        sprintf( buffer, "%d.%d.%d.%d %u",
                (int)peer_addr.sin_addr.S_un.S_un_b.s_b1,
                (int)peer_addr.sin_addr.S_un.S_un_b.s_b2,
                (int)peer_addr.sin_addr.S_un.S_un_b.s_b3,
                (int)peer_addr.sin_addr.S_un.S_un_b.s_b4,
                (uint)peer_addr.sin_port );
#   endif

    _peer_name = buffer;
}

//---------------------------------------------------------------------Sos_socket::accept_connection

SOCKET Sos_socket::accept_connection()
{
    struct sockaddr_in peer_addr;
    sockaddrlen_t      addrlen = sizeof peer_addr;
    SOCKET             sock;

    //_socket_manager_ptr->del( this );

    LOG( "accept(" << _socket_manager_ptr->_listen_socket << ")\n" );
    sock = ::accept( _socket_manager_ptr->_listen_socket,
                               (struct sockaddr*)&peer_addr, &addrlen );

    if( sock != (SOCKET)SOCKET_ERROR )
    {
        _socket_manager_ptr->del( this );
        _socket = sock;

        _socket_manager_ptr->ioctl_fionbio( _socket );
        _socket_manager_ptr->add( this );

        save_peername( peer_addr );
    }

    return sock;
}

//------------------------------------------------------------------------Sos_socket::connected

void Sos_socket::connected()
{
    sockaddr_in peer_addr;
    sockaddrlen_t sa_len = sizeof peer_addr;
    int rc = ::getpeername( _socket, (sockaddr*)&peer_addr, &sa_len );
    if( rc )  {
        if( socket_errno() == WSAENOTCONN )  throw_xc( "SOS-1250", "connect" );
        throw_socket_error( "getpeername" );
    }

    save_peername( peer_addr );
}


//------------------------------------------------------------------Sos_socket::mswin_message_event
#if defined SOSSOCK_WND

int Sos_socket::mswin_message_event( WORD, WORD, DWORD lParam )
{
    int ret;

    if( _socket_manager_ptr )  _socket_manager_ptr->_in_event_handler++;

    const uint2  error = WSAGETSELECTERROR( lParam );
    if( error )
    {
        LOGI( "Sos_socket_manager::mswin_message_event(): error " << dec << error << '\n' );

        if( _status == sta_none )  return 0;

        if( _status & sta_sending ) {
            status_clear( sta_sending );
            reply_error_msg( _obj_client_ptr, this, Sos_socket_error( error, "send" ) );
            _obj_client_ptr = 0;
        }

        if( _status & sta_receiving ) {
            status_clear( sta_receiving );
            reply_error_msg( _runner? _runner : _get_requestor_ptr,
                             this, Sos_socket_error( error, "recv" ) );
            _runner = 0;
        }

        if( _status & sta_connecting ) {
            status_clear( sta_connecting );
            obj_not_created( Sos_socket_error( error, "connect" ) );
        }

        if( _status & sta_listening ) {
            status_clear( sta_listening );
            obj_not_created( Sos_socket_error( error, "accept" ) );
        }

        ret = 1;
    } else {
        const uint2  e = WSAGETSELECTEVENT( lParam );

        Status sta = e == FD_READ   ? sta_receiving :
                     e == FD_WRITE  ? sta_sending :
                     e == FD_ACCEPT ? sta_listening :
                     e == FD_CONNECT? sta_connecting :
                     e == FD_CLOSE  ? sta_closing
                                    /*( _status & sta_sending? sta_sending :
                                        _status & sta_receiving? sta_receiving :
                                        _status & sta_connecting? sta_connecting :
                                        _status & sta_listening? sta_listening : sta_none )*/
                                    : sta_none;

        if( sta != sta_none ) event( sta );
                         else LOG( "Unbekanntes Socket-Ereignis: " << e );


        ret = 0;
    }

#   if 0 //def __BORLANDC__   // sossock_dispatch_self wird eigentlich von e370win.cxx auf true gesetzt.
    if( !sossock_dispatch_self ) {
        LOG( "\n\n*** sossock: sossock_dispatch_self=true;\n\n\n" );
        int SOSSOCK_DISPATCH_SELF_TRUE_FUER_WBRZ_DEMO;
        sossock_dispatch_self = true;
    }
#   endif

    if( sossock_dispatch_self ) {  // Diese Abfrage funktioniert nur mit #pragma -o-t ? 28.11.97
        static int nesting = 0;
        //LOG( "sossock: sossock_dispatch_self! nesting=" << nesting << '\n' );
        if( nesting++ == 1 ) {
            dispatch_waiting_msg();
            nesting--;
        }
    } else {
        //LOG( "sossock_dispatch_self = false\n" );
    }


    if( _socket_manager_ptr )  _socket_manager_ptr->_in_event_handler--;

    return ret;
}

#endif
//--------------------------------------------------------------------------------Sos_socket::event

void Sos_socket::event( Status sta )
{
    //LOG( "Sos_socket::event( " << sta << " )\n" );
    switch( sta )
    {
        case sta_listening:
        {
            LOGI( *this << ": FD_ACCEPT\n" );

            if( !( _status & sta_listening ) )  break;
            status_clear( sta_listening );

          //struct sockaddr_in sa;
          //int                addrlen = sizeof sa;
            SOCKET             sock;

            sock = accept_connection();

            if( sock != INVALID_SOCKET )
            {
                if( _continue ) {
                    try_send();
                    _continue = false;
                }
                else obj_created();
            }
            else
            {
                if( socket_errno() == WSAEWOULDBLOCK )  return;
                obj_not_created( Xc( Msg_code( "SOCKET-", Sos_socket::normalized_error( socket_errno() )),
                                     "accept" ));
            }
            return;
        }

        case sta_connecting:
        {
            LOGI( *this << ": FD_CONNECT\n" );

            if( !( _status & sta_connecting ) )  break;

            status_clear( sta_connecting );
            connected();
            obj_created();
            return;
        }

      //case FD_CLOSE  : s->closed_event();     return 1;

        case sta_receiving:
        {
            LOGI( *this << ": FD_READ\n" );

            if( !( _status & sta_receiving ) )  break;  // System meldet Verbindungsverlust?
            status_clear( sta_receiving );
            try_receive();
            return;
        }

        case sta_sending:
        {
            LOGI( *this << ": FD_WRITE\n" );

            if( !( _status & sta_sending ) )  break;
            status_clear( sta_sending );
            try_send();
            return;
        }

        case sta_closing:
        {
            LOGI( *this << ": FD_CLOSE\n" );

            if( _status & sta_closing ) {
                obj_reply_ack();
                return;
            }

            /// Wozu die folgenden Abfragen? jz 20.4.96
            if( _status & sta_receiving ) {
                status_clear( sta_receiving );
                try_receive();
            }

            if( _status & sta_connecting ) {
                status_clear( sta_connecting );
                obj_reply_error( Sos_socket_error( "connect" ) );
            }

            if( _status & sta_listening ) {
                status_clear( sta_listening );
                obj_reply_error( Sos_socket_error( "accept" ) );
            }

            return;
        }

        default: throw_xc( "Sos_socket::event" );
    }
}


//-----------------------------------------------------------------Sos_socket::_obj_client_name

Const_area_handle Sos_socket::_obj_client_name() const
{
    Dynamic_area name ( 50 );

    if( _socket == INVALID_SOCKET )
    {
        name = "socket:";
    }
    else
    {
        name = "tcp:";

#     if defined SYSTEM_WINxx
        int GETPEERNAME_EINBAUEN;
#      else
/*
        sockaddr_in sa;
        int         sa_len = sizeof sa;
        int rc = ::getpeername( _socket, (sockaddr*)&sa, &sa_len );
        if( !rc ) {
             ostrstream s ( name.char_ptr() + name.length(), name.size() - name.length() );

             hostent* h = ::gethostbyaddr( (const char*)&sa, sa_len, AF_INET );

             if( h )  s << h->h_name;
             else {
                s << (int)sa.sin_addr.S_un.S_un_b.s_b1 << '.'
                  << (int)sa.sin_addr.S_un.S_un_b.s_b2 << '.'
                  << (int)sa.sin_addr.S_un.S_un_b.s_b3 << '.'
                  << (int)sa.sin_addr.S_un.S_un_b.s_b4;
             }
             //s << '/' << sa.sin_port;  Der Fileserver (fs.cxx) braucht nur den Hostnamen, sonst ist's nicht mehr eindeutig
             name.length( name.length() + s.pcount() );
        }
        else
        if( socket_errno() != WSAENOTCONN )  throw_socket_error( "getpeername" );
*/
        name += _peer_name;
#     endif
    }

    return name;
}

//----------------------------------------------------------------------------Sos_socket::close

Bool Sos_socket::close_socket( Close_mode close_mode )
{
    LOGI( "Sos_socket::close\n" );

    if( _socket != INVALID_SOCKET )
    {
        if( close_mode == close_error ) {
            _socket_manager_ptr->set_linger( _socket, 0, 0 );
        }

        LOG( "Sos_socket::close: closesocket( " << _socket << " )\n");
        int rc = ::closesocket( _socket );
        if( !rc )  _socket_manager_ptr->_socket_count--;

        if( rc != SOCKET_ERROR ) {
            _socket = INVALID_SOCKET;
        } else {
            if( close_mode == close_normal ) {
                if( socket_errno() == WSAEWOULDBLOCK ) {
                    LOG( "Sos_socket::close  EWOULDBLOCK\n" );
                    status( sta_closing );
#                   if defined SOSSOCK_WND
                        if( _socket_manager_ptr->_in_event_handler ) {
                            //LOG( "closesocket wird verzögert\n" );
                            // closesocket später wiederholen:
                            //PostMessage( _socket_manager_ptr->_hwnd, socket_msg2, _socket, 0 );
                            return true;
                        }
#                   endif
                    return false;  // Quittung kommt später
                } else {
                    throw_sos_socket_error( "closesocket", this );
                }
            } else {
                LOG( "Sos_socket::close closesocket error " << socket_errno() << "\n" );
            }
        }
    }

    return true;
}

//---------------------------------------------------------------------------------Sos_socket::send

void Sos_socket::send( const Const_area& area, Send_flags flags )
{
    if ( _sam3 )
    {
        uint4 l = area.length();
        _send_data.allocate_min( l+3 );
        _send_data.byte_ptr()[ 0 ] = (Byte) ( l >> 16 );
        _send_data.byte_ptr()[ 1 ] = (Byte) ( l >> 8  );
        _send_data.byte_ptr()[ 2 ] = (Byte) ( l       );
        _send_data.length(3);
        _send_data.append( area );
    } else {
        _send_data.assign( area );
        if( _send_nl )  _send_data += '\n';
    }
    _sent_length = 0;
    _send_flags  = Send_flags(  ( ( flags & send_out_of_band )? MSG_OOB      : 0 )
                              | ( ( flags & send_dont_route  )? SO_DONTROUTE : 0 ));

    if( _send_data.length() == 0 ) {
        reply_ack_msg( _obj_client_ptr, this );
        _obj_client_ptr = 0;
    } else {
        try_send();
    }
}

//--------------------------------------------------------------------------------Sos_socket::flush
/*
void Sos_socket::flush()
{
    // Die Socket-Implementierung sieht kein sync() oder flush() vor.
}
*/
//-----------------------------------------------------------------------------Sos_socket::try_send

void Sos_socket::try_send()
{
    uint rest_length;
    int  rc;

    if( _unuseable_errno ) {
        reply_error_msg( _obj_client_ptr, this, Sos_socket_error( _unuseable_errno ) );
        _obj_client_ptr = 0;
        return;
    }

    static uint max_send_length = (uint)INT_MAX;

    rest_length = _send_data.length() - _sent_length;

    //if ( !big_buffer_allowed && rest_length > max_send_length ) throw Too_long_error( "SendData" );

    if( rest_length == 0 )  return;

    do {
        uint len = min( rest_length, (uint) max_send_length );
        LOG( "send(" << _socket << ",," << len << ")  " );
        rc = ::send( _socket,
                               _send_data.char_ptr() + _sent_length,
                               //min( rest_length, (uint) MAXINT ),
                               len,
                               _send_flags );
        LOG( rc << '\n' );
        if( rc == SOCKET_ERROR )  break;

        _sent_length += rc;
        rest_length  -= rc;

    } while( rest_length );


    if( rc != SOCKET_ERROR )
    {                                              // send() hat alles entgegengenommen
        reply_ack_msg( _obj_client_ptr, this );
        _obj_client_ptr = 0;
    }
    else
    if( socket_errno() == WSAEWOULDBLOCK              // später versuchen?
     || socket_errno() == WSAENOTCONN  &&  _continue )
    {
        status( sta_sending );
    } else {                                       // Fehler
        _unuseable_errno = socket_errno();
        _sent_length = _send_data.length();           // Daten verwerfen
        reply_error_msg( _obj_client_ptr, this, Sos_socket_error( "send" ) );
        _obj_client_ptr = 0;
    }
}

//--------------------------------------------------------------------------Sos_socket::receive

void Sos_socket::receive( Area& area, Receive_flags flags )
{
    _receive_area_ptr = &area;
    //_receive_area_ptr->length( 0 );
    _receive_flags = Receive_flags(  ( ( flags & recv_out_of_band )? MSG_OOB  : 0 )
                                   | ( ( flags & recv_peek        )? MSG_PEEK : 0 ));

    _collecting_length = _sam3;

    if ( _collecting_length ) {
        _receive_length = 3;
        _receive_sam3_area = Area( _length_bytes, 3 );
        _receive_sam3_area.length( 0 );
    }
    try_receive();
}

//--------------------------------------------------------------------------Sos_socket::try_receive

void Sos_socket::try_receive()
{
    uint len;
    uint rest_length;
    Bool eof = false;

    if( _unuseable_errno )  {
        if( _runner ) {
            reply_error_msg( _runner, this, Sos_socket_error( _unuseable_errno ) );
            _runner = 0;
        } else {
            reply_error_msg( _get_requestor_ptr, this, Sos_socket_error( _unuseable_errno ) );
            _get_requestor_ptr = 0;
        }
    }

    if( _status & ( sta_connecting | sta_listening ) ) {
        if( !( _status & sta_receiving ) )  status( sta_receiving );
        return;
    }

#   ifdef SYSTEM_LINUX
        //jz 20.9.00 _socket_manager_ptr->ioctl_fionbio( _socket );
#   endif

  recv_loop:
    if ( _collecting_length ) {
        rest_length = _receive_sam3_area.size() - _receive_sam3_area.length();
        //LOG( "Sos_socket::try_receive: collecting_length, rest_length=" << rest_length << "\n" );
    } else {
        rest_length = ( _receive_length? _receive_length : _receive_area_ptr->size() ) - _receive_area_ptr->length();
        //LOG( "Sos_socket::try_receive: data, rest_length=" << rest_length << "\n" );
    }

    if( rest_length == 0 )  return;

    do {  // Beim zweiten Durchlauf muß WSAEWOULDBLOCK kommen, wenn nicht zufällig neue Daten eingetroffen sind.
        if ( _collecting_length ) {
            LOG( "recv(" << _socket << ",," << (int)rest_length << ',' << _receive_flags << ")  " );
            len = ::recv( _socket,
                                    _receive_sam3_area.char_ptr() + _receive_sam3_area.length(),
                                    rest_length, // max. 3
                                    _receive_flags );
            LOG( (int)len << '\n' );
            if( len == (uint)SOCKET_ERROR )  break;
            if( len == 0 )  { eof = true; break; }
            _receive_sam3_area.length( _receive_sam3_area.length() + len );
            //if( log_ptr ) for( int i = 0; i < len; i++ )  {  if( i % 64 == 0 )  *log_ptr << '\n'; *log_ptr << hex << setw(2) << setfill('0') << (int)_receive_sam3_area.byte_ptr()[ i ]; }
            //LOG( "\n" );
        } else {
            if( _nl  &&  _receive_area_ptr->length() ) {
                const Byte* n = (Byte*)memchr( _receive_area_ptr->ptr(), '\n', _receive_area_ptr->length() );
                if( n ) {
                    n++;
                    _overflow_buffer.assign( n, _receive_area_ptr->byte_ptr() + _receive_area_ptr->length() - n );
                    _receive_area_ptr->length( n - _receive_area_ptr->byte_ptr() - 1 );
                    break;
                }
                _receive_area_ptr->resize_min( _receive_area_ptr->length() + 1536 );
            }

            LOG( "recv(" << _socket << ",," << rest_length << ',' << _receive_flags << ")  " );
            len = ::recv( _socket,
                                    _receive_area_ptr->char_ptr() + _receive_area_ptr->length(),
                                    min( rest_length, (uint) INT_MAX - 1000 /*recv() ist sonst überfordert bein irgendeiner winsock.dll?*/ ),
                                    _receive_flags );
            LOG( len << '\n' );

            if( len == (uint)SOCKET_ERROR )  break;
            if( len == 0 )  { eof = true; break; }

            _receive_area_ptr->length( _receive_area_ptr->length() + len );
            //if( log_ptr ) for( int i = 0; i < len; i++ )  {  if( i % 64 == 0 )  *log_ptr << '\n'; *log_ptr << hex << setw(2) << setfill('0') << (int)_receive_area_ptr->byte_ptr()[ i ]; }
            //LOG( "\n" );
        }
#if defined SYSTEM_WIN
      //LOG( "Sos_socket::try_receive()  recv("<<_socket<<",,"<<rest_length<<","<<_receive_flags<<") returns "<<len<<"\n");
#endif

        rest_length -= len;

    } while( _nl  ||  _receive_length/*0: soviel lesen wie gerade da ist*/  &&  rest_length );


    if( len != (uint)SOCKET_ERROR )
    {
        if( _collecting_length  &&  _receive_sam3_area.length() != 0 )
        {
            assert( _receive_sam3_area.length() == 3 );  // Alle Längenbytes müssen gelesen sein!
            _collecting_length = false;
            uint4 l =   ( (uint4)_receive_sam3_area.byte_ptr()[ 0 ] << 16 )  // ~= _length_bytes[0]
                      | ( (uint4)_receive_sam3_area.byte_ptr()[ 1 ] <<  8 )  // ~= _length_bytes[1]
                      | ( (uint4)_receive_sam3_area.byte_ptr()[ 2 ]       ); // ~= _length_bytes[2]

            if( (uint4)l > 34000 ) {
                LOG( "sossock: Falsche Länge empfangen: " << hex << _receive_length << dec << "." << endl );
                throw_data_error( "SOS-1124" );
            }

            _receive_length = l;

            _receive_area_ptr->allocate_min( _receive_length );
            _receive_area_ptr->length( 0 );
            if( _receive_length > 0 )  goto recv_loop;
        }
        else
        if( eof  &&  _receive_area_ptr->length() == 0 ) {           // EOF
            if( _runner ) {
                reply_ack_msg( _runner, this );
                _runner = 0;
            } else {
                reply_error_msg( _get_requestor_ptr, this, Eof_error() );
                _get_requestor_ptr = 0;
            }
            _receive_area_ptr = 0;
            return;
        }

        if( _get_requestor_ptr ) {
            _receive_buffer.length( _receive_area_ptr->length() );
            // reply( Data_reply_msg( _get_requestor_ptr, this, _receive_buffer/*_receive_area*/ ) ); // ???
            reply_data_reply_msg( _get_requestor_ptr, this, *_receive_area_ptr ); // ???
            _get_requestor_ptr = 0;
            _receive_length    = 0;
        } else {
          //LOG( "Sos_socket::obj_send: " << *obj_output_ptr() << ".ref==" << obj_output_ptr()->obj_ref_count() << "\n" );
            //LOG( "obj_send( *_receive_area_ptr )  " << _receive_area_ptr->length() << "\n" );
            obj_send( *_receive_area_ptr );
        }

        _receive_area_ptr = 0;
    }
    else    // SOCKET_ERROR
    {
        if( socket_errno() == WSAEWOULDBLOCK ) {
            status( sta_receiving );
        }
        else {
            //LOGI( "recv() returns " << len << '\n' );

            if( _receive_area_ptr->length() ) {
                //int BEI_RECV_FEHLER_WERDEN_DATEN_IGNORIERT;
            }

            _unuseable_errno = socket_errno();

            reply_error_msg( _runner? _runner : _get_requestor_ptr,
                             this, Sos_socket_error( "recv" ) );
            if( _runner )  _runner = 0;
                     else  _get_requestor_ptr = 0;
        }
    }
}

//---------------------------------------------------------------------Sos_socket::_obj_run_msg

void Sos_socket::_obj_run_msg( Run_msg* m )
{
    _runner = m->source_ptr();
    _obj_request_semaphore++;
    //receive( obj_output_ptr()->obj_input_buffer() );

    _receive_buffer.length( 0 );
    receive( _receive_buffer );
}

//--------------------------------------------------------------------Sos_socket::_obj_data_msg

void Sos_socket::_obj_data_msg( Data_msg* m )
{
    if( _sent_length < _send_data.length()  )  { obj_busy(); return; }

    _obj_client_ptr = m->source_ptr();

    send( m->data() );
}

//---------------------------------------------------------------------Sos_socket::_obj_end_msg

void Sos_socket::_obj_end_msg( End_msg* m )
{
    if( _sent_length < _send_data.length() )  { obj_busy(); return; }

    _obj_client_ptr = m->source_ptr();

    if( obj_output_ptr() ) {
        _status = Status(_status | sta_ending );                                    // s. obj_ack_msg()
        post_request( End_msg( obj_output_ptr(), this ) );
    }
    else
    {
        Bool ok = close_socket( close_normal );
        if( ok )  obj_reply_ack();
    }
}

//-------------------------------------------------------------------------Sos_socket::_obj_get_msg

void Sos_socket::_obj_get_msg( Get_msg* m )
{
    _get_requestor_ptr = m->source_ptr();
    _receive_length    = m->length();   // 0: Soviel lesen, wie gerade da ist
    assert( _sam3? _receive_length == 0 : true );
    _receive_buffer.allocate_min( _receive_length? _receive_length : _buffer_size/*TCP packet size*/ );

    _receive_buffer.length( 0 );

    if( _nl ) {
        _receive_buffer.assign( _overflow_buffer );
        _overflow_buffer.length( 0 );
    }

    receive( _receive_buffer );
}

//-------------------------------------------------------------------------Sos_socket::_obj_ack_msg

void Sos_socket::_obj_ack_msg( Ack_msg* )
{
    if( _status & sta_ending )
    {
        Bool ok = close_socket( close_normal );
        if( ok )  obj_reply_ack();
    }
    else
    {
        //receive( obj_output_ptr()->obj_input_buffer() );
        _receive_buffer.length( 0 );
        receive( _receive_buffer );
    }
}

//---------------------------------------------------------------Sos_socket::_obj_port_name_msg
/*
void Sos_socket::_obj_get_port_name_msg( Get_port_name_msg* m )
{
    sockaddr_in sa;
    int         sa_length = sizeof sockaddr_in;

    int rc = ::getsockname( _socket, (sockaddr*)&sa, &sa_length );
    reply( Data_msg( m->source_ptr(), this, ... ) );
}
*/
//---------------------------------------------------------------------------Sos_socket::_obj_print

void Sos_socket::_obj_print( ostream* s ) const
{
    if( _socket == INVALID_SOCKET ) {
        *s << "Sos_socket()";
    } else {
        *s << ( _socket_type == SOCK_STREAM? "tcp" : "udp" );
        *s << " -socket=" << dec << setw(0) << _socket;

#     if defined SYSTEM_WINxx
        int GETPEERNAME_EINBAUEN;
#      else
        *s << ' ' << _peer_name;
/*
        sockaddr_in sa;
        int         sa_len = sizeof sa;
        int rc = ::getpeername( _socket, (sockaddr*)&sa, &sa_len );
        if( !rc ) {
             *s << ' ';
             hostent* h = ::gethostbyaddr( (const char*)&sa, sa_len, AF_INET );
             if( h )  *s << h->h_name;
                else  *s << (int)sa.sin_addr.S_un.S_un_b.s_b1 << '.'
                         << (int)sa.sin_addr.S_un.S_un_b.s_b2 << '.'
                         << (int)sa.sin_addr.S_un.S_un_b.s_b3 << '.'
                         << (int)sa.sin_addr.S_un.S_un_b.s_b4;
             *s << ' ' << sa.sin_port;
        }
        else
        if( socket_errno() == WSAENOTCONN ) *s << ( _status & sta_listening? " listening"
                                                                        : ", not connected" );
*/
#     endif
    }
}

//---------------------------------------------------------------------Sos_socket::normalized_error

Sos_socket::Error_code Sos_socket::normalized_error( int wsa_error_code )
{
    for( int i = 0; i < NO_OF( socket_error_tab ); i++ ) {
        if( wsa_error_code == socket_error_tab[ i ].wsa_error_code ) {
            return Error_code( socket_error_tab[ i ].sos_error_code );
        }
    }

    return (Error_code) wsa_error_code;
}

//-------------------------------------------------------------------------------Sos_socket::select

void Sos_socket::select()
{
    Sos_socket_manager* socket_manager_ptr = sos_socket_manager_ptr();

    if( socket_manager_ptr )
    {
        socket_manager_ptr->select();
    }
}

//---------------------------------------------------------------------Sos_socket::openwin_event_count

int Sos_socket::open_event_count()
{
    Sos_socket_manager* socket_manager_ptr = sos_socket_manager_ptr();

    return socket_manager_ptr? socket_manager_ptr->_open_event_count : 0;
}

//-------------------------------------------------------------------------Sos_socket::my_host_name

#define UNKNOWN_HOSTNAME "UNKNOWN"

Sos_string Sos_socket::my_host_name()
{
    static Bool gethostname_fails = false;

    init();

    try
    {
//#       if defined SYSTEM_LINUX
//
//            struct utsname utsname_;
//            int rc = uname( &utsname_ );
//            if( rc )  return "unknown";
//
//            return Sos_string( utsname_.nodename );
//
//#       elsif !defined SYSTEM_WIN
#       if !defined SYSTEM_WIN
            char hostname [ 100 ];

            memset( hostname, 0, sizeof hostname );
            gethostname( hostname, sizeof hostname - 1 );
            return hostname;

#       else
            if( gethostname_fails )  return Sos_string( UNKNOWN_HOSTNAME );

            char hostname [ 100 ];

            Sos_static_ptr<Sos_socket_manager>& socket_manager_ptr = sos_static_ptr()->_socket_manager_ptr;
            if( !socket_manager_ptr )  {
                Sos_ptr<Sos_socket_manager> p = SOS_NEW_PTR( Sos_socket_manager );
                socket_manager_ptr = +p;
            }

            int rc = gethostname( hostname, sizeof hostname );
            if( rc )  throw_socket_error( "gethostname" );

            struct hostent* hostent_ptr = gethostbyname( hostname );
            if( !hostent_ptr )  throw_socket_error( "gethostbyname" );

            const char* p = hostent_ptr->h_name;

            while( *p  &&  *p != '.' )  p++;

            return as_string( hostent_ptr->h_name, p - hostent_ptr->h_name );
#       endif
    }
    catch( const Sos_socket_error& x )
    {
        if( !gethostname_fails ) {
            LOG_ERR( "Sos_string::my_host_name() liefert " UNKNOWN_HOSTNAME " wegen " << x << '\n' );
            SHOW_MSG( "TCP/IP hat für die IP-Nummer dieses Rechners keinen Namen.\n"
                      UNKNOWN_HOSTNAME " wird angenommen.\n"
                      "Der Name sollte in der TCP/IP-Datei 'hosts' eingetragen sein" );
        }
        gethostname_fails = true;
        return Sos_string( UNKNOWN_HOSTNAME );
    }
/*
    catch( const Sos_socket_error& )
    {
        if( !socket_manager_ptr )  throw Xc( "SOS-1121" );
        if( !socket_manager_ptr->_my_ip_no )  throw Xc( "SOS-1122" );

        char name [ 9 ];

        sprintf( name, "%02X%02X%02X%02X", ((in_addr*)h->h_addr)->S_un.S_un_b.s_b1,
                                       ((in_addr*)h->h_addr)->S_un.S_un_b.s_b2,
                                       ((in_addr*)h->h_addr)->S_un.S_un_b.s_b3,
                                       ((in_addr*)h->h_addr)->S_un.S_un_b.s_b4 );
        name[ 8 ] = '\0';

        return Sos_string( name );
    }
*/
}

} //namespace sos
