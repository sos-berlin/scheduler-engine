// $Id: sossock.cxx 14130 2010-11-09 16:33:53Z ss $

#include "precomp.h"
#include "sysdep.h"
//#define MODULE_NAME "sossock"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"


#include <time.h>               // egcs setzt FD_SETSIZE, also vor #define FD_SETSIZE einziehen! jz 1.11.98

#ifdef SYSTEM_LINUX             // Ab Suse 8 nicht mehr nötig
#   define __need_timeval
#   include <bits/time.h>
#endif

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#include "../kram/sosstrg0.h"
#include "../kram/sosstrng.h"           // MFC zieht selbst windows.h
#include "../kram/sysdep.h"

#include "../kram/sossock1.h"

#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/xception.h"
#include "../kram/sosarray.h"
#include "../kram/sosobj.h"
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

#   if (defined SYSTEM_SOLARIS  && ! defined INADDR_NONE) || defined SYSTEM_WIN
        const int    INADDR_NONE = -1;          // von inet_addr()
#   endif

    int socket_errno()
    {
        return errno;
    }

#endif

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

} //namespace sos
