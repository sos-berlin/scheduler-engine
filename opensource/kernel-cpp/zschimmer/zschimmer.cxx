// $Id: zschimmer.cxx 14110 2010-10-27 12:58:32Z jz $        © 2000 Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

// §1693

#include "zschimmer.h"
#include "log.h"
#include "z_io.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <math.h>
#include <stdarg.h>

#ifdef Z_WINDOWS
#   include <windows.h>
#   include <io.h>
#   define isatty   _isatty
#   include "z_windows.h"
#else
#   include <unistd.h>
#   include <locale.h>
#   include <sys/time.h>
#   include <signal.h>
#endif

#ifdef __GNUC__
#   include <cxxabi.h>                  // für abi::__cxa_demangle()
#endif

#ifdef SYSTEM_HPUX
#   include <time.h>
#endif


using namespace std;


namespace zschimmer {

//----------------------------------------------------------------------------Zschimmer_thread_data
/*
struct Zschimmer_thread_data
{
    int                        _mswin_last_error;
};
*/
//-------------------------------------------------------------------------------------------static

//Mutex                           zschimmer_mutex             ( "Global Mutex" );

static Message_code_text*         error_code_text_list = NULL;

#ifndef Z_WINDOWS
    int                         main_pid = 0;
#endif

static Zschimmer_static         zschimmer_static;
bool                            unloading_module        = false;        // True zu setzen bei DllMain( DLL_PROCESS_DETACH )
char                            check_initialized_data[]= "CHECK_INITIALIZED_DATA";

//static Thread_data<Zschimmer_thread_data> thread_data;    Bindet Threads mit ein. Die sollten aber nur bei Bedarf eingebunden werden. => Extra Modul! Vielleicht com.cxx

//-------------------------------------------------------------------------------------------static

static Message_code_text error_codes[] =
{
    { "Z-4001", "Missing value (string is empty)" },
    { "Z-4002", "Number is to big for type $1: $2" },
    { "Z-4003", "Number is invalid for type $1: $2" },
    { "Z-4004", "Only a decimal point (not a comma) is accepted: $1" },
    { "Z-4005", "System command returns exit code $1: $2" },
    { "Z-4006", "System command aborts with signal $1" },
    { "Z-4007", "Not enough memory" },
    { "Z-4008", "Thread $2 tries to use an object which is owned and reserved by thread $1" },
    { "Z-4009", "Missing program filename " },
    { "Z-4010", "Invalid character '$1' in string: $2" },
    { "Z-4011", "Invalid priority class (not idle, below_normal, normal, above_normal or high): $1" },
    { "Z-4012", "No process for function $1" },
    { "Z-4013", "Invalid hexadecimal digit '$1'" },
    { "WINSOCK-10004", "WSAEINTR - Interrupted function call" },                // A blocking operation was interrupted by a call to WSACancelBlockingCall.  
    { "WINSOCK-10013", "WSAEACCES - Permission denied" },                       // An attempt was made to access a socket in a way forbidden by its access permissions. An example is using a broadcast address for sendto without broadcast permission being set using setsockopt(SO_BROADCAST).  Another possible reason for the WSAEACCES error is that when the bind function is called (on Windows NT 4 SP4 or later), another application, service, or kernel mode driver is bound to the same address with exclusive access. Such exclusive access is a new feature of Windows NT 4 SP4 and later, and is implemented by using the SO_EXCLUSIVEADDRUSE option.
    { "WINSOCK-10014", "WSAEFAULT - Bad address" },                             // The system detected an invalid pointer address in attempting to use a pointer argument of a call. This error occurs if an application passes an invalid pointer value, or if the length of the buffer is too small. For instance, if the length of an argument, which is a sockaddr structure, is smaller than the sizeof(sockaddr).  
    { "WINSOCK-10022", "WSAEINVAL - Invalid argument" },                        // Some invalid argument was supplied (for example, specifying an invalid level to the setsockopt function). In some instances, it also refers to the current state of the socket—for instance, calling accept on a socket that is not listening.  
    { "WINSOCK-10024", "WSAEMFILE - Too many open files" },                     // Too many open sockets. Each implementation may have a maximum number of socket handles available, either globally, per process, or per thread.  
    { "WINSOCK-10035", "WSAEWOULDBLOCK - Resource temporarily unavailable" },   // This error is returned from operations on nonblocking sockets that cannot be completed immediately, for example recv when no data is queued to be read from the socket. It is a nonfatal error, and the operation should be retried later. It is normal for WSAEWOULDBLOCK to be reported as the result from calling connect on a nonblocking SOCK_STREAM socket, since some time must elapse for the connection to be established.  
    { "WINSOCK-10036", "WSAEINPROGRESS - Operation now in progress" },          // A blocking operation is currently executing. Windows Sockets only allows a single blocking operation—per- task or thread—to be outstanding, and if any other function call is made (whether or not it references that or any other socket) the function fails with the WSAEINPROGRESS error.  
    { "WINSOCK-10037", "WSAEALREADY - Operation already in progress" },         // An operation was attempted on a nonblocking socket with an operation already in progress—that is, calling connect a second time on a nonblocking socket that is already connecting, or canceling an asynchronous request (WSAAsyncGetXbyY) that has already been canceled or completed.  
    { "WINSOCK-10038", "WSAENOTSOCK - Socket operation on nonsocket" },         // An operation was attempted on something that is not a socket. Either the socket handle parameter did not reference a valid socket, or for select, a member of an fd_set was not valid.  
    { "WINSOCK-10039", "WSAEDESTADDRREQ - Destination address required" },      // A required address was omitted from an operation on a socket. For example, this error is returned if sendto is called with the remote address of ADDR_ANY.  
    { "WINSOCK-10040", "WSAEMSGSIZE - Message too long" },                      // A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram was smaller than the datagram itself.  
    { "WINSOCK-10041", "WSAEPROTOTYPE - Protocol wrong type for socket" },      // A protocol was specified in the socket function call that does not support the semantics of the socket type requested. For example, the ARPA Internet UDP protocol cannot be specified with a socket type of SOCK_STREAM.  
    { "WINSOCK-10042", "WSAENOPROTOOPT - Bad protocol option" },                // An unknown, invalid or unsupported option or level was specified in a getsockopt or setsockopt call.  
    { "WINSOCK-10043", "WSAEPROTONOSUPPORT -  Protocol not supported "},        // The requested protocol has not been configured into the system, or no implementation for it exists. For example, a socket call requests a SOCK_DGRAM socket, but specifies a stream protocol.  
    { "WINSOCK-10044", "WSAESOCKTNOSUPPORT - Socket type not supported" },      // The support for the specified socket type does not exist in this address family. For example, the optional type SOCK_RAW might be selected in a socket call, and the implementation does not support SOCK_RAW sockets at all.  
    { "WINSOCK-10045", "WSAEOPNOTSUPP - Operation not supported" },             // The attempted operation is not supported for the type of object referenced. Usually this occurs when a socket descriptor to a socket that cannot support this operation is trying to accept a connection on a datagram socket.  
    { "WINSOCK-10046", "WSAEPFNOSUPPORT - Protocol family not supported" },     // The protocol family has not been configured into the system or no implementation for it exists. This message has a slightly different meaning from WSAEAFNOSUPPORT. However, it is interchangeable in most cases, and all Windows Sockets functions that return one of these messages also specify WSAEAFNOSUPPORT.  
    { "WINSOCK-10047", "WSAEAFNOSUPPORT - Address family not supported by protocol family" },  // An address incompatible with the requested protocol was used. All sockets are created with an associated address family (that is, AF_INET for Internet Protocols) and a generic protocol type (that is, SOCK_STREAM). This error is returned if an incorrect protocol is explicitly requested in the socket call, or if an address of the wrong family is used for a socket, for example, in sendto.  
    { "WINSOCK-10048", "WSAEADDRINUSE - Address already in use" },              // Typically, only one usage of each socket address (protocol/IP address/port) is permitted. This error occurs if an application attempts to bind a socket to an IP address/port that has already been used for an existing socket, or a socket that was not closed properly, or one that is still in the process of closing. For server applications that need to bind multiple sockets to the same port number, consider using setsockopt (SO_REUSEADDR). Client applications usually need not call bind at all—connect chooses an unused port automatically. When bind is called with a wildcard address (involving ADDR_ANY), a WSAEADDRINUSE error could be delayed until the specific address is committed. This could happen with a call to another function later, including connect, listen, WSAConnect, or WSAJoinLeaf.  
    { "WINSOCK-10049", "WSAEADDRNOTAVAIL - Cannot assign requested address" },  // The requested address is not valid in its context. This normally results from an attempt to bind to an address that is not valid for the local computer. This can also result from connect, sendto, WSAConnect, WSAJoinLeaf, or WSASendTo when the remote address or port is not valid for a remote computer (for example, address or port 0).  
    { "WINSOCK-10050", "WSAENETDOWN - Network is down" },                       // A socket operation encountered a dead network. This could indicate a serious failure of the network system (that is, the protocol stack that the Windows Sockets DLL runs over), the network interface, or the local network itself.  
    { "WINSOCK-10051", "WSAENETUNREACH - Network is unreachable" },             // A socket operation was attempted to an unreachable network. This usually means the local software knows no route to reach the remote host.  
    { "WINSOCK-10052", "WSAENETRESET - Network dropped connection on reset" },  // The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress. It can also be returned by setsockopt if an attempt is made to set SO_KEEPALIVE on a connection that has already failed.  
    { "WINSOCK-10053", "WSAECONNABORTED - Software caused connection abort" },  // An established connection was aborted by the software in your host computer, possibly due to a data transmission time-out or protocol error.  
    { "WINSOCK-10054", "WSAECONNRESET - Connection reset by peer" },            // An existing connection was forcibly closed by the remote host. This normally results if the peer application on the remote host is suddenly stopped, the host is rebooted, the host or remote network interface is disabled, or the remote host uses a hard close (see setsockopt for more information on the SO_LINGER option on the remote socket). This error may also result if a connection was broken due to keep-alive activity detecting a failure while one or more operations are in progress. Operations that were in progress fail with WSAENETRESET. Subsequent operations fail with WSAECONNRESET.  
    { "WINSOCK-10055", "WSAENOBUFS - No buffer space available" },              // An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.  
    { "WINSOCK-10056", "WSAEISCONN - Socket is already connected" },            // A connect request was made on an already-connected socket. Some implementations also return this error if sendto is called on a connected SOCK_DGRAM socket (for SOCK_STREAM sockets, the to parameter in sendto is ignored) although other implementations treat this as a legal occurrence.  
    { "WINSOCK-10057", "WSAENOTCONN - Socket is not connected" },               // A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using sendto) no address was supplied. Any other type of operation might also return this error—for example, setsockopt setting SO_KEEPALIVE if the connection has been reset.  
    { "WINSOCK-10058", "Socket is not connected - Cannot send after socket shutdown" },         // A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call. By calling shutdown a partial close of a socket is requested, which is a signal that sending or receiving, or both have been discontinued.  
    { "WINSOCK-10060", "Socket is not connected - Connection timed out" },      // A connection attempt failed because the connected party did not properly respond after a period of time, or the established connection failed because the connected host has failed to respond.  
    { "WINSOCK-10061", "Socket is not connected - Connection refused" },        // No connection could be made because the target computer actively refused it. This usually results from trying to connect to a service that is inactive on the foreign host—that is, one with no server application running.  
    { "WINSOCK-10064", "Socket is not connected - Host is down" },              // A socket operation failed because the destination host is down. A socket operation encountered a dead host. Networking activity on the local host has not been initiated. These conditions are more likely to be indicated by the error WSAETIMEDOUT.  
    { "WINSOCK-10065", "Socket is not connected - No route to host" },          // A socket operation was attempted to an unreachable host. See WSAENETUNREACH.  
    { "WINSOCK-10067", "Socket is not connected - Too many processes" },        // A Windows Sockets implementation may have a limit on the number of applications that can use it simultaneously. WSAStartup may fail with this error if the limit has been reached.  
    { "WINSOCK-10091", "Socket is not connected - Network subsystem is unavailable" },  // This error is returned by WSAStartup if the Windows Sockets implementation cannot function at this time because the underlying system it uses to provide network services is currently unavailable. Users should check: 
                                                                                        // That the appropriate Windows Sockets DLL file is in the current path. 
                                                                                        // That they are not trying to use more than one Windows Sockets implementation simultaneously. If there is more than one Winsock DLL on your system, be sure the first one in the path is appropriate for the network subsystem currently loaded. 
                                                                                        // The Windows Sockets implementation documentation to be sure all necessary components are currently installed and configured correctly. 
 
    { "WINSOCK-10092", "Socket is not connected - Winsock.dll version out of range" },  // The current Windows Sockets implementation does not support the Windows Sockets specification version requested by the application. Check that no old Windows Sockets DLL files are being accessed.  
    { "WINSOCK-10093", "Socket is not connected - Successful WSAStartup not yet performed" },           // Either the application has not called WSAStartup or WSAStartup failed. The application may be accessing a socket that the current active task does not own (that is, trying to share a socket between tasks), or WSACleanup has been called too many times.  
    { "WINSOCK-10101", "Socket is not connected - Graceful shutdown in progress" },             // Returned by WSARecv and WSARecvFrom to indicate that the remote party has initiated a graceful shutdown sequence.  
    { "WINSOCK-10109", "Socket is not connected - Class type not found" },          // The specified class was not found.  
    { "WINSOCK-11001", "Socket is not connected - Host not found" },                // No such host is known. The name is not an official host name or alias, or it cannot be found in the database(s) being queried. This error may also be returned for protocol and service queries, and means that the specified name could not be found in the relevant database.  
    { "WINSOCK-11002", "Socket is not connected - Nonauthoritative host not found" },           // This is usually a temporary error during host name resolution and means that the local server did not receive a response from an authoritative server. A retry at some time later may be successful.  
    { "WINSOCK-11003", "Socket is not connected - This is a nonrecoverable error" },            // This indicates some sort of nonrecoverable error occurred during a database lookup. This may be because the database files (for example, BSD-compatible HOSTS, SERVICES, or PROTOCOLS files) could not be found, or a DNS request was returned by the server with a severe error.  
    { "WINSOCK-11004", "Socket is not connected - Valid name, no data record of requested type" },          // The requested name is valid and was found in the database, but it does not have the correct associated data being resolved for. The usual example for this is a host name-to-address translation attempt (using gethostbyname or WSAAsyncGetHostByName) which uses the DNS (Domain Name Server). An MX record is returned but no A record—indicating the host itself exists, but is not directly reachable.  
    { NULL }
};

//--------------------------------------------------------------------------------------------const

const char                      Rotating_bar::chars[4] = {'|','/','-','\\'};
const static size_t             max_insertion_length = 500;
extern const char               zschimmer_copyright[] = "\n$Id: zschimmer.cxx 14110 2010-10-27 12:58:32Z jz $ (C) 1999-2006 Zschimmer GmbH, http://www.zschimmer.com, Joacim Zschimmer\n";

//---------------------------------------------------------------Zschimmer_static::Zschimmer_static

Zschimmer_static::Zschimmer_static() 
:
    _valid(true)
{ 

/*  Ist leider zu spät. Die Umgebungsvariable muss schon bei Programmstart gesetzt sein.
#   if defined __GNUC__ && defined _DEBUG

        // Recent versions of Linux libc (later than 5.4.23) and GNU libc (2.x) include a malloc implementation which is tunable via environment variables.  When
        // MALLOC_CHECK_ is set, a special (less efficient) implementation is used which is designed to be tolerant against simple errors, such as  double  calls
        // of  free()  with  the same argument, or overruns of a single byte (off-by-one bugs).  Not all such errors can be proteced against, however, and memory
        // leaks can result.  If MALLOC_CHECK_ is set to 0, any detected heap corruption is silently ignored; if set to 1, a diagnostic is printed on stderr;  if
        // set  to  2,  abort() is called immediately.  This can be useful because otherwise a crash may happen much later, and the true cause for the problem is
        // then very hard to track down.

        getenv( "MALLOC_CHECK_" )  ||  putenv( "MALLOC_CHECK_=2" );
#   endif
*/
    add_message_code_texts( error_codes );
};

//--------------------------------------------------------------Zschimmer_static::~Zschimmer_static

Zschimmer_static::~Zschimmer_static() 
{ 
    close();
};

//--------------------------------------------------------------------------Zschimmer_static::close

void Zschimmer_static::close()
{
    if( !_valid )  return;

    //if( _java_vm )  _java_vm->close(), _java_vm = NULL;
}

//-----------------------------------------------------------------------------------zschimmer_init

void zschimmer_init()
{
    if( strcmp( check_initialized_data, "CHECK_INITIALIZED_DATA" ) != 0 )  
    {
        fprintf( stderr, "***ERROR*** Static variables are not initialized\n" );
    }

#   ifndef Z_WINDOWS
        main_pid = getpid();        // Das funktioniert, wenn wir gerade im Hauptthread sind. (Für pthreads)
#   endif

    // "all" soll diese Kategorien nicht einschließen, die müssten extra angeführt werden:
    set_log_category_explicit( "async"               );
    set_log_category_explicit( "developer"           );
    set_log_category_explicit( "function"            );
    set_log_category_explicit( "java"                );
    set_log_category_explicit( "mutex"               );
    set_log_category_explicit( "zschimmer"           );
    set_log_category_implicit( "socket.accept"      , false );
    set_log_category_implicit( "socket.close"       , false );
    set_log_category_implicit( "socket.connect"     , false );
    set_log_category_explicit( "socket.data"         );
    set_log_category_implicit( "socket.listen"      , false );
    set_log_category_implicit( "socket.setsockopt"  , false );
    set_log_category_implicit( "socket.shutdown"    , false );
    set_log_category_explicit( "windows.PeekMessage" );
}

//------------------------------------------------------------------------------zschimmer_terminate

void zschimmer_terminate()
{
    zschimmer_static.close();
}

//-----------------------------------------------------------------------------add_message_code_texts

void add_message_code_texts( Message_code_text* e )
{
    if( e[0]._next == NULL )
    {
        e[0]._next = error_code_text_list;
        error_code_text_list = e;

#       ifdef _DEBUG
            for( Message_code_text* d = error_code_text_list; d; d = d[0]._next )
            {
                for( Message_code_text* e = d; e->_code; e++ )
                {
                    strcmp( e->_code, ".........." );     // Prüfen, ob Liste mit leerem Eintrag abgeschlossen ist.
                }
            }
#       endif
    }
}

//-----------------------------------------------------------------------------------get_error_text

const char* get_error_text( const char* code )
{
    for( Message_code_text* d = error_code_text_list; d; d = d[0]._next )
    {
        for( Message_code_text* e = d; e->_code; e++ )
        {
            if( strcmp( e->_code, code ) == 0 )  return e->_text;
        }
    }

    return "";
}

//---------------------------------------------------------------------------------------_check_ptr

void _check_ptr( const void* p, uint length, const char* info )
{
    if( p == NULL )  return;
    if( length == 0 )  return;

#   if defined Z_WINDOWS
        if( IsBadWritePtr( (void*)p, length ) ) {
            throw_xc( "INVALID-POINTER", info );
        }
#   endif
}

//--------------------------------------------------------------------------------------_assert_ptr

void _assert_ptr( const void* p, uint length, const char* info )
{
    if( p == NULL ) {
        throw_xc( "NULL-POINTER", info );
    }
    _check_ptr( p, length, info );
}

//-----------------------------------------------------------AddRef_Release_implementation::Release

//STDMETHODIMP_( ULONG ) AddRef_Release_implementation::Release()
//{
//    long i = InterlockedDecrement( &_reference_count );
//    if( i == 0 )
//    {
//        AddRef();       // Damit AddRef-Release-Zyklus im Destruktur nicht zum rekursiven delete führt. 10.6.04
//        delete this;
//    }
//
//    return i;
//}

//-------------------------------------------------AddRef_Release_implementation_base::call_Release

ULONG AddRef_Release_counter::call_Release()
{
    long i = InterlockedDecrement( &_reference_count ); 

    if( i == 0 ) 
    { 
        call_AddRef();       // Damit AddRef-Release-Zyklus im Destruktur nicht zum rekursiven delete führt. 10.6.04
        // Aufrufer muss delete ausführen!
    }  
    
    return i; 
}

//---------------------------------------------------------------------------Object::obj_remove_ref
/*
long Object::obj_remove_ref()
{
    long i = InterlockedDecrement( &_obj_ref_count );
    if( i == 0 )
    {
        obj_add_ref();    // Damit AddRef-Release-Zyklus im Destruktur nicht zum rekursiven delete führt. 10.6.04
        delete this;
    }

    return i;
}
*/
//---------------------------------------------------------------------------Object::QueryInterface

STDMETHODIMP Object::QueryInterface( const IID& iid, void** result )
{
    *result = NULL;

    if( iid == IID_IUnknown 
     || iid == IID_IDispatch )
    {
        *result = this;
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

//-------------------------------------------------------------------------Object::GetTypeInfoCount
/*
STDMETHODIMP Object::GetTypeInfoCount( UINT* result )
{
    if( !result )              return E_INVALIDARG;
    if( (size_t)result == 1 )  return E_NOTIMPL;    //??? VBScript übergibt im separaten Prozess eine 1 als Pointer, in Verbindung mit einem Iterator (NewEnum spooler_task.params)!

    *result = 0;
    return S_OK;
}

//------------------------------------------------------------------------------Object::GetTypeInfo

STDMETHODIMP Object::GetTypeInfo( UINT, LCID, ITypeInfo** )
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------Object::GetIDsOfNames

STDMETHODIMP Object::GetIDsOfNames( REFIID, OLECHAR**, UINT, LCID, DISPID* )
{
    return DISP_E_UNKNOWNNAME;
}

//-----------------------------------------------------------------------------------Object::Invoke

STDMETHODIMP Object::Invoke( DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT* )
{
    return DISP_E_MEMBERNOTFOUND;
}
*/
//-----------------------------------------------------------------------------Smart_ptr::Smart_ptr
/*
Smart_ptr::Smart_ptr( const IUnknown* p )
{
    if( p ) {
        //if( !p->obj_ref_count() )  throw_xc( "Smart_ptr" );
        //((Object*)p)->obj_add_ref();
        ((IUnknown*)p)->AddRef();
    }
    _ptr = (IUnknown*)p;
}

//-------------------------------------------------------------------------------Smart_ptr::_assign

void Smart_ptr::_assign( IUnknown* src )
{
    IUnknown* p = copy( src );     // falls Smart_pointer = Object*, damit ref_count nicht vorübergehend 0 wird
    inline_del();
    _ptr = p;
}

//-----------------------------------------------------------------------------------Smart_ptr::del

void Smart_ptr::del()
{
    inline_del();
}
*/
//-------------------------------------------------------------------------------------------lcase_

void lcase_( string* str )
{
    size_t n = str->length();

    for( size_t i = 0; i < n; i++ )  (*str)[i] = (char)tolower( (unsigned char)(*str)[i] );
}

//-------------------------------------------------------------------------------------------ucase_

void ucase_( string* str )
{
    size_t n = str->length();

    for( size_t i = 0; i < n; i++ )  (*str)[i] = (char)toupper( (unsigned char)(*str)[i] );
}

//--------------------------------------------------------------------------------------------ltrim

string ltrim( const string& str )
{
    string      result;
    const char* p0 = str.c_str();
    const char* p  = p0;

    if( *p == ' ' )
    {
        while( *p == ' ' )  p++;
    
        result.assign( p, str.length() - ( p-p0 ) );
    }
    else
    {
        result = str;
    }

    return result;
}

//--------------------------------------------------------------------------------------------rtrim

string rtrim( const char* str, size_t length )
{
    const char* p0 = str;
    const char* p  = p0 + length;

    while( p > p0  &&  isspace( (unsigned char)p[-1] ) )  p--;
    return string( str, p - p0 );
}

//----------------------------------------------------------------------------------------as_string

string as_string( int64 o )
{
    char buffer [ 40 ];
    size_t n = z_snprintf( buffer, sizeof buffer, "%" Z_PRINTF_INT64, o );
    return string( buffer, n );
}

//-------------------------------------------------------------------------quoted_command_parameter
/*
string quoted_command_parameter( const string& command_parameter )
{
#   ifdef Z_WINDOWS
        return quoted_windows_command_parameter( command_parameter );
#    else
        return quoted_unix_command_parameter( command_parameter );
#   endif
}
*/
//-----------------------------------------------------------------quoted_windows_process_parameter
// Für CreateProcess()

string quoted_windows_process_parameter( const string& command_parameter )
{
    if( command_parameter.find(' ') == string::npos 
     && command_parameter.find('"') != string::npos )  return command_parameter;

    return quoted_string( command_parameter, '"', '\0' );   // '"' im String ist nicht möglich
}

//-----------------------------------------------------------------quoted_windows_command_parameter

string quoted_unix_command_parameter( const string& command_parameter )
{
    /*
    if( command_parameter.find(' ') == string::npos )  return command_parameter;

    const static characters_to_quote = " |&;()<> \"'`*?$@#\\\t\n";

    for( int i = 0; i < command_parameter.length(); i++ )
    {
        if( strchr( command_parameter[ i ], characters_to_quote ) )  goto QUOTE;
    }

    return command_parameter;

QUOTE:
    */

    string      result;
    const char* p      = command_parameter.c_str();
    const char* e      = command_parameter.c_str() + command_parameter.length();

    result.reserve( command_parameter.length() + 2 + (command_parameter.length()/10) );
    result += '"';

    while( p < e )
    {
        if( *p == '"' || *p == '\\' || *p == '$' || *p == '`' )  result += '\\';        // Siehe man bash, Quoting
        result += *p;
        p++;
    }

    result += '"';
    return result;
}

//------------------------------------------------------------------------------------quoted_string

string quoted_string( const char* text, char quote1, char quote2, size_t len )
{
    // SQL: quote1 == '"', quote2 == '"'   oder
    // C:   quote1 == '"', quote2 == '\\'
    // Windows Kommandozeile: quote1 == '"', quote2 == '\0'   ==> '"' im String ist nicht zugelassen und führt zu Exception

    string      result;
    const char* p      = text;
    const char* e      = text + len;

    result.reserve( len + 2 + (len/10) );
    result += quote1;

    if( quote1 == quote2 )
    {
        while( e - p ) 
        {
            const char* q = (const char*)memchr( p, quote2, e - p );
            if( !q )  break;
            result.append( p, q - p );
            result += quote2;
            result += quote2;
            p = q + 1;
        }

        result.append( p, e - p );
    }
    else
    {
        while( p < e )
        {
            if( *p == quote1  ||  *p == quote2 )
            {
                if( !quote2 )  throw_xc( "Z-4010", string( p, 1 ), text );
                result += quote2;
            }

            result += *p;
            p++;
        }
    }

    result += quote1;
    return result;
}

//-------------------------------------------------------------------------------------hex_to_digit

int hex_to_digit( char hex )
{
    if( isdigit( (Byte)hex ) )  return hex - '0';
    if( !isxdigit( (unsigned char)hex ) )  z::throw_xc( "Z-4013", string( hex, 1 ) );
    return toupper( (Byte)hex ) - 'A' + 10;
}

//----------------------------------------------------------------------------------------lcase_hex

string lcase_hex( const void* vp, size_t len )
{
    string result;
    result.reserve( 2 * len );

    const Byte* p = (const Byte*)vp;

    const char hex[] = "0123456789abcdef";

    for( size_t i = 0; i < len; i++ )
    {
        result += hex[ p[i] >> 4 ];
        result += hex[ p[i] & 0x0F ];
    }

    return result;
}

//----------------------------------------------------------------------------------------lcase_hex

string string_from_hex( const io::Char_sequence& seq )
{
    string result;
    result.reserve( seq.length() / 2 );

    for( size_t i = 0; i+1 < seq.length(); i += 2 )
    {
        result += (char)( hex_to_digit( seq[ i ] ) * 16 + hex_to_digit( seq[ i+1 ] ) );
    }

    return result;
}

//---------------------------------------------------------------------------------------z_snprintf

int z_snprintf( char* buffer, uint buffer_size, const char* format, ... )
{
    int result;
    va_list args;
    va_start( args, format );

    if( buffer_size == 0 )  return 0;

    strncpy( buffer, "snprintf-ERROR", buffer_size );
    buffer[ buffer_size - 1 ] = 0;  // Falls buffer_size zu kurz

    errno = 0;

#   ifdef Z_WINDOWS
        result = _vsnprintf_s( buffer, buffer_size, _TRUNCATE, format, args );
#   else
        result = vsnprintf( buffer, buffer_size, format, args );
        buffer[ buffer_size - 1 ] = '\0';
#   endif

    // Besser keine Exception auslösen bei ret=-1. Lieber eine eigene Funktion schreiben  z_snprintf_x

    return result;
}

//---------------------------------------------------------------------------------------z_strerror

string z_strerror( int errn )
{
#   ifdef Z_WINDOWS

        char buffer [ 300 ];
        int err = strerror_s( buffer, sizeof buffer, errn );
        int len = strlen( buffer );
        if( len > 0  &&  buffer[ len - 1 ] == '\n' )  len--;
        return string( buffer, len );

#    else    

        return strerror( errn );

#   endif
}

//------------------------------------------------------------------------------------printf_string

string printf_string( const char* pattern, long o )
{
    char buffer [100+1];

    memset( buffer, '\0', sizeof buffer );
    int len = z_snprintf( buffer, sizeof buffer, (char*)pattern, o );
    if( len == -1 )  len = sizeof buffer;
    return string( buffer, len );
}

//------------------------------------------------------------------------------------string_printf

string string_printf( const char* pattern, double o )
{
    char buffer [100+1];

    memset( buffer, '\0', sizeof buffer );
    int len = z_snprintf( buffer, sizeof buffer, (char*)pattern, o );
    if( len == -1 )  len = sizeof buffer;
    return string( buffer, len );
}

//---------------------------------------------------------------------length_without_trailing_char

size_t length_without_trailing_char( const char* text, size_t len, char c )
{
    const char* p = text + len;
    while( p > text && p[-1] == c )  p--;
    return p - text;

/* SOS
    // Der String wird Wortweise (int32) geprüft.
    // Dabei werden bis drei Bytes vor dem String gelesen, die müssen aber zugreifbar sein, weil sie im selben Speicherwort liegen.

    int32 cccc;

    cccc = ( (int32) c << 8 ) | c;
    cccc |= cccc << 16;

    const char* p  = text + len;
    const char* p0 = (const char*)( (size_t)p & (size_t)~3 );   // Auf int32-Grenze abrunden
    if( p0 < text )  p0 = text;
    while( p > p0  &&  p[-1] == c )  p--;

    if( p > text  &&  p[-1] == c )
    {
        if( p == p0 )
        {
            // p ist ausgerichtet, p >= text
            const char* text0 = (const char*)( (size_t)text & (size_t)~3 );  // Auf int32-Grenze abrunden
            if( *(int32*)text0 == cccc )       // (p0 < text) muss geprüft werden?
            {
                p -= 4;
                while( p > text  &&  *(int32*)p == cccc )  p -= 4;
                p += 4;
            }
            else
            {
                p -= 4;
                if( p >= text0 )  while( *(int32*)p == cccc )  p -= 4;
                p += 4;
            }
        }

        while( p > text  &&  p[-1] == c )  p--;
    }

    return p - text;
*/
}

//----------------------------------------------------------------------------------------as_uint64

uint64 as_uint64( const char* str )
{
    if( !str )  throw_xc( "Z-4001", "unsigned integer" ); 

    uint64      n = 0;
    const char* p = str;

    if( p[0] == '0'  &&  ( p[1] == 'x' || p[1] == 'x' ) )       // 0xHEX
    {
        p += 2;

        while(1)
        {
            uint digit = *p - '0';
            if( digit > 9 ) {
                digit = toupper( (Byte)*p ) - 'A';
                if( digit > 6 )  break;
                digit += 10;
            }

            if( n > UINT64_MAX / 0x10 )  goto OVERFLW;
            n *= 0x10;
            if( n > UINT64_MAX - digit )  goto OVERFLW;
            n += digit;
            p++;
        }
    }
    else
    {
        if( !isdigit( (Byte)*p ) )  throw_xc( "Z-4003", "unsigned integer", p ); 

        while( 1 )
        {
            uint digit = *p - '0';
            if( digit > 9 )  break;

            if( n > UINT64_MAX / 10 )  goto OVERFLW;
            n *= 10;
            if( n > UINT64_MAX - digit )  goto OVERFLW;
            n += digit;
            p++;
        }
    }

    while( p[0] == ' ' )  p++;
    if( p[0] != '\0' ) throw_xc( "Z-4003", "unsigned integer", str );

    return n;

  OVERFLW:
  	throw_overflow( "Z-4002", "uint64", str );
    return 0;
}

//-----------------------------------------------------------------------------------------as_int64

int64 as_int64( const char* str )
{
    if( !str )  throw_xc( "Z-4001", "signed integer" ); 

    const char* p    = str;
    int         sign = 1;

    if( *p == '+' )  p++;
    if( *p == '-' )  { sign = -1; p++; }

    uint64 n = as_uint64( p );
    if( n > (uint64)INT64_MAX + ( sign == -1? 1 : 0 ) )  throw_overflow( "Z-4002", "int64", str );
    return sign * n;
}

//-------------------------------------------------------------------------------------------as_int

int as_int( const char* str )
{
    int64 n = as_int64( str );
    if( n > (int64)INT_MAX  ||  n < -(int64)INT_MAX- 1 )  throw_overflow( "Z-4002", "int", str );
    return (int)n;
}

//------------------------------------------------------------------------------------------as_uint

uint as_uint( const char* str )
{
    uint64 n = as_uint64( str );
    if( n > (uint64)UINT_MAX )  throw_overflow( "Z-4002", "unsigned int", str );
    return (uint)n;
}

//-----------------------------------------------------------------------------------------as_int16

int16 as_int16( const char* str )
{
    int64 n = as_int64( str );
    if( n > (int64)INT16_MAX  ||  n < -(int64)INT16_MAX - 1 )  throw_overflow( "Z-4002", "int16", str );
    return (int16)n;
}

//----------------------------------------------------------------------------------------as_uint16

uint16 as_uint16( const char* str )
{
    uint64 n = as_uint64( str );
    if( n > UINT16_MAX )  throw_overflow( "Z-4002", "unsigned int16", str );
    return (uint16)n;
}

//----------------------------------------------------------------------------------------as_double

double as_double( const char* str )
{
    if( !str )  throw_xc( "Z-4001", "double" ); 

    char*  t;
    double a = strtod( str, &t );

    if( t == str )
    {
        while( *t == ' ' )  t++;
        if( *t == '\0' )  throw_xc( "Z-4001", "double" );
                    else  throw_xc( "Z-4003", "double", str );
    }

    while( *t == ' ' )  t++;
    if( *t != '\0' )  throw_xc( "Z-4003", str );

    return a;
}

//------------------------------------------------------------------------------------------as_bool

bool as_bool( const char* str )
{
    if( !str )  throw_xc( "Z-4001", "boolean" ); 

    if( strcmp( str, "0" ) == 0 )  return false;
    if( strcmp( str, "1" ) == 0 )  return true;

    char buffer [10+1];

    {
        size_t len = length_without_trailing_spaces( str, strlen( str ) );
        if( len > sizeof buffer - 1 )  goto FEHLER;
        for( size_t i = 0; i < len; i++ )  buffer[i] = (char)tolower( (unsigned char)str[i] );
        buffer[ len ] = '\0';
    }

    if( memcmp( buffer, "true" , 5 ) == 0 )  return true;
    if( memcmp( buffer, "false", 6 ) == 0 )  return false;

    if( memcmp( buffer, "yes"  , 4 ) == 0 )  return true;
    if( memcmp( buffer, "no"   , 3 ) == 0 )  return false;

    if( memcmp( buffer, "y"    , 2 ) == 0 )  return true;
    if( memcmp( buffer, "n"    , 2 ) == 0 )  return false;

    if( memcmp( buffer, "j"    , 2 ) == 0 )  return true;
    if( memcmp( buffer, "n"    , 2 ) == 0 )  return false;

    if( memcmp( buffer, "on"   , 3 ) == 0 )  return true;
    if( memcmp( buffer, "off"  , 4 ) == 0 )  return false;

    if( memcmp( buffer, "ja"   , 3 ) == 0 )  return true;
    if( memcmp( buffer, "nein" , 5 ) == 0 )  return false;

    if( memcmp( buffer, "wahr"  , 5 ) == 0 )  return true;
    if( memcmp( buffer, "falsch", 6 ) == 0 )  return false;

    if( memcmp( buffer, "an"   , 3 ) == 0 )  return true;
    if( memcmp( buffer, "aus"  , 4 ) == 0 )  return false;

    if( memcmp( buffer, "-1"   , 2 ) == 0 )  return true;        // OLE (VARIANT_TRUE)

  FEHLER:
    throw_xc( "Z-4003", "boolean", str );
    return false;
}

//----------------------------------------------------------------------------------Set_locale::set

void Set_locale::set( int category, const char* locale )
{ 
    if( strcmp( locale, setlocale( category, NULL ) ) != 0 )
    {
        _category = category;  
        _saved_locale = setlocale( category, locale ); 
    }
}

//------------------------------------------------------------------------------Set_locale::restore

void Set_locale::restore()
{ 
    if( _saved_locale )  
    {
        setlocale( _category, _saved_locale );
        _saved_locale = NULL; 
    }
}

//-------------------------------------------------------------------------------------set_c_locale

void set_c_locale() {
    const char* old_locale = setlocale(LC_ALL, "C");
    if (strcmp(old_locale, "C") != 0)  Z_LOG("setlocale(LC_ALL, \"C\"), previous locale was " << old_locale << "\n");
}

//-----------------------------------------------------------------------Rotating_bar::Rotating_bar

Rotating_bar::Rotating_bar( bool ok ) 
{ 
    _ok = ok && isatty( fileno(stderr) ) && isatty( fileno(stdin) );
    _i = 0;
}

//----------------------------------------------------------------------Rotating_bar::~Rotating_bar

Rotating_bar::~Rotating_bar()
{
   if( _ok )  fputs( " \x08", stderr );
}

//----------------------------------------------------------------------Rotating_bar::void operator

void Rotating_bar::operator()() 
{
    if( _ok )
    {
        char buffer [] = " \x08";
        buffer[0] = chars[ _i++ & 3 ];
        fputs( buffer, stderr );
    }
}

//----------------------------------------------------------------------------------------subst_env

string subst_env( const string& value, const Get_string_by_name_interface* get_string_by_name )
{
    if( !strchr( value.c_str(), '$' ) )  return value;      // Abkürzung


    //? Für Windows auch: "${registry:HKEY_LOCAL_MACHINE\\software\\JavaSoft\\Java Runtime Environment\\${registry:[CurrentVersion]}\\RuntimeLib}"

    string      result;
    const char* p0 = value.c_str();
    const char* p  = p0;

    result.reserve( value.length() + 100 );

    while( *p )
    {
        if( p[0] == '\\'  &&  p[1] == '$' )
        {
            p += 2;
            result += '$';
        }
        else
        if( *p == '$' )
        {
            p++;
            string name;
            
            if( *p == '{' )
            {
                p++;
                int e = value.find( '}', p - p0 );
                if( e == string::npos ) { p -= 2; break; }    // Fehler: Schließende Klammer
                name.assign( p, e - ( p - p0 ) );
                p = p0 + e + 1;
            }
            else
            {
                if( isalpha( (Byte)*p ) )
                {
                    while( isalnum( (Byte)*p )  ||  *p == '_' )  name += *p,  p++;
                }
            }

            if( name == "" )
            {
                result += '$';      // "${}" wird zu "$" ersetzt. Das war nicht beabsichtigt, aber wird jetzt so benutzt.
            }
            else
            {
                bool name_found = false;
                if( get_string_by_name )  result.append( get_string_by_name->get_string_by_name( name, &name_found ) );
                
                if( !name_found ) 
                {
                    const char* subst = getenv( name.c_str() );
                    if( subst )  result.append( subst );
                }
            }
        }
        else
            result += *p++;
    }

    result.append( p, value.length() - ( p - p0 ) );  // Rest anhängen

    return result;
}

//--------------------------------------------------------------------------------------localtime_r
#ifdef Z_WINDOWS

struct tm* localtime_r( const time_t* timep, struct tm* result )
{
#   if _MSC_VER < 1400
        *result = *localtime( timep );
#    else
        int my_errno = localtime_s( result, timep );
        if( my_errno ) 
        {
            Z_DEBUG_ONLY( DebugBreak(); )
            throw_errno( my_errno, "localtime_s" );
        }
#   endif

    return result;
}

#endif
//-----------------------------------------------------------------------------------------gmtime_r
#ifdef Z_WINDOWS

struct tm* gmtime_r( const time_t* timep, struct tm* result )
{
#   if _MSC_VER < 1400
        *result = *gmtime( timep );
#    else
        int my_errno = gmtime_s( result, timep );
        if( my_errno )  
        {
            Z_DEBUG_ONLY( DebugBreak(); )
            throw_errno( my_errno, "gmtime_s" );
        }
#   endif

    return result;
}

#endif
//-------------------------------------------------------------------------------time_t_from_gmtime

time_t time_t_from_gmtime()
{
#   if defined Z_UNIX

        timeval  tv;

        gettimeofday( &tv, NULL );
        return tv.tv_sec;

#   else

        timeb  tm;
        ftime( &tm );
        return tm.time;

#   endif
}

//-------------------------------------------------------------------------------double_from_gmtime

double double_from_gmtime()
{
#   if defined Z_UNIX

        timeval  tv;

        gettimeofday( &tv, NULL );
        return tv.tv_sec + (double)tv.tv_usec / 1e6;

#   else

        timeb  tm;
        ftime( &tm );
        return (double)tm.time + (double)tm.millitm / (double)1e3;

#   endif
}

//----------------------------------------------------------------------------double_from_localtime

double double_from_localtime()
{
#   if defined Z_LINUX

        // Linux füllt nicht time_b::dstflag

        timeval  tv;
        tm       local_tm;

        gettimeofday( &tv, NULL );
        localtime_r( &tv.tv_sec, &local_tm );
        return timegm( &local_tm ) + (double)tv.tv_usec / 1e6;

#   elif defined Z_HPUX || defined Z_SOLARIS

        timeval  tv;
        tm       local_tm;

        gettimeofday( &tv, NULL );
        localtime_r( &tv.tv_sec, &local_tm );

        return (double)tv.tv_sec + (double)tv.tv_usec / (double)1e6; // - timezone + ( local_tm.tm_isdst? 3600 : 0 );

#   else

        timeb  tm;
        ftime( &tm );
        return (double)tm.time + (double)tm.millitm / (double)1e3;

#   endif
}

//----------------------------------------------------------------------------localtime_from_gmtime

double localtime_from_gmtime( double gmtime )
{
    timeval  tv = timeval_from_seconds( gmtime );
    return localtime_from_gmtime( (time_t)tv.tv_sec ) + (double)tv.tv_usec / 1e6;
}

//----------------------------------------------------------------------------localtime_from_gmtime

time_t localtime_from_gmtime( time_t gmtime )
{
    if( gmtime >= time_max )
    {
        if( gmtime > time_max )  Z_LOG( "*** localtime_from_gmtime(" << gmtime << " > time_max!) ***\n" );
        return time_max;
    }


    tm local_tm;

#   if defined Z_LINUX

        localtime_r( &gmtime, &local_tm );
        return timegm( &local_tm );

#   elif defined Z_HPUX || defined Z_SOLARIS || defined Z_AIX

        localtime_r( &gmtime, &local_tm );
        return gmtime - timezone + ( local_tm.tm_isdst? 3600 : 0 );

#   else

        errno_t errn = localtime_s( &local_tm, &gmtime );
        if( errn )  
        {
            Z_DEBUG_ONLY( DebugBreak(); )
            throw_errno( errn, "localtime_s" );     // Das sollte aber nicht passieren!
        }

        long my_timezone = 0;
        errn = _get_timezone( &my_timezone );
        if( errn )  
        {
            Z_DEBUG_ONLY( DebugBreak(); )
            throw_errno( errn, "_get_timezone" );     // Das sollte aber nicht passieren!
        }

        return gmtime - my_timezone + ( local_tm.tm_isdst? 3600 : 0 );

#   endif
}

//----------------------------------------------------------------------------gmtime_from_localtime

double gmtime_from_localtime( double localtime )
{
    time_t localtime_int = (time_t)localtime;
    double fraction      = localtime - localtime_int;

    return gmtime_from_localtime( localtime_int ) + fraction;
}

//----------------------------------------------------------------------------gmtime_from_localtime
// Nicht getestet

time_t gmtime_from_localtime( time_t localtime )
{
    if( localtime >= time_max )
    {
        if( localtime > time_max )  Z_LOG( "*** gmtime_from_localtime(" << localtime << " > time_max!) ***\n" );
        return time_max;
    }


    tm gm_tm;

//#   if defined Z_LINUX
//
//        gmtime_r( &localtime, &gm_tm );
//        return timegm( &gm_tm );
//
//#   if defined Z_HPUX || defined Z_SOLARIS 
#   if defined Z_UNIX

        gmtime_r( &localtime, &gm_tm );
        return localtime + timezone - ( gm_tm.tm_isdst? 3600 : 0 );

#   else

        errno_t errn = gmtime_s( &gm_tm, &localtime );
        if( errn ) 
        {
            Z_DEBUG_ONLY( DebugBreak(); )
            throw_errno( errn, "gmtime_s" );     // Das sollte aber nicht passieren!
        }

        long my_timezone = 0;
        errn = _get_timezone( &my_timezone );
        if( errn )  
        {
            Z_DEBUG_ONLY( DebugBreak(); )
            throw_errno( errn, "_get_timezone" );     // Das sollte aber nicht passieren!
        }

        return localtime + my_timezone - ( gm_tm.tm_isdst? 3600 : 0 );

#   endif
}

//-------------------------------------------------------------------------string_local_from_time_t

string string_local_from_time_t( time_t time )
{
    return string_local_strftime( "%Y-%m-%d %H:%M:%S", time );
}

//-------------------------------------------------------------------------string_local_from_time_t

string string_local_from_time_t( double time )
{
    return string_local_strftime( "%Y-%m-%d %H:%M:%S", time );
}

//----------------------------------------------------------------------------string_local_strftime

string string_local_strftime( const string& format, time_t time )
{
    return string_strftime( format, local_tm_from_time_t( time ) );
}

//----------------------------------------------------------------------------string_local_strftime

string string_local_strftime( const string& format, double time )
{
    char   buffer [ 50 ];
    string result;

    result.reserve( format.size() + 4 );

    return string_strftime( format, local_tm_from_time_t( (time_t) time ) ) 
         + ( buffer + z_snprintf( buffer, sizeof buffer, ".%0.3lf", time ) - 4 );
}

//---------------------------------------------------------------------------string_gmt_from_time_t

string string_gmt_from_time_t( time_t time )
{
    return string_gmt_strftime( "%Y-%m-%d %H:%M:%S", time );
}

//----------------------------------------------------------------------------string_local_strftime

string string_gmt_strftime( const string& format, time_t time )
{
    return string_strftime( format, tm_from_time_t( time ) );
}

//----------------------------------------------------------------------------------string_strftime

string string_strftime( const string& format, const struct tm& tm )
{
    char      buffer [ 100+1 ];
    int length = strftime( buffer, sizeof buffer, format.c_str(), &tm );
    return string( buffer, length );
}

//-----------------------------------------------------------------------------local_tm_from_time_t

struct tm local_tm_from_time_t( time_t time )
{
    struct tm tm;

    memset( &tm, 0, sizeof tm );
    localtime_r( &time, &tm );

    return tm;
}

//-----------------------------------------------------------------------------------tm_from_time_t

struct tm tm_from_time_t( time_t time )
{
    struct tm tm;

    memset( &tm, 0, sizeof tm );
    gmtime_r( &time, &tm );

    return tm;
}

//---------------------------------------------------------------------------------current_timezone
// MEZ liefert 3600, MESZ liefert 7200
/*
int current_timezone()
{
#   if defined SYSTEM_LINUX

        // Linux füllt nicht time_b::dstflag

        timeval  tv;
        tm       local_tm;

        gettimeofday( &tv, NULL );
        localtime_r( &tv.tv_sec, &local_tm );
        return timegm( &local_tm ) + (double)tv.tv_usec / 1e6;

#   elif defined SYSTEM_HPUX || defined SYSTEM_SOLARIS

        timeval  tv;
        tm       local_tm;

        gettimeofday( &tv, NULL );
        localtime_r( &tv.tv_sec, &local_tm );

        return timezone - ( local_tm.tm_isdst? _dstbias : 0 );

#   else

        timeb  tm;
        ftime( &tm );
        return timezone + ( tm.dstflag? _dstbias : 0 );

#   endif
}
*/
//-----------------------------------------------------------------------------timeval_from_seconds

timeval timeval_from_seconds( double seconds )
{
    timeval t;

    double sec = floor( seconds + 0.0000005 );
    t.tv_sec  = (int)( sec + 0.1 );
    t.tv_usec = (long)( ( seconds - sec ) * 1e6 );
  //t.tv_sec  = (int)seconds;
  //t.tv_usec = (long)( (int64)(seconds*1e6) - (int64)t.tv_sec*1e6 );


    //Z_LOG2( "zschimmer", "timeval_from_seconds(" << seconds << ") => tv_sec=" << t.tv_sec << " tv_usec=" << t.tv_usec << "\n" );

    // Vorsichtshalber
    if( t.tv_usec < 0      )  t.tv_usec = 0;
    if( t.tv_usec > 999999 )  t.tv_usec = 999999;

    return t;
}

//--------------------------------------------------------------------------------------------sleep

void sleep( double seconds )
{
#   ifdef Z_WINDOWS

        Sleep( (uint)ceil( seconds * 1000 ) );

#    else

        struct timespec t;
        t.tv_sec  = (time_t)floor( seconds );
        t.tv_nsec = (time_t)max( 0.0, ( seconds - floor( seconds ) ) * 1e9 );

        nanosleep( &t, NULL );

#   endif
}

//----------------------------------------------------------------------------name_of_type_info_gcc
#ifdef __GNUC__

string name_of_type_info_gcc( const type_info& ti )
{
    //return "Kein Typ in GCC 3.3.3";         // ti.name() stürzt ab, wenn über java/jni gerufen (in libhostjava.so)

    string      result;
    const char* name      = ti.name();
    int         status    = 0;
    char*       demangled = abi::__cxa_demangle( ti.name(), 0, 0, &status );

    if( demangled ) 
    {
        result = demangled;
        free( demangled );
    }
    else 
    {
        result = name;
    }

    return result;
}

#endif
//-------------------------------------------------------------------------set_environment_variable

void set_environment_variable( const string& name, const string& value )
{
#   if defined Z_WINDOWS

        Z_LOG2( "env", "SetEnvironmentVariable(\"" << name << "\",\"" << value << "\")\n" );

        BOOL ok = SetEnvironmentVariable( name.c_str(), value.c_str() );
        if( !ok )  throw_mswin( "SetEnvironmentVariable", name );

        int errn = _putenv_s( name.c_str(), value.c_str() );    // Nur für getenv()
        if( errn )  throw_errno( errn, "_putenv_s", name.c_str() );

#   elif defined Z_HPUX || defined Z_SOLARIS || defined Z_AIX

        string e = name;
        e += '=';
        e += value;

        Z_LOG2( "env", "putenv(\"" << e << "\")\n" );

        int error = putenv( strdup( e.c_str() ) );      // Das gibt ein Speicherleck, aber was sollen wir tun?
        if( error )  throw_errno( errno, "putenv", name.c_str() );

#   else

        Z_LOG2( "env", "setenv(\"" << name << "\",\"" << value << "\")\n" );

        errno = 0;
        int error = setenv( name.c_str(), value.c_str(), 1 );
        if( error )  throw_errno( errno, "setenv", name.c_str() );

#   endif
}

//-------------------------------------------------------------------------get_environment_variable

string get_environment_variable( const string& name )
{
#   ifdef Z_WINDOWS
        //_dupenv_s() verwenden!
        const char* result = getenv( name.c_str() );
        return result? result : "";
#    else
        const char* result = getenv( name.c_str() );
        return result? result : "";
#   endif
}

//------------------------------------------------------------------------------remove_password

string remove_password( const string& text_with_password, const char* replacement )
{
    string text = text_with_password;
    int    pos  = 0;

    while(1)
    {
        const char keyword[] = "password="; 
        int keyword_len = strlen(keyword);
        pos = text.find( keyword, pos );
        if( pos == string::npos )  break;

        const char* p = text.c_str() + pos + keyword_len;

        if( *p == '"' || *p == '\'' ) 
        {
            char quote = *p;

            while( (++p)[0] )
            {
                if( p[0] == '\\' )  p++;
                else
                if( p[0] == quote )  { p++;  if( p[0] != quote )  break; }
            }
        }
        else
        {
            while( *p  &&  !isspace( (Byte)*p ) )  p++;
        }

        int len = p - ( text.c_str() + pos );

        if( replacement )
        {
            pos += keyword_len;
            len -= keyword_len;
            text.replace( pos, len, replacement );
        }
        else
        {
            if( pos > 0  &&  ispunct( (unsigned char)text[pos-1] ) )  pos--, len++;
            if( pos > 0  &&  text[pos-1] == ' ' &&  ( pos+len == text.length() || text[pos+len] == ' ' ) )  pos--, len++;
            text.erase( pos, len );
        }
    }

    return text;
}

//---------------------------------------------------------------------------complete_computer_name

string complete_computer_name()
{
#   ifdef Z_WINDOWS

        // Nehmen wir den Namen des Clusters oder der echten Machine?
        // ComputerNameDnsFullyQualified: "If the local computer is a node in a cluster, lpBuffer receives the fully qualified DNS name of the cluster virtual server."
        // ComputerNamePhysicalDnsFullyQualified: "The fully qualified DNS name that uniquely identifies the computer. If the local computer is a node in a cluster, lpBuffer receives the fully qualified DNS name of the local computer, not the name of the cluster virtual server." 

        char  buffer [ 200+1 ];
        DWORD length = sizeof buffer;

        Z_LOG( "GetComputerNameEx()\n" );
        BOOL ok = GetComputerNameEx( ComputerNamePhysicalDnsFullyQualified, buffer, &length );
        if( !ok )  throw_mswin( GetLastError(), "GetComputerNameEx" );

        return string( buffer, length );
        
#    else
        S    result;
        char buffer [ 200 + 1 ];

        buffer[ sizeof buffer - 1 ] = '\0';
        
        errno = 0;
        Z_LOG( "gethostname()\n" );
        int err = gethostname( buffer, sizeof buffer - 1 );
        if( err )  throw_errno( errno, "gethostname" );
        result << buffer;

#       if !defined Z_SOLARIS && !defined Z_HPUX && !defined Z_AIX
            errno = 0;
            Z_LOG( "getdomainname()\n" );
            err = getdomainname( buffer, sizeof buffer - 1 );   // centos liefert "(null)"
            if( err )  throw_errno( errno, "getdomainname" );

            if( buffer[0]  &&  strcmp( buffer, "(none)" ) != 0 )  result << "." << buffer;
#	endif

        return result;

#   endif
}

//----------------------------------------------------------------------------------------z_memrchr

const char* z_memrchr( const char* s, char c, size_t length )
{
    if( length == 0 )  return NULL;

    const char* p = s + length - 1;

    while(1)
    {
        if( *p == c )  return p;
        if( p == s )  return NULL;
    }
}

//---------------------------------------------------------------------------truncate_with_ellipsis

string truncate_with_ellipsis( const io::Char_sequence& seq, size_t length, const string& ellipsis )
{
    string result;

    //while( my_length > 0  &&  isspace( (unsigned char)seq[ my_length - 1 ] ) )  my_length--;

    if( seq.length() <= length )  
    {
        result.assign( seq.ptr(), seq.length() );
    }
    else
    if( length < ellipsis.length() )
    {
        result = ellipsis;
    }
    else
    {
        size_t my_length    = length - ellipsis.length();
        size_t begin_length = ( my_length * 3 + 2) / 4;
        size_t end_length   = my_length - begin_length;

        result.reserve( my_length );
        result.assign( seq.ptr(), begin_length );
        result += ellipsis;
        result.append( seq.ptr() + seq.length() - end_length, end_length );
    }

    return result;
}

//---------------------------------------------------------------truncate_to_one_line_with_ellipsis

string truncate_to_one_line_with_ellipsis( const io::Char_sequence& seq, size_t length, const string& ellipsis )
{
    string      result;
    const char* p      = seq.ptr();
    const char* p_end  = seq.ptr() + seq.length();

    while( p < p_end  &&  isspace( (unsigned char)*p ) )  p++;
    while( p_end > p  &&  isspace( (unsigned char)p_end[-1] ) )  --p_end;

    const char* nl = (const char*)memchr( p, '\n', p_end - p );

    if( !nl )
    {
        result = truncate_with_ellipsis( io::Char_sequence( p, p_end - p ), length, ellipsis );
    }
    else
    {
        size_t my_length = min( length, seq.length() );
        
        my_length -= my_length < ellipsis.length()? my_length : ellipsis.length();

        size_t first_line_length = nl? nl - p : p_end - p;
        size_t last_line_length  = 0;
        while( p_end > p  &&  p_end[ -1 - last_line_length ] != '\n' )  last_line_length++;

        size_t begin_length = min( first_line_length, ( my_length * 3 + 2) / 4 );
        size_t end_length   = my_length - begin_length;

        if( end_length > last_line_length )
        {
            begin_length += end_length - last_line_length;
            if( begin_length > first_line_length )  begin_length = first_line_length;
            end_length = last_line_length;
        }

        result.reserve( begin_length + ellipsis.length() + end_length );
        result.assign( p, begin_length );
        result += ellipsis;
        result.append( p_end - end_length, end_length );
    }

    return result;
}

//------------------------------------------------------------------------test_truncate_to_ellipsis
#ifdef Z_DEBUG

void test_truncate_to_ellipsis()
{
    int n = 12;
    int m = n + 3;

    string first_line;

    for( int i = 0; i < m; i++ )
    {
        string last_line;

        for( int j = 0; j < m; j++ )
        {
            cout << '"' << truncate_to_one_line_with_ellipsis( first_line + last_line, n, ".." ) << "\"\n";
            if( last_line == "" )  last_line = "\n";
                             else  last_line += 'A' + j - 1;
        }

        first_line += 'a' + i;
    }
}
#endif

//---------------------------------------------------------------------------------------z_function

string z_function( const char* pretty_function_name )
{
    string result;

    #ifdef Z_WINDOWS
        result.reserve( strlen( pretty_function_name ) + 2 );
        result = pretty_function_name;
        //result.append( "()" );
    #else
        const char* p_end = strchr( pretty_function_name, '(' );   // Parameterliste abschneiden
        if( !p_end )  p_end = pretty_function_name + strlen( pretty_function_name );
        const char* p = p_end;
        while( p > pretty_function_name  &&  p[-1] != ' ' )  p--;   // Rückgabetyp und static usw. abschneiden
        result.reserve( p_end - p + 2 );
        result = string( p, p_end - p );
        //result.append( "()" );
    #endif

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
