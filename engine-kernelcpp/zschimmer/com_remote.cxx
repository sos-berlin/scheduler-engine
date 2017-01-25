// $Id: com_remote.cxx 14221 2011-04-29 14:18:28Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

// §1669

/*
    WÜNSCHE

    Datentransfer rechnerunabhängig. Besonders double.
    Die Partner können sich zu Beginn über die Datenformate einigen.

    Strings als UTF-8.

    Datentransfer lesbarer machen: Jeden Parameter mit einem Typ und einer Länge versehen, 
    so dass ein Transfer-String lesbar ist, ohne dass die Methode bekannt ist.
    Also nicht einfach die Parameter aneinanderreihen.


    Objectserver auf anderem Rechner:

        pid bekanntgeben

        kill ermöglichen
            Der Superserver muss dann seine Kinder überwachen
            Client schickt Superserver eine UDP-Nachricht (damit Client nicht warten muss)
            Superserver prüft, ob UDP-Nachricht vom richtigen Client kommt

        XML-Schnittstelle per TCP
            Zeigt, welche Prozesse laufen
            Beendet Prozesse (ordentlich oder kill -9).

        Lastverteilung
            Wie wird ein Rechner ausgewählt?
            Man sollte den mit der geringsten Last nehmen.
            Wie erfahren wir diese Last?
            Wir müssen wohl jeden Superserver fragen.
            Client hält also ständig Verbindungen zu den Superservern
            Darüber kann er auch XML-Infos erhalten, die ins eigene show_state eingeblendet werden. (Vorsicht: Nicht warten!)

        SCHEDULER
            Der classpath muss lokal beim Server eingestellt werden.
            Sicherer ist, der Superserver lässt nur ihm bekannte Java-Klassen zum Start zu.
            Dann braucht er die spooler.xml.


    Connection Manager / Operation Manager
        Ein Objekt, dass alle Verbindungen verwaltet.
        Wird vom Spooler angelegt
        Enthält den select_thread
        Nimmt auch andere Operationen auf, z.B. Job und Task
        Enthält einen eigenen Mini-Scheduler, der auf ein Signal hin (von select) gezielt eine Operation fortsetzt
        Operationen sind vom Scheduling ausschließbar, damit der Spooler die Tasks mit eigener Prioritätensteuerung aufrufen kann
        _listen_socket ist dann zentral. Wir brauchen nur einen Port (statt alle unter 60000)


    Spooler und Events  
        Wenn es keine Threads mehr gibt (außer Comnunication für XML-Schnittstelle), brauchen wir dann noch Ereignisse?
        Auf Unix kann pthread_cond_signal() durch send() auf einen Dummy-Socket ersetzt werden.
        Dann kann auf alles mit select() gewartet werden.
        Der Dummy-Socket kann mit socketpair() angelegt werden.

    Java-Compiler
        Sollten die Module beim Start des Spoolers vom Hauptprozess übersetzt werden?

*/

#include "zschimmer.h"
#include "com.h"
#include "z_com.h"

#ifdef _WIN32
#   define _WIN32_DCOM          // Für CoCreateInstanceEx()
#   include <winsock2.h>        // Vor <winsock.h> einziehen!
#   include <objbase.h>         // Für CoCreateInstanceEx(), nach winsock2.h einziehen, mit _WIN32_DCOM
#   include <io.h>
#   include <process.h>
#endif
#if defined Z_UNIX
#   include <signal.h>
#   include <sys/resource.h>
#endif

#include "mutex.h"
#include "log.h"
#include "async_socket.h"       // Für set_socket_not_inheritable()
#include "com_server.h"
#include "com_remote.h"
#include "z_process.h"

#ifdef Z_WINDOWS
#   include "z_windows_process.h"
#endif


using namespace std;
using namespace zschimmer::file;


extern char** _argv;

#ifndef Z_WINDOWS
#   define USE_SOCKETPAIR
#endif

#ifdef Z_AIX
#   define MSG_NOSIGNAL 0
#endif

const string socket_environment_name = "__scheduler_socket";

//-------------------------------------------------------------------------------------------------

namespace zschimmer {

//-------------------------------------------------------------------------------------------Z_INIT

extern Message_code_text com_remote_messages[];

Z_INIT( z_com_remote )
{
    add_message_code_texts(com_remote_messages);
    srand( (uint)( double_from_localtime() * 1000 ) );
}

//-------------------------------------------------------------------------------------------------

namespace com {
namespace object_server {

//--------------------------------------------------------------------------------------------const

const int   connect_timeout             = 60;  //1000*24*60*60;     // Nach sovielen Sekunden warten wir nicht mehr (das brauchen wir nicht mehr)
const int   connection_buffer_size      = 50000;
const string& Connection_reset_exception::exception_name = "zschimmer::com::object_server::Connection_reset_exception";

struct Keep_alive_thread : Thread {

    private: ptr<Session> const _session;
    private: int const _timeout_seconds;
    private: Event _stop;

    public: Keep_alive_thread(Session* session, int timeout_seconds) :
        _session(session),
        _timeout_seconds(max(1, timeout_seconds))
    {
        Z_LOG2("object_server.keep_alive", Z_FUNCTION << " " << timeout_seconds << "s\n");
    }

    public: void start() {
        thread_start();
    }

    public: void stop() {
        _stop.signal("stop");
        thread_wait_for_termination();
        Z_LOG2("object_server.keep_alive", "Stopped\n");
    }

    protected: int thread_main() {
        while (!_stop.wait(_timeout_seconds)) {
            send_and_receive_keep_alive();
        }
        return 0;
    }

    private: void send_and_receive_keep_alive() {
        ptr<Simple_operation> request_operation = Z_NEW(Simple_operation(_session, NULL, "keep-alive"));
        request_operation->_output_message.write_char(msg_keep_alive);
        request_operation->_output_message.finish();
        if (request_operation->start(Connection::diffthr_thread_allowed)) {  // No need to send something if the main thread (or any other) is sending something already
            request_operation->async_finish();
            _session->connection()->pop_operation(NULL, "keep-alive");
        } else {
            Z_LOG2("object_server.keep_alive", "Sending suppressed due to other activity\n");
        }
    }
};

//---------------------------------------------------------------------------------------set_linger

static void set_linger( SOCKET socket )
{
    struct linger l; 
    
    l.l_onoff  = 1; 
    l.l_linger = 0;  // Sekunden

    setsockopt( socket, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof l );
}

//--------------------------------------------onnection_reset_exception::Connection_reset_exception

Connection_reset_exception::Connection_reset_exception( const string& code ) 
: 
    Xc( code )  
{
    set_name( exception_name );        // Für den Scheduler
}

//-------------------------------------------Connection_reset_exception::Connection_reset_exception
    
Connection_reset_exception::Connection_reset_exception( const string& code, int pid )
: 
    Xc( code, pid )  
{
    set_name( exception_name );        // Für den Scheduler
}

//---------------------------------------------------------------------------Connection::Connection

Connection::Connection( Connection_manager* m, Server* server )
: 
    _zero_(this+1),
    _manager(m),
    _remote_host( Ip_address::localhost ),
    _server(server)
{
    set_log_category_default( "socket.connect", true );

    _socket        = SOCKET_ERROR;
    _listen_socket = SOCKET_ERROR;

#   ifdef Z_WINDOWS
        WSADATA wsa_data;
        int ret = WSAStartup( 0x0101, &wsa_data );
        if( ret )  check_socket_error( ret, "WSAStartup" );

        _wsastartup_called = true;
#   endif

    _manager->add_connection( this );
}

//--------------------------------------------------------------------------Connection::~Connection

Connection::~Connection()
{
#   ifdef Z_WINDOWS
        if( _wsastartup_called  )  WSACleanup(), _wsastartup_called = false;
#   endif

    try
    {
        close();
    }
    catch( const exception& x ) 
    {
        Z_LOG( Z_FUNCTION << "  ERROR " << x.what() << "\n" );
    }

    Z_DEBUG_ONLY(assert(!_in_use_by_thread_id || _in_use_by_thread_id == current_thread_id()));
}

//--------------------------------------------------------------------------------Connection::close

void Connection::close()
{
    close__start() -> async_finish();
    close__end();
}

//---------------------------------------------------------------------------Connection::close__end

Async_operation* Connection::close__start()
{
    if( _socket != SOCKET_ERROR )  
    {
        if( !_close_abnormally )  shutdown( _socket, SHUT_WR );
        close_socket( &_socket );
    }

    if( _listen_socket != SOCKET_ERROR )
    {
        close_socket( &_listen_socket );
        _listen_socket = SOCKET_ERROR;
    }

    while( !_operation_stack.empty() )
    {
        Z_LOG( "pid=" << pid() << " Connection::close: Operation is on top of stack: " << _operation_stack.top()->async_state_text() << "\n" );
        _operation_stack.pop();
    }

    if( _manager )
    {
        if( _tcp_port )  _manager->_used_tcp_ports.unregister_port( _tcp_port );
        _manager->remove_connection( this ), _manager = NULL;
    }

    _my_operation = Z_NEW(Sync_operation);
    return _my_operation;
}

//---------------------------------------------------------------------------Connection::close__end

void Connection::close__end()
{
}

//-------------------------------------------------------------------------Connection::close_socket

int Connection::close_socket( SOCKET* s )
{
    int result = 0;

    if( *s != SOCKET_ERROR )
    {
        if( _manager )
        {
            _manager->clear_fd( Socket_manager::read_fd  , *s );
            _manager->clear_fd( Socket_manager::write_fd , *s );
            _manager->clear_fd( Socket_manager::except_fd, *s );
        }

        int ret = closesocket( *s );
        if( ret )  result = socket_errno();

        Z_LOG2( "socket", "closesocket(" << *s << ") ==> " << ret << "\n" );
         
        *s = SOCKET_ERROR;
    }

    return result;
}

//------------------------------------------------------------------------------Connection::connect

void Connection::connect( const Host_and_port& controller_address )
{
    _socket = socket( AF_INET, SOCK_STREAM, 0 );
    if( _socket == SOCKET_ERROR )  throw_socket( socket_errno(), "socket" );

    set_socket_not_inheritable( _socket );
    set_linger( _socket );

    sockaddr_in addr;
    memset( &addr, 0, sizeof addr );
    addr.sin_family = AF_INET;
    addr.sin_addr   = controller_address.ip().as_in_addr();
    addr.sin_port   = htons( controller_address.port() );


    Z_LOG2( "socket.connect", "pid=" << pid() << " connect(" << _socket << "," << controller_address << ") ...\n" );

    int ret = ::connect( _socket, (sockaddr*)&addr, sizeof addr );
    if( ret == SOCKET_ERROR ) throw_socket( socket_errno(), "connect", controller_address);

    Z_LOG2( "socket.connect", "pid=" << pid() << " connect(" << _socket << "," << controller_address << ") OK\n" );

    _manager->set_fd( Socket_manager::except_fd, _socket );
}

//----------------------------------------------------------------Connection::connect_server__start

Connection::Connect_operation* Connection::connect_server__start()
{
    ptr<Connect_operation> operation = Z_NEW( Connect_operation( this ) );
    _my_operation = +operation;
    return operation;
}

//------------------------------------------------------------------Connection::connect_server__end

void Connection::connect_server__end()
{
    ptr<Async_operation> operation = _my_operation;
    _my_operation = NULL;
    operation->async_check_error( Z_FUNCTION );

    if (_is_async) set_socket_non_blocking(_socket);
}

//-------------------------------------------------Connection::Connect_operation::Connect_operation

Connection::Connect_operation::Connect_operation( Connection* connection )
: 
    _zero_(this+1),
    _connection(connection)
{
    _timeout = time(NULL) + connect_timeout;     // Nach sovielen Sekunden warten wir nicht mehr
    set_async_next_gmtime( _timeout );
}

//------------------------------------------------Connection::Connect_operation::async_check_error_
    
void Connection::Connect_operation::async_check_error_()
{ 
    _connection->async_check_error( Z_FUNCTION );
}

//-------------------------------------------------Connection::Connect_operation::async_state_text_

string Connection::Connect_operation::async_state_text_() const
{ 
    S result;

    result << "Connection::Connect_operation(";
    result << ( _connection->connected()? "connected" : "connecting..." ); 
    result << "), ";
    result << _connection->obj_name();

    return result;
}

//---------------------------------------------------Connection::Connect_operation::async_continue_

bool Connection::Connect_operation::async_continue_( Continue_flags flags )
{
    bool something_done = false;


    switch( _state )
    {
        case s_initial:
            _state = s_waiting_for_connection;
            // Weiter im nächsten case


        case s_waiting_for_connection:
        {
            if( _connection->_socket != SOCKET_ERROR )  
            {
                // Bei socketpair()
                // Und bei _noch_ synchronem connect(): PROVISORIUM (connect() ist bereits gerufen. Es sollte aber asynchron sein!)
                something_done = true;
                _state = s_writing_to_stdin;
                //return true;        
            }
            else
            {
                if( flags & cont_wait )
                {
                    _connection->check_async( this );     // Warnung ausgeben, wenn asynchroner Betrieb verlangt ist

                    #ifdef Z_UNIX
                        struct pollfd my_pollfd;
                        my_pollfd.fd = _connection->_listen_socket;
                        my_pollfd.events = POLLIN;
                        int rc = ::poll(&my_pollfd, 1, -1);
                        if (rc == -1) z::throw_errno(errno, "poll");
                    #else
                        fd_set fds;   FD_ZERO( &fds );
                        FD_SET( _connection->_listen_socket, &fds  );                    
                        int ret = ::select( _connection->_listen_socket + 1, &fds, NULL, NULL, NULL );
                        if( ret == -1 )  _connection->_last_errno = socket_errno(), _connection->_new_error = true;
                    #endif
                }



                sockaddr_in peer_addr;
                memset( &peer_addr, 0, sizeof peer_addr );
                sockaddrlen_t len = sizeof peer_addr;

                _connection->_socket = ::accept( _connection->_listen_socket, (sockaddr*)&peer_addr, &len );

                int err = _connection->_socket == SOCKET_ERROR? socket_errno() : 0;

                Z_LOG2( err == Z_EWOULDBLOCK? "zschimmer" : "", "pid=" << _connection->pid() << " accept()  errno=" << err << "\n" );

                if( _connection->_socket == SOCKET_ERROR )  
                {
                    if( err == Z_EWOULDBLOCK )  
                    {
                        if( _connection->process_terminated() )  _connection->_process_lost = true, _connection->_new_error = true, something_done = true;
                        else
                        if( time(NULL) >= _timeout            )  _connection->_timedout     = true, _connection->_new_error = true, something_done = true;
                    }
                    else
                    {
                        _connection->_last_errno = err, _connection->_new_error = true, something_done = true;
                    }

                    //set_async_next_gmtime( _timeout );
                }
                else
                {
                    Z_LOG2("socket", "accept(" << _connection->_listen_socket << ") => " << _connection->_socket << " " << Host_and_port(peer_addr).as_string() << "\n");
                    _connection->_peer = peer_addr;

                    //Prüfung ist zu streng, wenn der Rechner im Cluster ist. Dann hat er zwei IP-Adressen.
                    //if( _connection->_peer.ip()  != _connection->_remote_host )
                    //{
                    //    _connection->close_socket( &_connection->_socket );
                    //    throw_xc( "Z-REMOTE-130", _connection->_peer.as_string() );
                    //}

                    _connection->_peer._host = _connection->_remote_host;    // Hostnamen übernehmen

                    set_socket_not_inheritable( _connection->_socket );
                    set_socket_non_blocking( _connection->_socket );
                    _connection->close_socket( &_connection->_listen_socket );
                    _connection->_manager->set_fd( Socket_manager::except_fd, _connection->_socket );
                    if( _connection->_event )  _connection->set_event( _connection->_event );

                    _state = s_writing_to_stdin;
                }
            }
        }
        if( _state != s_writing_to_stdin )  break;


        case s_writing_to_stdin:
        {
            // Windows kann nicht asynchron (mit select()) zu stdin des Prozesses schreiben. Also nehmen wir eine temporäre Datei
            _state = s_ok;
        }
        if( _state != s_ok )  break;

        case s_ok:
            break;

        default: 
            assert( !"Connection::Connect_operation::async_continue_" );
    }

    return something_done;
}

//------------------------------------------------------Connection::Connect_operation::async_abort_

void Connection::Connect_operation::async_abort_()
{
    if( _connection  &&  _connection->_my_operation == this )  _connection->_my_operation = NULL;
}

//-------------------------------------------------------Connection::Connect_operation::async_kill_

bool Connection::Connect_operation::async_kill_()
{ 
    _connection->kill_process();
    return true;
}

//-------------------------------------------------------------------Connection::listen_on_tcp_port

void Connection::listen_on_tcp_port( const Ip_address& my_ip_address )
{
    int ret = 0;

    _listen_socket = socket( AF_INET, SOCK_STREAM, 0 );
    if( _listen_socket == SOCKET_ERROR )  check_socket_error( socket_errno(), "socket" );

    set_socket_not_inheritable( _listen_socket );
    if( _event )  set_event( _event );

    set_linger( _listen_socket );


    sockaddr_in addr;
    memset( &addr, 0, sizeof addr );
    addr.sin_family = AF_INET;
    addr.sin_addr   = my_ip_address.as_in_addr();

    int errn = _manager->_used_tcp_ports.bind_free_port( _listen_socket, &addr );
    if( errn )  throw_socket( errn, "bind" );

    _tcp_port = ntohs( addr.sin_port );


    if( Log_ptr log = "socket" )
    {
        *log << "listen(" << _listen_socket << ",1)";
        
        sockaddr_in   address;
        sockaddrlen_t address_length = sizeof address;
        memset( &address, 0, sizeof address );
        int err = getsockname( _listen_socket, (sockaddr*)&address, &address_length );
        if( int errn = err? socket_errno() : 0 )  *log << ", gethostname() ==> ERRNO-" << errn;
                                            else  *log << " on " << Host_and_port( address );
        *log << "\n" << flush;
    }

    ret = listen( _listen_socket, 1 );
    if( ret == SOCKET_ERROR )  check_socket_error( socket_errno(), "listen" );

    _manager->set_fd( Socket_manager::read_fd  , _listen_socket );
    _manager->set_fd( Socket_manager::except_fd, _listen_socket );

    set_async();
}


//---------------------------------------------------------------------Connection::execute_callback

void Connection::execute_callback( Session* session, Input_message* in, Output_message* out )
{
    _callback_count++;
    _callback_nesting++;

    execute( session, in, out );

    _callback_nesting--;
}

//------------------------------------------------------------------------------Connection::execute

void Connection::execute( Session* session, Input_message* in, Output_message* out )
{
    leave_exclusive_mode(  __FUNCTION__ );
    session->execute( in, out );
    enter_exclusive_mode(  __FUNCTION__ );
}

//-----------------------------------------------------------------------Connection::push_operation
// Wird von mehreren Threads gerufen

bool Connection::push_operation( Simple_operation* operation, Different_thread_allowed different_thread_allowed )
{
    if( !different_thread_allowed )  assert_is_owners_thread();
    // Nichts am Objekt ändern, wird von verschiedenen Threads gerufen!

    Z_LOG2( "object_server.push", "pid=" << pid() << " Connection::push_operation " << operation->async_state_text() << "\n" );

    if( operation->_on_stack )  throw_xc( "Connection::push_operation", operation->async_state_text() );

    if (different_thread_allowed == diffthr_try) {
        bool entered = try_enter_exclusive_mode(__FUNCTION__);
        if (!entered) return false;
    } else {
        enter_exclusive_mode( __FUNCTION__ );     // Blockiert, wenn wir ein anderer Thread als server_loop() sind und der Server gerade keine Funktion ausführt (also im Leerlauf)
    }
    // Jetzt haben wir die Connection exklusiv und können sie ändern. 
    // KEINE EXCEPTION MEHR!, außer mit leave_exclusive_mode()


    if( !_operation_stack.empty() )
    {
        Simple_operation* top_operation = _operation_stack.top();

        if( top_operation->_callback_nesting != _callback_nesting - 1 )
        {
            leave_exclusive_mode( __FUNCTION__ );
            throw_xc( "Z-REMOTE-119", top_operation->async_state_text(), operation->async_state_text() );
        }

        if( top_operation->async_finished() )  
        {
            leave_exclusive_mode( __FUNCTION__ );
            throw_xc( "Z-REMOTE-112", top_operation->async_state_text() );
        }
    }


    _operation_stack.push( operation );

    operation->_on_stack         = true;
    operation->_callback_nesting = _callback_nesting;

    _operation_count++;
    return true;
}

//------------------------------------------------------------------------Connection::pop_operation

ptr<Simple_operation> Connection::pop_operation( const IDispatch* object, const char* method )
{
    Z_LOG2( "object_server.push", "pid=" << pid() << " Connection::pop_operation " << (void*)object << "->" << method << "\n" );

    ptr<Simple_operation> operation;

    if( _operation_stack.empty() )  throw_xc( "Z-REMOTE-121", method );
    operation = _operation_stack.top();

    if( operation->_object != object || operation->_method != method )  throw_xc( "Z-REMOTE-113", operation->_method, method );
    if( !operation->async_finished() )                                  throw_xc( "Z-REMOTE-114", method );

    _operation_stack.pop();
    operation->_on_stack = false;

    leave_exclusive_mode( __FUNCTION__ );

    operation->async_check_error( Z_FUNCTION );     // Löst Exception aus!

    return operation;
}

//--------------------------------------------------------------Connection::current_super_operation

Async_operation* Connection::current_super_operation()
{
    Async_operation* operation = current_operation();

    return operation? operation->async_super_operation()
                    : NULL;
}

//-----------------------------------------------------------------Connection::enter_exclusive_mode

void Connection::enter_exclusive_mode( const char* )
{
    //Z_LOG2( "zschimmer", Z_FUNCTION << "(\"" << debug_text << "\")\n" );

    if( !_exclusive_io_mutex.try_enter() )    // Mutex wird von pop_operation() gelöst
    {
        Z_LOG2( "object_server", Z_FUNCTION << "()  Warten ...\n" );
        _exclusive_io_mutex.enter();                                       
        Z_LOG2( "object_server", Z_FUNCTION << "()  OK\n" );
    }
    begin_exclusive_mode();
}



bool Connection::try_enter_exclusive_mode(const char*) {
    //Z_LOG2( "zschimmer", Z_FUNCTION << "(\"" << debug_text << "\")\n" );

    if( !_exclusive_io_mutex.try_enter() )    // Mutex wird von pop_operation() gelöst
        return false;
    else {
        begin_exclusive_mode();
        return true;
    }
}

void Connection::begin_exclusive_mode() {
    assert( !_in_use_by_thread_id );        //? Wird nach einem schlimmeren Fehler vielleicht pop_operation() nicht gerufen? Dann hätte der exklusive Modus verlassen müssen!!!
    _in_use_by_thread_id = current_thread_id();
}
//-----------------------------------------------------------------Connection::leave_exclusive_mode

void Connection::leave_exclusive_mode( const char* )
{
    //Z_LOG2( "zschimmer", Z_FUNCTION << "(\"" << debug_text << "\")\n" );
    assert( _in_use_by_thread_id == current_thread_id() );

    _in_use_by_thread_id = 0;
    _exclusive_io_mutex.leave();
}

//------------------------------------------------------------------Connection::assert_right_thread

void Connection::assert_right_thread()
{
    if( _in_use_by_thread_id != current_thread_id() )  
        throw_xc( "Z-REMOTE-111", current_thread_id(), _in_use_by_thread_id );
}

//-----------------------------------------------------------------------Connection::async_continue

bool Connection::async_continue()
{
    //if( _exception_signaled )  
    if( _socket != SOCKET_ERROR )  check_connection_error();


    Async_operation* operation = current_super_operation();

    if( !operation )   return false;
    
    Z_LOGI2( "object_server.continue", "pid=" << pid() << " Connection::async_continue " << operation->async_state_text() << " ...\n" );
    
    bool something_done = operation->async_continue();

    if( something_done )  Z_LOG2( "object_server.continue", "pid=" << pid() << " Connection::async_continue " << operation->async_state_text() << ", something_done!\n" );

    return something_done;
}

//--------------------------------------------------------------------Connection::async_next_gmtime

double Connection::async_next_gmtime()
{
    Async_operation* operation = current_operation(); //current_super_operation();
    return operation? operation->async_next_gmtime()
                    : double_time_max;
}

//----------------------------------------------------------------------------Connection::set_async

void Connection::set_async()
{
    if( _listen_socket != SOCKET_ERROR )  set_socket_non_blocking( _listen_socket );
    if( _socket        != SOCKET_ERROR )  set_socket_non_blocking( _socket );

    _is_async = true;
}

//--------------------------------------------------------------------Connection::check_socket_error

void Connection::check_socket_error( int err, const string& ins )
{
    if( err == Z_ECONNRESET )
    {
        if( _killed )  
        {
            throw_xc( Connection_reset_exception( "Z-REMOTE-122", pid() ) );
        }
        else
        {
            try
            {
                throw_socket( err, ins.c_str() );
            }
            catch( Xc& x ) 
            {
                Connection_reset_exception xx ( x.code() );
                xx.set_what( x.what() );
                xx.append_text( S() << "pid=" << pid() );
                throw_xc( xx );
            }
        }
    }

    throw_socket( err, ins.c_str() );
}

//----------------------------------------------------------------------------Connection::set_event

void Connection::set_event( Event* event )
{
    _event = event;

    if( _is_async )
    {
#       ifdef Z_WINDOWS
        {
            int err;

            if( _listen_socket != SOCKET_ERROR )
            {
                if( _event )  err = WSAEventSelect( _listen_socket, *_event, FD_ACCEPT );
                        else  err = WSAEventSelect( _listen_socket, NULL, 0 );
                if( err )  check_socket_error( socket_errno(), "WSAEventSelect" );
            }

            if( _socket != SOCKET_ERROR )
            {
                if( _event )  err = WSAEventSelect( _socket, *_event, FD_READ | FD_WRITE | FD_CLOSE );
                        else  err = WSAEventSelect( _socket, 0, 0 );
                if( err )  check_socket_error( socket_errno(), "WSAEventSelect" );
            }
        }
#       endif            
    }
}

//---------------------------------------------------------------------------------Connection::wait

void Connection::wait()
{
    check_async( current_operation() );     // Warnung ausgeben, wenn asynchroner Betrieb verlangt ist

    #ifdef Z_UNIX
        struct pollfd my_pollfd;
        my_pollfd.fd = _socket;
        my_pollfd.events = POLLIN | POLLOUT | POLLERR;
        int rc = ::poll(&my_pollfd, 1, -1);
        if (rc == -1) z::throw_errno(errno, "poll");
    #else
        fd_set readfds;    FD_ZERO( &readfds  );
        fd_set writefds;   FD_ZERO( &writefds );
        fd_set exceptfds;  FD_ZERO( &exceptfds );

        FD_SET( _socket, &readfds   );
        FD_SET( _socket, &writefds  );
        FD_SET( _socket, &exceptfds );
    
        int ret = ::select( _socket + 1, &readfds, &writefds, &exceptfds, NULL );
        if( ret == -1 )  check_socket_error( socket_errno(), "select" );
    #endif
}

//---------------------------------------------------------------------------Connection::send_async

int Connection::send_async( const void* data, int length )
{
    assert_right_thread();


    int written = ::send( _socket, (const char*)data, length, MSG_NOSIGNAL );

    int err = written == SOCKET_ERROR? socket_errno() 
                                    : 0;

    Z_LOG2( "socket.send", "pid=" << pid() << " send(" << _socket << ",length=" << length << ")  ret=" << written << "\n" );

    if( written == -1 ) 
    {
        if( !_is_async  ||  err != Z_EWOULDBLOCK )   _last_errno = err, _new_error = true, check_socket_error( err, "send" );
        if( process_terminated() )  _process_lost = true, _new_error = true, async_check_error( Z_FUNCTION );

        written = 0;
    }
    
    if( written < length )
        _manager->set_fd( Socket_manager::write_fd, _socket );
    else
        _manager->clear_fd( Socket_manager::write_fd, _socket );

    return written;
}

//---------------------------------------------------------------------------Connection::read_async

int Connection::read_async( void* buffer, int size, bool* eof )
{
    assert_right_thread();

    if( eof )  *eof = false;
    if( size == 0 )  return 0;

    int length = ::recv( _socket, (char*)buffer, size, MSG_NOSIGNAL );

    int err = length == SOCKET_ERROR? socket_errno() : 0;

    Z_LOG2( "socket.recv", "pid=" << pid() << " recv(" << _socket << ",size=" << size << ")  ret=" << length << " err=" << err << "\n" );

    if( length <= 0 )
    {
        if( length == 0 )
        {
            Z_LOG2( "socket.recv", "pid=" << pid() << " socket=" << _socket << " EOF\n");
            if( !eof )  throw_xc( Connection_reset_exception( "Z-REMOTE-101", pid() ) );
            *eof = true; 
        }
        else
        {
            if( !_is_async  ||  err != Z_EWOULDBLOCK )   _last_errno = err, _new_error = true, check_socket_error( err, "recv" );
            if( process_terminated() )  _process_lost = true, _new_error = true, async_check_error( Z_FUNCTION );

            _manager->set_fd( Socket_manager::read_fd, _socket );

            return 0;
        }
    }

    return length;
}

//------------------------------------------------------------------------Connection::receive_async_start

void Connection::receive_async_start( Input_message* input_message )
{
    _input_message_builder.start( input_message );
}

//------------------------------------------------------------------------------Connection::receive

void Connection::receive( Input_message* in, bool* eof )
{
    bool something_done = false;

    receive_async_start( in );

    while(1)
    {
        something_done |= receive_async( something_done? NULL : eof );
        
        if( _input_message_builder.is_complete() )  break;
        if( eof && *eof )  break;

        wait();
    }
}

//------------------------------------------------------------------------Connection::receive_async

bool Connection::receive_async( bool* eof )
{
    if( eof )  *eof = false;

    int length = read_async( _manager->_common_buffer._buffer, _manager->_common_buffer._buffer_size, eof );
    if( eof && *eof )  return false;

    _input_message_builder.add_data( _manager->_common_buffer._buffer, length );

    return length > 0;
}

//--------------------------------------------------------------------------Connection::check_async

void Connection::check_async( Async_operation* operation )
{
    if( _is_async )
    {
        string msg2;

        if( operation )
        {
            if( operation->set_async_warning_issued() )  return;  // Nur warnen, wenn nicht schon in dieser oder übergeordneter Operation geschehen

            msg2 += " IN " + operation->async_state_text();
        }

        string msg = "WARTEN TROTZ ASYNCHRONEM BETRIEB";
        msg += msg2;
        msg += "\n";

        Z_LOG( "pid=" << pid() << ' ' << msg );
        Z_DEBUG_ONLY( fputs( msg.c_str(), stderr ) );

#       if defined Z_WINDOWS
            if( IsDebuggerPresent() )  assert( !"Connection::check_async  BLOCKING WHEN IN ASYNCHRONOUS OPERATION" );
#       endif
    }
}

//---------------------------------------------------------------Connection::check_connection_error

void Connection::check_connection_error()
{
    if( !_last_errno )
    {
        char buffer [1];

#       ifdef Z_WINDOWS
            int read = recv( _socket, buffer, 0, MSG_NOSIGNAL );
            int err = read == -1? socket_errno() : 0;
            bool broken = read == -1  &&  err != Z_EWOULDBLOCK;   // Linux meldet read==0 und errno==0, wenn Prozess abgebrochen ist.
#        else
            int read = recv( _socket, buffer, 1, MSG_NOSIGNAL | MSG_PEEK );    //?? Meldet keinen Fehler, wenn Verbindung abgebrochen ist.
            int err = read == -1? socket_errno() : 0;
            bool broken = read == -1  &&  err != Z_EWOULDBLOCK  ||  read == 0;
#       endif

        Z_LOG2( "socket.recv", "pid=" << pid() << " recv(" << _socket << ",0) => " << read << "   errno=" << err << " " << z_strerror(err) << "\n" );


        if( broken )
        {
            _new_error = true;
            _last_errno = err;
            _broken = 0;
            _manager->clear_fd( Socket_manager::except_fd, _socket );

            Z_LOG( "\npid=" << pid() << " *** CONNECTION ERROR *** errno=" << _last_errno << "\n\n" );
        }


        if( process_terminated() )  _process_lost = true, _new_error = true;
    }
}

//-------------------------------------------------------------------Connection::async_check_error

void Connection::async_check_error( const string& text )
{ 
    {
        _new_error = false;

        if( _last_errno   )  check_socket_error( _last_errno, text );    //throw_socket( _last_errno, "sockets" );
        if( _timedout     )  throw_xc( "Z-REMOTE-118", pid(), connect_timeout, text );
        if( _process_lost )  throw_xc( Connection_reset_exception( _killed? "Z-REMOTE-122" : "Z-REMOTE-123", pid() ) );
    }
}

void Connection::prepare_connection_to_controller() {
    listen_on_tcp_port( Ip_address::localhost );
    _controller_address._host = Ip_address::localhost;
    _controller_address._port = _tcp_port;
}

//---------------------------------------------------------------------------Connection::short_name

string Connection::short_name() const
{ 
    S result;

    if( _peer._port )  result << _peer;
                 else  result << _remote_host;
    if (_pid) result << ",pid=" << _pid;
    return result;
}

//-----------------------------------------------------------------------------Connection::obj_name

string Connection::obj_name() const
{ 
    return S() << "object_server::Connection(" << _peer << ")"; 
}

//-----Connection_to_own_server_process::Wait_for_process_termination::Wait_for_process_termination

Connection_to_own_server_process::Wait_for_process_termination::Wait_for_process_termination( Connection_to_own_server_process* connection )
: 
    _zero_(this+1),
    _connection(connection)
{
    async_continue_( cont_default );
}

//---------------Connection_to_own_server_process::Wait_for_process_termination::async_check_error_
    
void Connection_to_own_server_process::Wait_for_process_termination::async_check_error_()
{ 
}

//------------------Connection_to_own_server_process::Wait_for_process_termination::async_continue_

bool Connection_to_own_server_process::Wait_for_process_termination::async_continue_( Continue_flags flags )
{
    bool something_done = true;

    if( _connection->pid() == 0 )  return false;

    if( flags & cont_wait )  
    {
        _connection->check_async( this );     // Warnung ausgeben, wenn asynchroner Betrieb verlangt ist
    }
    else
    {
        set_async_delay( 0.1 );  // Alle 1/10 Sekunden probieren, ob Prozess beendet ist
    }


#   ifdef Z_WINDOWS

        if( flags & cont_wait )
        {
            Z_LOG( "pid=" << _connection->pid() << " WaitForSingleObject()  ...\n" );
            DWORD ret = WaitForSingleObject( _connection->_process_handle, INFINITE );
            Z_LOG( "pid=" << _connection->pid() << " WaitForSingleObject()  ret=" <<  ret << "\n" );
        }

        _connection->_exit_code = 0;

        BOOL ok = GetExitCodeProcess( _connection->_process_handle, &_connection->_exit_code );
        if( ok )
        {
            if( _connection->_exit_code == STILL_ACTIVE )  
            {
                Z_LOG2( "zschimmer", "pid=" << pid() << " " << Z_FUNCTION << " GetExitCodeProcess() => STILL_ACTIVE\n" ); 
                return false; 
            }
            
            if( _connection->_exit_code != 0 )  Z_LOG( "pid=" << pid() << " *** GetExitCodeProcess() => " << printf_string("0x%X",_connection->_exit_code) << ", process has been terminated\n" );
                                          else  Z_LOG( "pid=" << pid() << " GetExitCodeProcess() => 0, Prozess hat geendet\n" );
        }

        _connection->_process_handle.close();
        _connection->_pid = 0;
        
        something_done = true;

#    else

        something_done |= _connection->call_waitpid( flags );

#   endif


    return something_done;
}

//----------------Connection_to_own_server_process::Wait_for_process_termination::async_state_text_

string Connection_to_own_server_process::Wait_for_process_termination::async_state_text_() const
{ 
    S result;
    result << Z_FUNCTION;

    int pid = _connection->pid(); 
    if( pid == 0 )  result << "(process terminated)";
              else  result << "(terminating process pid=" + as_string(pid) + " ...)"; 

    return result;
}

//----------------------Connection_to_own_server_process::Wait_for_process_termination::async_kill_

bool Connection_to_own_server_process::Wait_for_process_termination::async_kill_()
{ 
    _connection->kill_process();
    return true;
}

//------------------------------Connection_to_own_server_process::~Connection_to_own_server_process

Connection_to_own_server_process::~Connection_to_own_server_process()
{
    try 
    { 
        kill_process(); 
    }
    catch( const exception& x ) 
    {
        Z_LOG( Z_FUNCTION << "  ERROR " << x.what() << "\n" );
    }

#   ifdef Z_UNIX
        if( _pid )
        {
            Z_LOG( "waitpid(" << _pid << ")\n" );
            call_waitpid( true );
        }
#   endif

    // Das macht schon ~File, aber nicht in der fürs Protokoll gewünschten Reihenfolge:
    try_delete_files( NULL );
}

//--------------------------------------------------Connection_to_own_server_process::start_process
/**
 * Aus dem Haupt-Scheduler (server) heraus werden über diese Methode die anstehenden Tasks in einem eigenen
 * Prozess gestartet.
 */
void Connection_to_own_server_process::start_process( const Parameters& params )
{
    string          object_server_filename;
    vector<string>  args_vector;

    Z_FOR_EACH_CONST( Parameters, params, param )
    {
        if( param->first == "program" )  object_server_filename = param->second;
        else
        if( param->first == "java-options" )  {}        // Schon von spooler_main() ausgewertet
        else
        if( param->first == "java-classpath" )  {}     // Schon von spooler_main() ausgewertet
        else
        if( param->first == "param"   )  args_vector.push_back( param->second );
        else
        if( param->first == "keep-alive" )  { args_vector.push_back(S() << "-keep-alive=" << param->second); }
        else
            throw_xc( Z_FUNCTION, param->first );
    }


    if( object_server_filename.empty() )
    {
#       ifdef Z_WINDOWS
            char filename [ _MAX_PATH+1 ];
            int len = GetModuleFileName( NULL, filename, sizeof filename );
            if( len <= 0 )  throw_mswin( "GetModuleFileName" );
            object_server_filename = filename;
#       endif
    }


    int           ret = 0;
    int           argc;
    const char*   argv [100+2];

    Socket other_socket;
    if (!_controller_address) {
        #ifdef USE_SOCKETPAIR
            SOCKET socket_pair [2] = { -1, -1 };
    
            int ret = socketpair( PF_UNIX, SOCK_STREAM, 0, socket_pair );
            if( ret == -1 )  check_socket_error( socket_errno(), "socketpair" );

            Z_LOG2( "socket", "socketpair() ==> (" << socket_pair[0] << "," << socket_pair[1] << ")\n" );
            _socket = socket_pair[ 0 ];
            other_socket.assign( socket_pair[ 1 ] );
            //Z_LOG( "set_except_fd(" << _socket << ")\n" );
            _manager->set_fd( Socket_manager::except_fd, _socket );
        #else
            prepare_connection_to_controller();
        #endif
    }
    if( _controller_address )
    {
        args_vector.push_back( S() << "-controller=" << _controller_address._host.ip_string() << ":" 
                                                     << _controller_address._port );
    }

    if( args_vector.size() > NO_OF(argv) - 2 )  throw_xc( Z_FUNCTION, "Too many arguments" );

    argv[0] = object_server_filename.c_str();
    for( uint i = 0; i < args_vector.size(); i++ )  argv[1+i] = args_vector[i].c_str();
    argc = int_cast(args_vector.size()) + 1;
    argv[argc] = NULL;


    // Windows kann nicht asynchron (mit select()) zu stdin des Prozesses schreiben. Also nehmen wir eine temporäre Datei
    _stdin_file.open_temporary( File::open_unlink_later | File::open_inheritable );
    _stdin_file.print( _stdin_data );
    _stdin_file.flush();
    _stdin_file.seek( 0 );

    open_stdout_stderr_files();


#   ifdef Z_WINDOWS

        PROCESS_INFORMATION process_info; 
        STARTUPINFOW        startup_info; 
        string              command_line;

        command_line = quoted_windows_process_parameter( object_server_filename );

        if( params.empty() )
        {
            command_line += " -object-server ";
        }
        else
        {
            Z_FOR_EACH( vector<string>, args_vector, arg )  command_line += " " + quoted_windows_process_parameter( *arg );
        }

        memset( &process_info, 0, sizeof process_info );

      //SetHandleInformation( GetStdHandle(STD_OUTPUT_HANDLE), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT );
      //SetHandleInformation( GetStdHandle(STD_ERROR_HANDLE ), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT );

        memset( &startup_info, 0, sizeof startup_info );
        startup_info.cb          = sizeof startup_info; 
        startup_info.dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        startup_info.hStdInput   = _stdin_file.handle();
        startup_info.hStdOutput  = _stdout_file.handle();
        startup_info.hStdError   = _stderr_file.handle();
        startup_info.wShowWindow = SW_MINIMIZE;            // Als Dienst mit Zugriff auf Desktop wird ein leeres Konsol-Fenster gezeigt. Das minimieren wir.

        DWORD creation_flags = CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT;
        if( _priority != "" )  creation_flags |= windows::priority_class_from_string( _priority );        // Liefert 0 bei Fehler
        Bstr environment_bstr = _has_environment? Bstr(_environment_string) : Bstr((BSTR)NULL);

        Process::create_process(_login, object_server_filename, command_line, creation_flags, environment_bstr, &startup_info, &process_info);
        _pid = process_info.dwProcessId;

        CloseHandle( process_info.hThread );
        _process_handle.set_handle( process_info.hProcess );
        _process_handle.set_name( "Process " + as_string( _pid ) );

        //_stdin_file.close();        // Schließen, damit nicht ein anderer Prozess die Handles erbt und damit das Löschen verhindert (ERRNO-13 Permission denied)
        //_stdout_file.close();
        //_stderr_file.close();

#    else

        Z_LOG( "fork(), execv( \"" << object_server_filename << "\", " << join( ", ", args_vector ) << " )\n" );


        // Verbindung zu stdin:

        //int stdin_socket_pair[2] = { -1, -1 };
        //ret = socketpair( PF_UNIX, SOCK_STREAM, 0, stdin_socket_pair );
        //if( ret == -1 )  check_socket_error( socket_errno(), "socketpair" );

        //_stdin_of_process = stdin_socket_pair[0];
        //set_non_blocking( _stdin_of_process );
        //stdin_fileno = stdin_socket_pair[ 1 ];
        //stdin_socket_pair[ 1 ] = SOCKET_ERROR;
        

        _pid = fork();
        if( _pid == 0 )
        {
            Log_ptr::disable_logging(); // fork() kann gesperrte Mutex übernehmen, was zum Deadlock führt (stimmt das?)
            // Z_LOG() ist jetzt wirkunglos. Kann cerr auch gesperrt sein? Wenigstens ist es unwahrscheinlich, weil cerr kaum benutzt wird.

            setpgid(0,0);   // Neue Prozessgruppe für den API Prozess vgl. JS-930

            if( _priority != "" ) 
            {
                try
                {
                    int error = setpriority( PRIO_PROCESS, getpid(), posix::priority_from_string( _priority ) );
                    if( error )  throw_errno( errno, "setpriority" );
                }
                catch( exception& x ) { cerr << "setpriority(" << _priority << ") ==> ERROR " << x.what() << "\n"; }
            }

            ::signal(SIGINT, SIG_DFL);
            ::signal(SIGTERM, SIG_DFL);
            ::signal(SIGUSR1, SIG_DFL);
            sigset_t sigset;
            sigemptyset(&sigset);
            sigaddset(&sigset, SIGINT);
            sigaddset(&sigset, SIGTERM);
            sigaddset(&sigset, SIGUSR1);
            pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);

            if( _has_environment )
            {
                int err = posix::z_clearenv();
                if( err ) { int e = errno; fprintf( stderr, "clearenv() returns error, errno=%d %s\n", e, strerror( e ) ); exit(2); }
                for( const char* e = _environment_string.c_str(); *e; e += strlen( e ) + 1 )  putenv( (char*)e );
            }

            //ret = closesocket( stdin_fileno );
            //if( ret == -1 )  throw_errno( errno, "close", Z_FUNCTION );
            
            dup2( _stdin_file .file_no(), STDIN_FILENO  );
            dup2( _stdout_file.file_no(), STDOUT_FILENO );
            dup2( _stderr_file.file_no(), STDERR_FILENO );

            // Alle Dateien schließen
            int n = sysconf( _SC_OPEN_MAX );
            for( int i = 3; i < n; i++ )  if( i != other_socket )  ::close(i);

            // stdin haben wir schon geöffnet
            //int new_stdin = ::open( "/dev/null", O_RDONLY );
            //if( new_stdin != -1  &&  new_stdin != STDIN_FILENO )  dup2( new_stdin, STDIN_FILENO ),  ::close( new_stdin );


            if( other_socket != SOCKET_ERROR )
                set_environment_variable( socket_environment_name, as_string( other_socket ) );

            if (_ld_library_path_set) {
                errno = 0;
                int error = setenv("LD_LIBRARY_PATH", _ld_library_path.c_str(), 1);
                if (error) throw_errno(errno, "setenv", "LD_LIBRARY_PATH");
            }
#           ifdef Z_HPUX
                set_environment_variable( "LD_PRELOAD", _ld_preload );
#           endif


            //Z_LOG( "execv(" << object_server_filename.c_str() );
            //for( int i = 0; argv[i] != NULL; i++ )  Z_LOG( ",\"" << argv[i] << "\"" );
            //Z_LOG( " )\n" );

            execv( object_server_filename.c_str(), (char**)argv );
            //Bereits oben mit Z_LOG protokolliert:
            //cerr << "execv(" << object_server_filename.c_str();
            //for( int i = 0; argv[i] != NULL; i++ )  cerr << ",\"" << argv[i] << "\"";
            //cerr << " )\n";
            fprintf( stderr, "execv err=%s\n", strerror(errno) );
            exit(99);
        }
        else
        if( _pid == -1 )  
        {
            _pid = 0;   // §1529
            throw_errno( errno, "fork" );
        }

        Z_LOG( "pid=" << _pid << "\n" );

        //_stdin_file.close();        // Unter Unix können wir die Datei jetzt schon schließen und deren Namen löschen (sie bleibt dem Prozess erhalten)
        
#   endif

    _stdin_file.close();        // Schließen, damit nicht ein anderer Prozess die Handles erbt und damit das Löschen verhindert (ERRNO-13 Permission denied)
    _stdout_file.close();
    _stderr_file.close();

    set_async();
}

//---------------------------------------Connection_to_own_server_process::open_stdout_stderr_files

void Connection_to_own_server_process::open_stdout_stderr_files()
{
    if( !_stdout_file.opened() )  _stdout_file.open_temporary( File::open_unlink_later | File::open_inheritable );
    if( !_stderr_file.opened() )  _stderr_file.open_temporary( File::open_unlink_later | File::open_inheritable );
}

//---------------------------------------------------Connection_to_own_server_process::close__start

Async_operation* Connection_to_own_server_process::close__start()
{
    if( _timedout )  
    {
        try 
        { 
            kill_process(); 
        }
        catch( const exception& x ) 
        {
            Z_LOG( Z_FUNCTION << "  ERROR " << x.what() << "\n" );
        }
    }

    Connection::close__start();

    _last_errno = 0;   // Damit Operation nicht sofort abbricht.

    ptr<Wait_for_process_termination> operation = Z_NEW( Wait_for_process_termination( this ) );
    _my_operation = +operation;

    return _my_operation;
}

//-----------------------------------------------------Connection_to_own_server_process::close__end

void Connection_to_own_server_process::close__end()
{
    ptr<Async_operation> operation = _my_operation;
    _my_operation = NULL;
    operation->async_check_error( Z_FUNCTION );

    _stdin_file.close();
    _stdout_file.close();
    _stderr_file.close();
}

//---------------------------------------------------Connection_to_own_server_process::kill_process

bool Connection_to_own_server_process::kill_process()
{
    bool is_killed = false;

    if( !process_terminated() )
    {
        try
        {
#           ifdef Z_UNIX
                posix::try_kill_process_group_immediately( _pid );    // alle Prozesse der Task beenden (vgl. JS-930)
#           else
                windows::try_kill_process_with_descendants_immediately( _pid, (Has_log*)NULL, (Message_string*)NULL, Z_FUNCTION );
#           endif
            is_killed = true;
        }
        catch( const exception& x ) 
        { 
            Z_LOG( "pid=" << _pid << " " << x.what() << "\n" ); 
        }
        
        _killed |= is_killed;
    }

    return is_killed;
}

//---------------------------------------------Connection_to_own_server_process::process_terminated

bool Connection_to_own_server_process::process_terminated()
{
    if( _pid == 0 )  return true;


#   ifdef Z_WINDOWS

        if( !_process_handle.valid() )  return false;
        
        GetExitCodeProcess( _process_handle, &_exit_code );
        return _exit_code != STILL_ACTIVE;

#    else

        call_waitpid( false );
        return pid() == 0;

#   endif

}

//-----------------------------------------------Connection_to_own_server_process::try_delete_files

bool Connection_to_own_server_process::try_delete_files( Has_log* log )
{
    bool result = true;

    if( _stdin_file .is_to_be_unlinked() )  result &= _stdin_file .try_unlink( log );
    if( _stdout_file.is_to_be_unlinked() )  result &= _stdout_file.try_unlink( log );
    if( _stderr_file.is_to_be_unlinked() )  result &= _stderr_file.try_unlink( log );

    return result;
}

//------------------------------------------------Connection_to_own_server_process::undeleted_files

list<File_path> Connection_to_own_server_process::undeleted_files()
{
    list<File_path> result;

    if( _stdin_file .is_to_be_unlinked() )  result.push_back( _stdin_file .path() );
    if( _stdout_file.is_to_be_unlinked() )  result.push_back( _stdout_file.path() );
    if( _stderr_file.is_to_be_unlinked() )  result.push_back( _stderr_file.path() );

    return result;
}

//---------------------------------------------------Connection_to_own_server_process::::short_name

string Connection_to_own_server_process::short_name() const
{ 
    return S() << "pid=" << pid() << " controller=" << _controller_address;
}

//-------------------------------------------------------Connection_to_own_server_process::obj_name

string Connection_to_own_server_process::obj_name() const
{ 
    return S() << "object_server::Connection_to_own_server_process(pid=" << pid() << ")"; 
}

//---------------------------------------------------Connection_to_own_server_process::call_waitpid
#ifdef Z_UNIX

bool Connection_to_own_server_process::call_waitpid( bool wait )
{
    bool ok = false;
    int  status = 0;

    if( wait )  Z_LOG( "pid=" << _pid << " waitpid() ...\n" );
    int ret = waitpid( _pid, &status, wait? 0 : WNOHANG );

    int err = ret == -1? errno : 0;
    if( wait )  Z_LOG( "pid=" << _pid << " waitpid() => " << ret << " status=" << printf_string("0x%x",ret) << "\n" );

    if( ret == _pid )
    {
        if( WIFEXITED  ( status ) )  _exit_code          = WEXITSTATUS( status ),  ok = true;
        if( WIFSIGNALED( status ) )  _termination_signal = WTERMSIG   ( status ),  ok = true;  

        _pid = 0;
    }
    else
    {
        if( ret == -1 )
        {
            Z_LOG( "pid=" << _pid << " waitpid()  ERRNO-" << err << "  " << strerror(err) << "\n" );
            if( err == ECHILD )  _pid = 0;
        }
    }

    return ok;
}

#endif

//------------------------------------Connection_to_own_server_thread::Server_thread::Server_thread

Connection_to_own_server_thread::Server_thread::Server_thread( Connection_to_own_server_thread* connection ) 
: 
    _zero_(this+1),
    _connection(connection) 
{
    set_thread_name( S() << "Connection_to_own_server_thread::Server_thread(" << _connection->_controller_address << ")" );
}

//-----------------------------------Connection_to_own_server_thread::Server_thread::~Server_thread
    
Connection_to_own_server_thread::Server_thread::~Server_thread()
{
}

//---------------------------------------Connection_to_own_server_thread::Server_thread::run_server

int Connection_to_own_server_thread::Server_thread::run_server(jobject injectorJ, int keep_alive_timeout)
{
    Com_initialize com_initialize;

    _server->simple_server(_connection->_controller_address, injectorJ, keep_alive_timeout);

    return 0;
}

//--------Connection_to_own_server_thread::Wait_for_thread_termination::Wait_for_thread_termination

Connection_to_own_server_thread::Wait_for_thread_termination::Wait_for_thread_termination( Connection_to_own_server_thread* connection )
: 
    _zero_(this+1),
    _connection(connection),
    _wait_for_termination_poll(0.01)
{
    async_continue_( cont_default );
}

//-------------------Connection_to_own_server_thread::Wait_for_process_termination::async_continue_

bool Connection_to_own_server_thread::Wait_for_thread_termination::async_continue_( Continue_flags flags )
{
    bool something_done = true;

    if( flags & cont_wait )  
    {
        _connection->check_async( this );     // Warnung ausgeben, wenn asynchroner Betrieb verlangt ist
    }
    else
    {
        Z_LOG(Z_FUNCTION << " " << _wait_for_termination_poll << "s\n");
        set_async_delay(_wait_for_termination_poll);
        _wait_for_termination_poll = min(0.1, _wait_for_termination_poll + 0.02);  // Limit delay to 0.1s
    }

    if( _connection->_thread )
    {
        bool terminated = !_connection->_thread->thread_is_running();
        if( terminated )
        {
            _connection->_thread->thread_wait_for_termination();
            _connection->_thread = NULL;
            something_done = true;
        }
    }

    return something_done;
}

//------------------Connection_to_own_server_thread::Wait_for_thread_termination::async_state_text_

string Connection_to_own_server_thread::Wait_for_thread_termination::async_state_text_() const
{ 
    S result;
    result << Z_FUNCTION;

    if( _connection->_thread )  result << "(terminating thread thread_id=" << _connection->_thread->thread_id() << " ...)"; 
                          else  result << "(thread terminated)";

    return result;
}

//---------------------------------onnection_to_own_server_thread::~Connection_to_own_server_thread

Connection_to_own_server_thread::~Connection_to_own_server_thread()
{
}

//----------------------------------------------------Connection_to_own_server_thread::start_thread

void Connection_to_own_server_thread::start_thread( Server_thread* thread )
{
    if (!_controller_address) {
        prepare_connection_to_controller();
    }
    assert( _controller_address );
    assert( thread->connection() == this );

    if( thread )  _thread = thread;
          //else  _thread = Z_NEW( Server_thread( this ) );

    _thread->thread_start();

    set_async();
}

//---------------------------------------------------Connection_to_own_server_thread::close__start

Async_operation* Connection_to_own_server_thread::close__start()
{
    Connection::close__start();

    _last_errno = 0;   // Damit Operation nicht sofort abbricht.

    ptr<Wait_for_thread_termination> operation = Z_NEW( Wait_for_thread_termination( this ) );
    _my_operation = +operation;

    return _my_operation;
}

//-----------------------------------------------------Connection_to_own_server_thread::close__end

void Connection_to_own_server_thread::close__end()
{
    ptr<Async_operation> operation = _my_operation;
    _my_operation = NULL;
    operation->async_check_error( Z_FUNCTION );
}

//---------------------------------------------------Connection_to_own_server_thread::::short_name

string Connection_to_own_server_thread::short_name() const
{ 
    S result;

    result << "thread_id=" << thread_id() << " controller=" << _controller_address;

    return result;
}

//-------------------------------------------------------Connection_to_own_server_thread::obj_name

string Connection_to_own_server_thread::obj_name() const
{ 
    return S() << "object_server::Connection_to_own_server_thread(thread_id=" << thread_id() << ")"; 
}

//----------------------------------------------------------------------------string_from_object_id

string string_from_object_id( Object_id id )
{
    return printf_string( "%08X", (uint32)(id._value >> 32        ) ) + "." + 
           printf_string( "%08X", (uint32)(id._value & 0xffffffff ) );
}

//-----------------------------------------------------------------------Object_entry::Object_entry

Object_entry::Object_entry( const Object_entry& e )
:
    _zero_(this+1)
{
    _id             = e._id;
    _iunknown       = e._iunknown;
    _table_is_owner = e._table_is_owner;

#   ifdef _DEBUG
        _debug_string = e._debug_string;
#   endif

    //Z_DEBUG_ONLY( if( _iunknown )  Z_LOG( "com_remote: Object_entry(" << _id << ") == " << e << "  " << ( _table_is_owner? "AddRef()\n" : "\n" ) ); )

    if( _table_is_owner && _iunknown )  _iunknown->AddRef();
}

//----------------------------------------------------------------------Object_entry::~Object_entry

Object_entry::~Object_entry()
{ 
    close();
}

//------------------------------------------------------------------------------Object_entry::close

void Object_entry::close()
{ 
    IUnknown* iunknown = _iunknown;

    _iunknown = NULL;

    //Z_DEBUG_ONLY( if( _iunknown )  Z_LOG( "com_remote: Object_entry(" << _id << ") == " << *this << "  " << ( _table_is_owner? "Release()\n" : "\n" ) ); )

    if( _table_is_owner && iunknown )  iunknown->Release();
}

//-------------------------------------------------------------------Object_entry::set_debug_string

void Object_entry::set_debug_string()
{
#   ifdef Z_DEBUG
        S s;
        s << string_local_from_time_t( double_from_localtime() ) 
          << ' ' 
          << name_of_type( *_iunknown );

        Proxy* proxy = dynamic_cast<Proxy*>( _iunknown );
        if( proxy )  s << " -> " << proxy->title();

        _debug_string = s;
#   endif
}

//--------------------------------------------------------------------------Object_entry::obj_print

void Object_entry::obj_print( ostream* s ) const
{
    *s << _id << " - " << (void*)_iunknown;

#   ifdef _DEBUG
        *s << ' ';
        *s << _debug_string;
#   endif

    *s << ' ';

#   if defined Z_WINDOWS      // Manche Zeiger sind (nach Fehler) ungültig. Nur Windows kann das mit ... abfangen.
        try 
        { 
            *s << name_of_type( *_iunknown );
            Proxy* p = dynamic_cast<Proxy*>( _iunknown );
            if( p )  *s << " -> " << p->title();
          //if( p )  *s << name_of_type( *_iunknown ) << " " << p->title();
          //   else  *s << "Lokal " << name_of_type( *_iunknown ); 
        }
        catch( ... ) { *s << "(no type information)"; }    // typeid stürzt bei manchen Adressen ab (Microsoft)
#   endif

    //_iunknown ist manchmal, nach Exception, ungültig.   *s << ", " << ( _iunknown->AddRef(), _iunknown->Release() ) << " references";
}

//------------------------------------------------------------------------------Object_table::clear

void Object_table::clear()
{ 
    Z_FOR_EACH( Map, _objects, o )
    {
        o->second.close();
    }

    _objects.clear(); 
}

//-----------------------------------------------------------------------------Object_table::remove

void Object_table::remove( Object_id id )
{
    Map::iterator it = _objects.find( id );
    if( it == _objects.end() )  throw_xc( "Z-REMOTE-120", string_from_object_id( id ), "remove" );

    Object_entry* e = &it->second;

    Z_DEBUG_ONLY( Z_LOG2( "object_server", "Object_table::remove(" << id << ") => " << *e << " " << ( e->_table_is_owner? ".Release()\n" : "\n" ) ); )

    if( e->_table_is_owner  &&  e->_iunknown )  e->_iunknown->Release();

    e->_iunknown = NULL;

    _objects.erase( it );
}

//-------------------------------------------------------------------------Object_table::get_object

ptr<IUnknown> Object_table::get_object( Session*, Object_id id ) //, bool is_new, bool become_owner, const string& title )
{
    if( id._value == 0 )  return NULL;

    Map::iterator it = _objects.find( id );
    if( it != _objects.end() ) 
    {
        //Z_DEBUG_ONLY( Z_LOG( "com_remote: Object_table::get_object(" << id << ") => " << it->second << "\n" ); )
        //if( is_new )  throw_xc( "Z-REMOTE-110", string_from_object_id( id ) );
        return it->second._iunknown;
    }
    else
        throw_xc( "Z-REMOTE-102", string_from_object_id( id ) );
}

//-------------------------------------------------------------------------Object_table::add_proxy

void Object_table::add_proxy( Session*, Object_id id, Proxy* proxy )
{
    Object_entry& e = _objects[ id ];

    e._id             = id;
    e._iunknown       = (IDispatch*)+proxy;
    e._table_is_owner = false;

#   ifdef Z_DEBUG
        e.set_debug_string();
#   endif

    //Z_DEBUG_ONLY( Z_LOG( "com_remote: Object_table::get_object(" << id << ") => new " << e << "  " << ( e._table_is_owner? "AddRef()\n" : "\n" ) ); )
}

//----------------------------------------------------------------------Object_table::get_object_id

Object_id Object_table::get_object_id(IUnknown* iunknown, bool* is_new)
{
    if( is_new )  *is_new = false;

    Z_FOR_EACH( Map, _objects, o )
    {
        if( o->second._iunknown == iunknown )  return  o->first;
    }

    if( !is_new )  throw_xc( "Z-REMOTE-103", printf_string( "%lX", (long)(void*)iunknown ) );


    
    *is_new = true;

    Object_id     id = ++_next_id  |  ( uint64(   ( rand() << 16 ) 
                                                ^ rand() 
                                                ^ (int)(size_t)iunknown 
                                                ^ (int)(size_t)(void*)this ) << 32 );
    Object_entry& e  = _objects[ id ];

    e._id             = id;
    e._iunknown       = iunknown;
    e._table_is_owner = true;

#   ifdef Z_DEBUG
        e.set_debug_string();
#   endif

    //Z_DEBUG_ONLY( Z_LOG( "com_remote: Object_table::get_object_id(" << (void*)iunknown << ") => " << e << "  " << ( e._table_is_owner? "AddRef()\n" : "\n" ) ); )
    e._iunknown->AddRef();

    return e._id;
}

//--------------------------------------------------------------------------Object_table::obj_print

void Object_table::obj_print( ostream* s ) const
{
    Z_FOR_EACH_CONST( Map, _objects, o )
    {
        const Object_entry& e = o->second;

        *s << e << '\n';
    }
}

//--------------------------------------------------------------------------------Session::~Session

Session::~Session()
{
    try
    {
        if( !_object_table.empty() )  Z_LOG( "pid=" << pid() << " Open objects:\n" << _object_table << "\n" );
        _object_table.clear();

        //close__start() -> async_finish();
        //close__end();
    }
    catch( exception& x ) { Z_LOG( "~Session(): " << x.what() << "\n" ); }
}

//----------------------------------------------------------------------------Session::close__start

Async_operation* Session::close__start()
{
    _sync_operation = Z_NEW(Sync_operation);
    Async_operation* operation = _sync_operation;

    if( !_object_table.empty() )  Z_LOG( "pid=" << pid() << " Open objects:\n" << _object_table << "\n" );

    _object_table.clear();

    if( _connection )
    {
        if( _connection_has_only_this_session )  operation = _connection->close__start();
        _connection = NULL;
    }

    return operation;
}

//-----------------------------------------------------------------Session::execute_create_instance

HRESULT Session::execute_create_instance( const CLSID& clsid, IUnknown* outer, DWORD context, 
                                          COSERVERINFO* coserverinfo, unsigned long count, MULTI_QI* query_interfaces )
{
    HRESULT hr;

    Z_DEBUG_ONLY( Z_LOGI( "pid=" << _connection->pid() << " Session::execute_create_instance(" << string_from_clsid( clsid ) << ")\n" ); )

    if( count < 1 )  return E_NOINTERFACE;

    Create_instance_function* f = _server->get_class_factory( clsid );
    if( f )
    {
        ptr<IUnknown> iunknown;

        hr = f( this, &_server->_class_register[ clsid ]._class_object, *query_interfaces[0].pIID, &iunknown );
        query_interfaces[0].hr = hr;

        if( !FAILED(hr) )
        {
            query_interfaces[0].pItf = iunknown;
            query_interfaces[0].pItf->AddRef();

            for( int i = 1; i < (int)count; i++ )
            {
                ptr<IUnknown> iface;
                HRESULT hr2 = iface.Assign_qi( iunknown, *query_interfaces[i].pIID );
                query_interfaces[i].hr = hr2;

                if( FAILED(hr2) )
                {
                    if( hr2 == E_NOINTERFACE )  
                    {
                        hr = E_NOINTERFACE;
                    }
                    else
                    {
                        hr = hr2;
                        break;
                    }
                }
                else
                {
                    query_interfaces[i].pItf = iface;
                    query_interfaces[i].pItf->AddRef();
                    if( hr == E_NOINTERFACE )  hr = CO_S_NOTALLINTERFACES;      // Wenigstens ein Interface ist da (kein Fehler).
                }
            }
        }
    }
    else
    {
        hr = CoCreateInstanceEx( clsid, outer, context, coserverinfo, count, query_interfaces );
    }

    return hr;
}

//-----------------------------------------------------------Session::execute_object_queryinterface

void Session::execute_object_queryinterface( IUnknown* iunknown, Input_message* in, Output_message* out )
{
    HRESULT       hr;
    //IUnknown*   result = NULL;
    ptr<IUnknown> result;
    IID           iid = in->read_guid();

    Z_LOG2( "object_server.QueryInterface", "pid=" << _connection->pid() << " Session::execute_object_queryinterface " << string_from_guid(iid)  << ' ' << name_of_iid_or_empty(iid) << "\n" );


    hr = result.Assign_qi( iunknown, iid );

    out->write_int32( hr );

    if( !FAILED(hr) ) 
    {
        out->write_iunknown( result );
    }

    out->finish();
}

//---------------------------------------------------------------------Session::execute_object_call

void Session::execute_object_call( IDispatch* idispatch, Input_message* in, Output_message* out )
{
    Bstr        names [1];
    DISPID      dispids[1];
    Dispparams  dispparams;
    Excepinfo   excepinfo;
    uint        argnr = (uint)-1;
    Variant     result;
    HRESULT     hr;
    
    in->read_bstr( &names[0]._bstr );
    in->read_dispparams( &dispparams );
    in->finish();

    Z_LOG2( "object_server.call", "pid=" << _connection->pid() << " Session::execute_object_call " << names[0] << "\n" );

    hr = idispatch->GetIDsOfNames( IID_NULL, &names[0]._bstr, 1, STANDARD_LCID, dispids );
    if( FAILED(hr) )  throw_com( hr, "GetIDsOfNames", string_from_bstr(names[0]) );

    hr = idispatch->Invoke( dispids[0], IID_NULL, STANDARD_LCID, DISPATCH_METHOD, &dispparams, &result, &excepinfo, &argnr );
    if( FAILED(hr) )  throw_com_excepinfo( hr, &excepinfo, string_from_bstr(names[0]).c_str(), "" );

    out->write_int32( hr );
    out->write_variant( result );
    out->finish();
}

//------------------------------------------------------------Session::execute_object_getidsofnames

void Session::execute_object_getidsofnames( IDispatch* idispatch, Input_message* in, Output_message* out )
{
    //Bstr   names   [ max_dispids_per_method ];
    DISPID dispids [ max_dispids_per_method ];

    IID  iid         = in->read_guid();
    LCID lcid        = in->read_int32();
    int  names_count = in->read_int32();
    int  i;

  //if( (uint)names_count > NO_OF( names ) )  throw_xc( "Z-REMOTE-124" );
    if( (uint)names_count != 1 )  throw_xc( "Z-REMOTE-124" );

    //for( i = 0; i < names_count; i++ )  in->read_bstr( &names[i]._bstr );
    Bstr   name;
    in->read_bstr( &name._bstr );

    in->finish();

    Z_LOGI2( "object_server.GetIDsOfNames", "pid=" << _connection->pid() << " Session::execute_object_getidsofnames " << string_from_bstr( name ) << "\n" );

    HRESULT hr = idispatch->GetIDsOfNames( iid, &name._bstr, 1, lcid, dispids );
    
    out->write_int32( hr );

    if( !FAILED(hr) ) 
    {
        for( i = 0; i < names_count; i++ )  out->write_int32( dispids[i] );
    }

    out->finish();
}

//-------------------------------------------------------------------Session::execute_object_invoke

void Session::execute_object_invoke( IDispatch* idispatch, Input_message* in, Output_message* out )
{
    Dispparams dispparams;
    Excepinfo  excepinfo;
    Variant    result;
    uint       argerr = (uint)-1;

    DISPID dispid = in->read_int32();
    IID    iid    = in->read_guid();
    LCID   lcid   = in->read_int32();
    int    flags  = in->read_int32();

    in->read_dispparams( &dispparams );
    in->finish();

    Z_LOG2( "object_server.Invoke", "pid=" << _connection->pid() << " Session::execute_object_invoke " << (void*)(long)dispid << ' ' << name_of_dispid_or_empty( idispatch, dispid ) << "\n" ); 

    HRESULT hr = idispatch->Invoke( dispid, iid, lcid, flags, &dispparams, &result, &excepinfo, &argerr );

    out->write_int32( hr );

    if( !FAILED( hr ) )
    {
        out->write_variant( result );
    }
    else
    {
        out->write_int32( argerr );
        out->write_excepinfo( excepinfo );
    }
}

//--------------------------------------------------------------------------Session::execute_object

void Session::execute_object( IUnknown* iunknown, Input_message* in, Output_message* out )
{
    Message_cmd cmd = (Message_cmd)in->read_char();

    switch( cmd )
    {
        case cmd_queryinterface:
        {
            execute_object_queryinterface( iunknown, in, out );
            break;
        }

        default:
        {
            qi_ptr<IDispatch> idispatch = iunknown;

            switch( cmd )
            {
                case cmd_getidsofnames: 
                    execute_object_getidsofnames( idispatch, in, out );  
                    break;

                case cmd_invoke:
                    execute_object_invoke( idispatch, in, out );
                    break;

                case cmd_call: 
                    execute_object_call( idispatch, in, out );  
                    break;

                default:
                    throw_xc( "MSGERROR", "cmd" );
            }
        }
    }
}

//---------------------------------------------------------------------------------Session::execute

void Session::execute( Input_message* in, Output_message* out )
{
    Message_class  message_class = (Message_class)in->read_char();
  //Output_message out           ( this );

    out->clear();
    out->_is_answer = true;
    out->write_char( msg_answer );

    try
    {
        switch( message_class )
        {
            case msg_session:
            {
                if (in->read_int64() != 0)  throw_xc("Z-REMOTE-104", string_from_object_id(0));   // Session ID is always 0
            
                switch( in->read_char() )
                {
                    case cmd_createinstance:
                    {
                        CLSID           clsid   = in->read_guid();
                        ptr<IUnknown>   outer   = in->read_iunknown();
                        int             context = in->read_int32();
                        int             count   = in->read_int32();
                        Multi_qi        qi      ( count );
                        int             i;

                        for( i = 0; i < count; i++ )  qi.set_iid( i, in->read_guid() );
                        in->finish();

                        HRESULT hr = execute_create_instance( clsid, outer, context, NULL, count, qi );

                        out->write_int32( hr );

                        if( !FAILED(hr) ) 
                        {
                            for( i = 0; i < count; i++ )  
                            {
                                out->write_int32( qi[i].hr );  
                                out->write_iunknown( qi[i].pItf );
                            }
                        }

                        break;
                    }

                    default:
                        throw_xc( "Z-REMOTE-105" );
                }

                break;
            }

            case msg_object:
            {
                Object_id object_id = in->read_int64();
                if( object_id._value == 0 )  throw_com( E_POINTER, "remote_call" );

                if( in->peek_char() == cmd_release )
                {
                    _object_table.remove( object_id );
                }
                else
                {
                    execute_object( object_from_id( object_id ), in, out );
                }

                break;
            }

            case msg_keep_alive: 
                Z_LOG2("object_server.keep_alive", "Received\n");
                break;

            case msg_none:
            default:
                throw_xc( "Z-REMOTE-106" );
        }
    }
    catch( const _com_error& x )
    {
        out->clear();
        out->_is_answer = true;
        out->write_char( msg_error );
        out->write_string( "name=COM" );
        out->write_string( "code=COM-" + printf_string( "%08X", x.Error() ) );
        out->write_string( "what=" + string_from_ole( x.Description() ) );
    }
    catch( const exception& x )
    {
        string name;
        string code;
        
#       ifndef DYNAMIC_CAST_CRASHES        
            const Xc* xc = dynamic_cast< const Xc* >( &x );

            if( xc ) 
            {
                name = xc->name();
                code = xc->code();
            }
#       endif            

        out->clear();
        out->_is_answer = true;
        out->write_char( msg_error );
        out->write_string( "name=" + name );
        out->write_string( "code=" + code );
        out->write_string( "what=" + string(x.what()) );
    }

    out->finish();
}

//---------------------------------------------------------------------------Session::send_response

void Session::send_response( Output_message* out, bool* connection_lost_ptr )
{
    try
    {
        if( connection_lost_ptr )  *connection_lost_ptr = false;
        out->send();
    }
    catch( const exception& ) 
    {
        if( connection_lost_ptr )  *connection_lost_ptr = true;
                             else  throw;
    }
}

//------------------------------------------------------------------Session::create_instance__start

Simple_operation* Session::create_instance__start( const CLSID& clsid, IUnknown* outer, DWORD, COSERVERINFO*, uint count, MULTI_QI* query_interfaces )
{
    ptr<Simple_operation> operation = Z_NEW( Simple_operation( this, NULL, "create_instance" ) );

    operation->_output_message.write_char    ( msg_session );
    operation->_output_message.write_int64   ( 0 );  // Session ID, is always 0
    operation->_output_message.write_char    ( cmd_createinstance );
    operation->_output_message.write_guid    ( clsid );
    operation->_output_message.write_iunknown( outer );
    operation->_output_message.write_int32   ( 0 );        // context
    operation->_output_message.write_int32   ( count );

    for( int i = 0; i < (int)count; i++ )  operation->_output_message.write_guid( *query_interfaces->pIID );

    operation->_output_message.finish();
    operation->start();

    return operation;
}

//--------------------------------------------------------------------Session::create_instance__end

HRESULT Session::create_instance__end( uint count, MULTI_QI* query_interfaces )
{
    HRESULT hr = NOERROR;

    for( int i = 0; i < (int)count; i++ )  query_interfaces[i].pItf = NULL;

    ptr<Simple_operation> operation = _connection->pop_operation( NULL, "create_instance" );

    try
    {
        operation->_input_message.read_char();     // msg_answer!

        hr = operation->_input_message.read_int32();

        if( !FAILED( hr ) ) 
        {
            for( int i = 0; i < (int)count; i++ )
            {
                query_interfaces[i].hr = operation->_input_message.read_int32();
                operation->_input_message.read_iunknown().move_to( &query_interfaces[i].pItf );
            }
        }
    
        operation->_input_message.finish();
    }
    catch( const exception& )
    {
        for( int i = 0; i < (int)count; i++ )  if( query_interfaces[i].pItf )  query_interfaces[i].pItf->Release(),  query_interfaces[i].pItf = NULL;
        throw;
    }

    return hr;
}

//--------------------------------------------------------------------------Session::object_from_id

ptr<IUnknown> Session::object_from_id( Object_id id )
{
    return _object_table.get_object( this, id );
}

//-----------------------------------------------------------Connection_manager::Connection_manager

Connection_manager::Connection_manager()
:
    _common_buffer( connection_buffer_size )
{
}

//----------------------------------------------------------Connection_manager::~Connection_manager

Connection_manager::~Connection_manager()
{
}

//---------------------------------------------------------------Connection_manager::new_connection

ptr<Connection> Connection_manager::new_connection()
{
    ptr<Connection> connection = Z_NEW( Connection( this ) );

    return +connection;
}

//-----------------------------------------Connection_manager::new_connection_to_own_server_process

ptr<Connection_to_own_server_process> Connection_manager::new_connection_to_own_server_process()
{
    ptr<Connection_to_own_server_process> connection = Z_NEW( Connection_to_own_server_process( this ) );

    return +connection;
}

//------------------------------------------Connection_manager::new_connection_to_own_server_thread

ptr<Connection_to_own_server_thread> Connection_manager::new_connection_to_own_server_thread()
{
    ptr<Connection_to_own_server_thread> connection = Z_NEW( Connection_to_own_server_thread( this ) );

    return +connection;
}

//------------------------------------------------------------Connection_manager::remove_connection

void Connection_manager::remove_connection( Connection* connection )
{
    Z_FOR_EACH( Connection_list, _connection_list, c )
    {
        if( *c == connection ) { _connection_list.erase( c ); break; }
    }
}

//-----------------------------------------------------------------------------Output_message::send

void Output_message::send()
{
    bool ok = send_async();
    if( !ok )  throw_xc( Connection_reset_exception( "Z-REMOTE-101", pid() ) );
}

//-----------------------------------------------------------------------Output_message::send_async

bool Output_message::send_async()
{
    //Z_LOG( "Simple_operation::send_async(" << _data_bytes_written << ',' << ( _data.length() - _data_bytes_written ) << " Bytes)\n" );

    int written = _session->connection()->send_async( _data.data() + _data_bytes_written, int_cast(_data.length() - _data_bytes_written) );

    _data_bytes_written += written;
    _is_sent = _data_bytes_written == _data.length();

    //Z_LOG2( "zschimmer", Z_FUNCTION << " _is_sent=" << _is_sent << " _data_bytes_written=" << _data_bytes_written << "\n" );

    return written > 0;
}

//---------------------------------------------------------------------------Output_message::finish

void Output_message::finish()
{
    int len = int_cast(_data.length()) - 4;

    _data[0] = (char)( ( (uint32)len >> 24 ) & 0xff );
    _data[1] = (char)( ( (uint32)len >> 16 ) & 0xff );
    _data[2] = (char)( ( (uint32)len >>  8 ) & 0xff );
    _data[3] = (char)( ( (uint32)len >>  0 ) & 0xff );
}

//----------------------------------------------------------------------Output_message::write_int16

void Output_message::write_int16( int16 i )
{
    _data += (char)( ( (uint16)i >> 8 ) & 0xff );
    _data += (char)( ( (uint16)i >> 0 ) & 0xff );
}

//----------------------------------------------------------------------Output_message::write_int32

void Output_message::write_int32( int32 i )
{
    _data += (char)( ( (uint32)i >> 24 ) & 0xff );
    _data += (char)( ( (uint32)i >> 16 ) & 0xff );
    _data += (char)( ( (uint32)i >>  8 ) & 0xff );
    _data += (char)( ( (uint32)i >>  0 ) & 0xff );
}

//----------------------------------------------------------------------Output_message::write_int64

void Output_message::write_int64( int64 i )
{
    write_int32( int32( i >> 32        ) );
    write_int32( int32( i & 0xffffffff ) );
}

//---------------------------------------------------------------------Output_message::write_string

void Output_message::write_string( const string& str )
{
    int bytes = int_cast(str.length());
    write_int32( bytes );
    _data += str;
    //_data += utf8_from_iso_8859_1( str );
}

//---------------------------------------------------------------------Output_message::write_string

void Output_message::write_string( const OLECHAR* unicode, int length )
{
    //write_int32( utf8_length(  );
    //write_unicode_as_utf8( unicode, length );
    write_int32( length );
    _data += string_from_ole( unicode, length );
}

//------------------------------------------------------------------Output_message::write_safearray

void Output_message::write_safearray( const SAFEARRAY* const_safearray )
{
    SAFEARRAY* safearray = const_cast<SAFEARRAY*>( const_safearray );
    VARTYPE    vartype = 0;

    HRESULT hr = z_SafeArrayGetVartype( safearray, &vartype );
    if( FAILED( hr ) )  throw_com( hr, "SafeArrayGetVartype" );

    if( safearray->cDims != 1 )  throw_xc( Z_FUNCTION );
    if( safearray->rgsabound[ 0 ].lLbound != 0 )  throw_xc( Z_FUNCTION );
    assert( safearray->rgsabound[ 0 ].cElements == Locked_any_safearray( safearray ).count() );

    write_int16( safearray->cDims );
    write_int16( safearray->fFeatures );
    write_int32( safearray->rgsabound[ 0 ].cElements );     
    write_int32( safearray->rgsabound[ 0 ].lLbound );
    write_int32( vartype );


    switch( vartype )
    {
        case VT_UI1    : { Locked_any_safearray a ( safearray ); write_bytes( a->pvData, a.count() * 1 ); break; }
      //case VT_UI4    : { Locked_any_safearray a ( safearray ); write_bytes( a->pvData, a.count() * 4 ); break; }

        case VT_BSTR: {
            Locked_safearray<BSTR> a ( safearray ); 
            for( int i = 0; i < a.count(); i++ )  write_bstr( a[ i ] ); 
            break;
        }

        case VT_VARIANT: 
        { 
            Locked_safearray<VARIANT> a ( safearray ); 
            for( int i = 0; i < a.count(); i++ )  write_variant( a[ i ] ); 
            break;
        }

        default: throw_xc( Z_FUNCTION );
    }
}

//-----------------------------------------------------------------------Output_message::write_guid

void Output_message::write_guid( const GUID& guid )
{
    write_int32( guid.Data1 );
    write_int16( guid.Data2 );
    write_int16( guid.Data3 );
    write_bytes( guid.Data4, sizeof guid.Data4 );
}

//--------------------------------------------------------------------Output_message::write_variant

void Output_message::write_variant( const VARIANT& v )
{
    if( v.vt == (VT_VARIANT|VT_BYREF) )
    {
        write_variant( *V_VARIANTREF(&v) );
        return;
    }


    write_int32( v.vt & ~VT_BYREF );

    {
        //Z_LOG2( "DEBUG", Z_FUNCTION << " " << debug_string_from_variant( v ) << "\n" );

        switch( v.vt & ~VT_BYREF )
        {
            case VT_EMPTY:      break;
            case VT_NULL:       break;
            case VT_I2:         write_int16   ( v.vt & VT_BYREF? *V_I2REF      ( &v ) : V_I2      ( &v ) );     break;
            case VT_I4:         write_int32   ( v.vt & VT_BYREF? *V_I4REF      ( &v ) : V_I4      ( &v ) );     break;
            case VT_R4:         write_double  ( v.vt & VT_BYREF? *V_R4REF      ( &v ) : V_R4      ( &v ) );     break;
            case VT_R8:         write_double  ( v.vt & VT_BYREF? *V_R8REF      ( &v ) : V_R8      ( &v ) );     break;
            case VT_CY:         write_int64(  ( v.vt & VT_BYREF? *V_CYREF      ( &v ) : V_CY      ( &v ) ).int64 );  break;
            case VT_DATE:       write_double  ( v.vt & VT_BYREF? *V_DATEREF    ( &v ) : V_DATE    ( &v ) );     break;
            case VT_BSTR:       write_bstr    ( v.vt & VT_BYREF? *V_BSTRREF    ( &v ) : V_BSTR    ( &v ) );     break;
            case VT_DISPATCH:   write_iunknown( v.vt & VT_BYREF? *V_DISPATCHREF( &v ) : V_DISPATCH( &v ) );     break;
            case VT_ERROR:      write_int32   ( v.vt & VT_BYREF? *V_ERRORREF   ( &v ) : V_ERROR   ( &v ) );     break;
            case VT_BOOL:       write_char  ( ( v.vt & VT_BYREF? *V_BOOLREF    ( &v ) : V_BOOL    ( &v ) )? 1 : 0 );      break;
          //case VT_VARIANT:
            case VT_UNKNOWN:    write_iunknown( v.vt & VT_BYREF? *V_UNKNOWNREF ( &v ) : V_UNKNOWN ( &v ) );     break;
          //case VT_DECIMAL:
            case VT_I1:         write_char    ( v.vt & VT_BYREF? *V_UI1REF     ( &v ) : V_UI1     ( &v ) );     break;
            case VT_UI1:        write_char    ( v.vt & VT_BYREF? *V_UI1REF     ( &v ) : V_UI1     ( &v ) );     break;
            case VT_UI2:        write_int16   ( v.vt & VT_BYREF? *V_UI2REF     ( &v ) : V_UI2     ( &v ) );     break;
            case VT_UI4:        write_int32   ( v.vt & VT_BYREF? *V_UI4REF     ( &v ) : V_UI4     ( &v ) );     break;
            case VT_I8:         write_int64   ( v.vt & VT_BYREF? *V_I8REF      ( &v ) : V_I8      ( &v ) );     break;
            case VT_UI8:        write_int64   ( v.vt & VT_BYREF? *V_UI8REF     ( &v ) : V_UI8     ( &v ) );     break;
            case VT_INT:        write_int32   ( v.vt & VT_BYREF? *V_INTREF     ( &v ) : V_INT     ( &v ) );     break;
            case VT_UINT:       write_int32   ( v.vt & VT_BYREF? *V_UINTREF    ( &v ) : V_UINT    ( &v ) );     break;
          //case VT_VOID:
          //case VT_HRESULT:
          //case VT_PTR:
          //case VT_CARRAY:
          //case VT_USERDEFINED:
          //case VT_LPSTR:
          //case VT_LPWSTR:
          //case VT_RECORD:
          //case VT_FILETIME:
          //case VT_BLOB:
          //case VT_STREAM:
          //case VT_STORAGE:
          //case VT_STREAMED_OBJECT:
          //case VT_STORED_OBJECT:
          //case VT_BLOB_OBJECT:
          //case VT_CF:
          //case VT_CLSID:

            // Diese Typen werden binär übertragen. Wenn der Zielrechner eine andere Prozessorarchitektur hat, wird's nicht funktionieren!
         //   case VT_R4:         write_bytes   ( &V_R4(&v), 4 );                                                 break;
         //   case VT_R8:         write_bytes   ( &V_R8(&v), 8 );                                                 break;
         ////?case VT_CY:         write_bytes   ( &V_CY(&v), 8 );                                                 break;
         //   case VT_DATE:       write_bytes   ( &V_CY(&v), 8 );                                                 break;

            default:
            {
                if( v.vt & VT_ARRAY )
                {
                    SAFEARRAY* safearray = const_cast<SAFEARRAY*>( v.vt & VT_BYREF? *V_ARRAYREF( &v ) : V_ARRAY( &v ) );
                    
                    VARTYPE vartype = 0;
                    HRESULT hr = z_SafeArrayGetVartype( safearray, &vartype );
                    if( FAILED( hr ) )  throw_com( hr, "SafeArrayGetVartype" );
                    if( ( v.vt & ~VT_BYREF ) != ( VT_ARRAY | vartype ) )  throw_xc( "Z-REMOTE-128", vartype_name(v.vt), vartype_name(vartype) );

                    write_safearray( safearray );
                }
                else
                    throw_xc( "Z-REMOTE-108", vartype_name( v.vt ) );
            }
        }
    }
}

//---------------------------------------------------------------------Output_message::write_double

void Output_message::write_double( double d )
{
    char buffer [ 100+1 ];
    buffer[ sizeof buffer - 1 ] = '\0';

    int length = snprintf( buffer, sizeof buffer - 1, "sn%.16e", d );      // double hat 16 (17?) Dezimalstellen, also eine Vorkomma- und 15 (16?) Nachkommastellen
  //buffer[ 0 ] = 's';      // Kennzeichen, dass double als string folgt
    buffer[ 1 ] = length - 2;
    write_bytes( buffer, length );
}

//-------------------------------------------------------------------Output_message::write_iunknown

void Output_message::write_iunknown( IUnknown* iunknown )
{
    bool        is_new = false;
    Object_id   object_id = 0;

    if( iunknown )
    {
        object_id = _session->_object_table.get_object_id(iunknown, &is_new);
    }

    write_int64( object_id._value );
    write_char( is_new );

    if( is_new )  
    {
        write_string( name_of_type( *iunknown ) );  // Zur Debug-Ausgabe


        // Gibt es zu iunknown eine eigene Implementierung eines Proxy (derzeit spooler::Com_log_proxy)? 
        // Dann übertragen wir den Klassennamen des Proxys und einige Eigenschaften des Objekts.

        ptr<Reference_with_properties>      ref_with_props;
        ptr<Ihas_reference_with_properties> has_ref_with_props;

        HRESULT hr = has_ref_with_props.Assign_qi( iunknown );
        if( !FAILED( hr ) )
        {
            ref_with_props = has_ref_with_props->get_reference_with_properties();

            write_guid( ref_with_props->_proxy_clsid );
            
            write_int32( int_cast(ref_with_props->_properties.size()) );
            Z_FOR_EACH( Properties, ref_with_props->_properties, p )  write_string( p->first ),  write_variant( p->second );
        }
        else
        {
            write_guid( CLSID_NULL );
            write_int32( 0 );
        }
    }
}

//-----------------------------------------------------------------Output_message::write_dispparams

void Output_message::write_dispparams( const DISPPARAMS& dispparams )
{
    write_int32( dispparams.cArgs );
    write_int32( dispparams.cNamedArgs );

    // Nur [in]-Parameter!

    for( int i = 0; i < (int)dispparams.cNamedArgs; i++ )  write_int32  ( dispparams.rgdispidNamedArgs[i] );
    for( int i = 0; i < (int)dispparams.cArgs     ; i++ )  write_variant( dispparams.rgvarg[i] );
}

//------------------------------------------------------------------Output_message::write_excepinfo

void Output_message::write_excepinfo( const EXCEPINFO& xi )
{
    write_int16( xi.wCode );
    write_int16( xi.wReserved );
    write_bstr ( xi.bstrSource );
    write_bstr ( xi.bstrDescription );
    write_bstr ( xi.bstrHelpFile );
    write_int32( xi.dwHelpContext);
    write_int32( xi.scode );
}

//-----------------------------------------------------------------Input_message::Builder::add_data

void Input_message::Builder::add_data( const Byte* data, int length )
{
    const Byte* data_end = data + length;

    if( _length_bytes_read < sizeof _length_bytes )
    {
        while( _length_bytes_read < sizeof _length_bytes  &&  data < data_end )
        {
            _length_bytes[ _length_bytes_read++ ] = *data++;
        }

        if( _length_bytes_read == sizeof _length_bytes )
        {
            _length = (  (int)_length_bytes[ 0 ] << 24 ) +
                      ( (uint)_length_bytes[ 1 ] << 16 ) +
                      ( (uint)_length_bytes[ 2 ] <<  8 ) +
                      ( (uint)_length_bytes[ 3 ] <<  0 );

            if( (uint)_length > max_message_size )  throw_xc( "Z-REMOTE-115", _input_message->pid(), _length );

            _input_message->_data.erase();

            if( (int)_input_message->_data.capacity() < _length )
            {
                _input_message->_data.reserve( _length );
            }
        }
    }

    if( _length_bytes_read == sizeof _length_bytes )
    {
        _input_message->_data.append( (const char*)data, data_end - data );

        if( (int)_input_message->_data.length() >  _length )  throw_xc( "Z-REMOTE-127" );
        if( (int)_input_message->_data.length() == _length )  _is_complete = true,  _input_message->_is_complete = true;
    }
}

//------------------------------------------------------------------------Input_message::need_bytes

void Input_message::need_bytes( int byte_count )
{
    if( byte_count < 0  ||  _index + byte_count > (int)_data.length() )  throw_xc( "Z-REMOTE-109" );
}

//-------------------------------------------------------------------------Input_message::read_char

char Input_message::read_char()
{
    need_bytes( 1 );

    return _data[ _index++ ];
}

//------------------------------------------------------------------------Input_message::operator[]

char Input_message::operator[]( int index ) const
{
    if( index < 0  ||  index > length() )  throw_xc( "Z-REMOTE-109" );
    return _data[ 4 + index ];
}

//-------------------------------------------------------------------------Input_message::peek_char

char Input_message::peek_char()
{
    need_bytes( 1 );

    return _data[ _index ];
}

//------------------------------------------------------------------------Input_message::read_int16

int16 Input_message::read_int16()
{
    need_bytes( 2 );

    int16 result = (  (int)(Byte)_data[ _index + 0 ] <<  8 ) |
                   ( (uint)(Byte)_data[ _index + 1 ] <<  0 );

    _index += 2;

    return result;
}

//------------------------------------------------------------------------Input_message::read_int32

int32 Input_message::read_int32()
{
    need_bytes( 4 );

    int32 result = (  (int32)(Byte)_data[ _index + 0 ] << 24 ) |
                   ( (uint32)(Byte)_data[ _index + 1 ] << 16 ) |
                   ( (uint32)(Byte)_data[ _index + 2 ] <<  8 ) |
                   ( (uint32)(Byte)_data[ _index + 3 ] <<  0 );

    _index += 4;

    return result;
}

//------------------------------------------------------------------------Input_message::read_int64

int64 Input_message::read_int64()
{
    int32 first = read_int32();
    return (int64(first) << 32) | (uint32)read_int32();
}

//-----------------------------------------------------------------------Input_message::read_double

double Input_message::read_double()
{
    double      result;
    char        buffer [ 100+1 ];


    need_bytes( 2 );

    if( _data.data()[ _index ] != 's' )  throw_xc( Z_FUNCTION );

    int length = _data.data()[ _index + 1 ];
    if( length > sizeof buffer - 1 )  throw_xc( Z_FUNCTION );
    
    _index += 2;


    read_bytes( buffer, length );
    buffer[ length ] = '\0';

    char* end = NULL;

    set_c_locale();
    result = strtod( buffer, &end );
    if( end != buffer + length )  throw_xc( Z_FUNCTION, buffer, (int)(end - (buffer+length)) );

    return result;
}

//------------------------------------------------------------------------Input_message::read_bytes

void Input_message::read_bytes( void* p, int len )
{
    need_bytes( len );

    memcpy( p, _data.data() + _index, len );
    _index += len;
}

//-------------------------------------------------------------------------Input_message::read_guid

void Input_message::read_guid( GUID* guid )
{
    guid->Data1 = read_int32();
    guid->Data2 = read_int16();
    guid->Data3 = read_int16();
    read_bytes( guid->Data4, sizeof guid->Data4 );
}

//-----------------------------------------------------------------------Input_message::read_string

string Input_message::read_string()
{
    int byte_count = read_int32();

    need_bytes( byte_count );

    string result = _data.substr( _index, byte_count );  //iso_8859_1_from_utf8( _data.data() + _index, byte_count );

    _index += byte_count;

    return result;
}

//-------------------------------------------------------------------------Input_message::read_bstr

void Input_message::read_bstr( BSTR* bstr )
{
    int byte_count = read_int32();
    
    if( byte_count == 0 )  
    { 
        *bstr = NULL;  
    }
    else
    {
        int bstr_size  = byte_count; 

        need_bytes( byte_count );
        *bstr = bstr_from_string( _data.data() + _index, byte_count );
        _index += byte_count;

        (*bstr)[ bstr_size ] = 0;
    }
}

//---------------------------------------------------------------------Input_message::read_iunknown

ptr<IUnknown> Input_message::read_iunknown()
{
    string      title;
    string      proxy_class_name;

    Object_id   id     = read_int64();
    bool        is_new = read_char() != 0;
    
    if( is_new )
    {
        title = read_string();

        CLSID   proxy_clsid          = read_guid();
        int     local_property_count = read_int32();

        ptr<Proxy> proxy;

        if( id._value == 0 )  return NULL;

        if( proxy_clsid != CLSID_NULL )
        {
            Create_instance_function* f = _session->_server->get_class_factory( proxy_clsid );
            if( f )
            {
                ptr<IUnknown> iunknown;

                HRESULT hr = f( _session, &_session->_server->_class_register[ proxy_clsid ]._class_object, IID_Iproxy, &iunknown );
                if( FAILED( hr ) )   throw_com( hr, "create_instance", "Input_message::read_iunknown" );

                proxy = (Proxy*)(Object*)+iunknown;

                for( int i = 0; i < local_property_count; i++ )     // Lokale Properties setzen
                {
                    string name = read_string();
                    Variant value;
                    read_variant( &value );
                    proxy->set_property( name, value );
                }
            }                
            else
                Z_LOG( "*** com_remote: Proxy CLSID is unknown.\n" );
        }


        if( !proxy ) 
        {
            proxy = Z_NEW( Proxy );         //Proxy( _session, id, false, title ) );
            Variant dummy;
            for( int i = 0; i < local_property_count; i++ )  read_string(),  read_variant( &dummy );   // Properties ignorieren
        }

        proxy->set_session       ( _session );
        proxy->set_object_id     ( id );
        proxy->set_title         ( title );

        _session->_object_table.add_proxy( _session, id, proxy );

        return (Object*)+proxy;
    }
    else
    {
        return _session->object_from_id( id );
    }
}

//--------------------------------------------------------------------Input_message::read_idispatch

ptr<IDispatch> Input_message::read_idispatch()
{
    return ptr<IDispatch>( DYNAMIC_CAST( IDispatch*, +read_iunknown() ) );
}

//--------------------------------------------------------------------Input_message::read_safearray

SAFEARRAY* Input_message::read_safearray()
{
    SAFEARRAY       s;
    SAFEARRAYBOUND  b;
    
    s.cDims     = read_int16();
    s.fFeatures = read_int16();
    if( s.cDims != 1 )  throw_xc( Z_FUNCTION );

    b.cElements = read_int32();     
    b.lLbound   = read_int32();

    VARTYPE vartype = read_int32();


    SAFEARRAY* safearray = SafeArrayCreateVector( vartype, b.lLbound, b.cElements );

    try
    {
        switch( vartype )
        {
            case VT_UI1    : { Locked_any_safearray a ( safearray ); read_bytes( a->pvData, b.cElements * 1 ); break; }
          //case VT_UI4    : { Locked_any_safearray a ( safearray ); read_bytes( a->pvData, b.cElements * 4 ); break; }

            case VT_BSTR: {
                Locked_safearray<BSTR> a ( safearray ); 
                for( int i = 0; (uint)i < b.cElements; i++ )  read_bstr( &a[ i ] ); 
                break;
            }

            case VT_VARIANT: 
            { 
                Locked_safearray<VARIANT> a ( safearray ); 
                for( int i = 0; (uint)i < b.cElements; i++ )  read_variant( &a[ i ] ); 
                break;
            }

            default: throw_xc( Z_FUNCTION );
        }
    }
    catch( exception& )
    {
        SafeArrayDestroy( safearray );
        throw;
    }

    return safearray;
}

//----------------------------------------------------------------------Input_message::read_variant

void Input_message::read_variant( VARIANT* v )
{
    memset( v, 0, sizeof *v );

    v->vt = read_int32();

    if( v->vt & VT_ARRAY )
    {
        V_ARRAY( v ) = read_safearray();

        VARTYPE vartype = 0;
        HRESULT hr = z_SafeArrayGetVartype( V_ARRAY( v ), &vartype );
        if( FAILED( hr ) )  throw_com( hr, "SafeArrayGetVartype" );

        if( vartype != ( v->vt & ~VT_ARRAY ) )  throw_xc( Z_FUNCTION );
    }
    else
    {
        switch( v->vt )
        {
            case VT_EMPTY:                                          break;
            case VT_NULL:                                           break;
            case VT_I2:         V_I2      (v) = read_int16   ();    break;
            case VT_I4:         V_I4      (v) = read_int32   ();    break;
          //case VT_R4:         V_R4      (v) = read_real    ();    break;
          //case VT_R8:         V_R8      (v) = read_double  ();    break;
          //case VT_CY:
          //case VT_DATE:
            case VT_BSTR:       V_BSTR    (v) = read_bstr    ();    break;
            case VT_DISPATCH:   read_idispatch().move_to( &V_DISPATCH(v) );
                                break;
            case VT_ERROR:      V_ERROR   (v) = read_int32   ();    break;
            case VT_BOOL:       V_BOOL    (v) = read_char    ()? VARIANT_TRUE : VARIANT_FALSE;    break;
          //case VT_VARIANT:
            case VT_UNKNOWN:    read_iunknown().move_to( &V_UNKNOWN(v) );
                                break;
          //case VT_DECIMAL:
            case VT_I1:         V_I1      (v) = read_char    ();    break;
            case VT_UI1:        V_UI1     (v) = read_char    ();    break;
            case VT_UI2:        V_UI2     (v) = read_int16   ();    break;
            case VT_UI4:        V_UI4     (v) = read_int32   ();    break;
            case VT_I8:         V_I8      (v) = read_int64   ();    break;
            case VT_UI8:        V_UI8     (v) = read_int64   ();    break;
            case VT_INT:        V_INT     (v) = read_int32   ();    break;
            case VT_UINT:       V_UINT    (v) = read_int32   ();    break;
          //case VT_VOID:
          //case VT_HRESULT:
          //case VT_PTR:
          //case VT_SAFEARRAY:
          //case VT_CARRAY:
          //case VT_USERDEFINED:
          //case VT_LPSTR:
          //case VT_LPWSTR:
          //case VT_RECORD:
          //case VT_FILETIME:
          //case VT_BLOB:
          //case VT_STREAM:
          //case VT_STORAGE:
          //case VT_STREAMED_OBJECT:
          //case VT_STORED_OBJECT:
          //case VT_BLOB_OBJECT:
          //case VT_CF:
          //case VT_CLSID:

            case VT_R4:         V_R4(v) = (float)read_double();   break;
            case VT_R8:         V_R8(v) =        read_double();   break;
            case VT_CY:         V_CY(v).int64 = read_int64();            break;
            case VT_DATE:       V_DATE(v)     =  read_double();   break;

            // Diese Typen werden binär übertragen. Wenn der sendende Rechner eine andere Prozessorarchitektur hat, wird's nicht funktionieren!
         //   case VT_R4:         read_bytes   ( &V_R4(v), 4 );   break;
         //   case VT_R8:         read_bytes   ( &V_R8(v), 8 );   break;
         ////?case VT_CY:         read_bytes   ( &V_CY(v), 8 );   break;
         //   case VT_DATE:       read_bytes   ( &V_CY(v), 8 );   break;

            default:     
            {
                VARTYPE vt = v->vt;
                v->vt = VT_EMPTY; 
                throw_xc( "read_variant", vt );
            }
        }
    }

    //Z_LOG2( "DEBUG", Z_FUNCTION << " " << debug_string_from_variant( *v ) << "\n" );
}

//-------------------------------------------------------------------Input_message::read_dispparams

void Input_message::read_dispparams( Dispparams* dispparams )
{
    int n;

    n = read_int32();
    if( (uint)n > 100 )  throw_xc( "dispparam", "argcount", as_string(n) );
    dispparams->set_arg_count( n );

    n = read_int32();
    if( (uint)n > dispparams->cArgs )  throw_xc( "dispparam", "namedargcount", as_string(n) );
    dispparams->set_named_arg_count( n );

    for( int i = 0; i < (int)dispparams->cNamedArgs; i++ )  dispparams->set_dispid( i, read_int32() );
    for( int i = 0; i < (int)dispparams->cArgs     ; i++ )  read_variant( &dispparams->rgvarg[i] );
}

//--------------------------------------------------------------------Input_message::read_excepinfo

void Input_message::read_excepinfo( EXCEPINFO* xi )
{
    xi->wCode              = read_int16();
    xi->wReserved          = read_int16();
    xi->bstrSource         = read_bstr();
    xi->bstrDescription    = read_bstr();
    xi->bstrHelpFile       = read_bstr();
    xi->dwHelpContext      = read_int32();
  //xi->pvReserved );
    xi->scode              = read_int32();
}

//----------------------------------------------------------------------------Input_message::finish

void Input_message::finish()
{
    if (_index != _data.length()) throw_xc("Input_message::finish", as_string(_index), as_string(_data.length()));
}

//-------------------------------------------------------------------------------Input_message::pid

int Input_message::pid() const
{ 
    return _session? _session->pid() : 0; 
}

//---------------------------------------------------------------Simple_operation::Simple_operation

Simple_operation::Simple_operation( Session* s, IDispatch* o, const string& method, const string& debug_text )
: 
    _zero_(_end_), 
    _session( s ), 
    _output_message( s ), 
    _input_message( s ),
    _object( o ),
    _method( method ),
    _debug_text( debug_text )
{
}

//--------------------------------------------------------------Simple_operation::~Simple_operation

Simple_operation::~Simple_operation()
{
    close();
}

//--------------------------------------------------------------------------Simple_operation::close

void Simple_operation::close()
{
    if( _on_stack )
    {
        Z_LOG( "\npid=" << pid() << " *** Simple_operation::close(): Operation is on top of stack: " << async_state_text() << "\n\n" );

        if( _session )
        {
            Connection* conn = _session->connection();
            if( conn )
            {
                if( conn->current_operation() == this )
                {
                    //Nicht automatisch abräumen! Denn die Antwort vom Server ist ja noch nicht gelesen.
                    //Z_LOG( "pop_operation\n" );
                    //_session->connection()->pop_operation( _object, _method.c_str() );
                }
                else
                if( conn->current_operation() )
                {
                    Z_LOG( "\npid=" << pid() << " *** Not the same operation on top of stack: " << conn->current_operation()->async_state_text() << "\n\n" );
                }
            }
        }
    }
}

//--------------------------------------------------------------------------Simple_operation::start

bool Simple_operation::start( Connection::Different_thread_allowed different_thread_allowed )
{
    bool ok = _session->_connection->push_operation( this, different_thread_allowed );
    if (!ok) {
        assert(different_thread_allowed == Connection::Different_thread_allowed::diffthr_try);
        return false;
    }
    _state = s_starting;
    async_continue();
    return true;
}

//----------------------------------------------------------------Simple_operation::async_continue_

bool Simple_operation::async_continue_( Continue_flags flags )
{
    bool  result         = false;
    State original_state = _state;

    //if( this != _session->current_operation() )  throw_xc( "Z-REMOTE-111", "Simple_operation::async_continue", _name );
    if( async_has_error() )  return false;

    if( flags & cont_wait )  _session->connection()->check_async( this );     // Warnung ausgeben, wenn asynchroner Betrieb verlangt ist


    while(1)
    {
        bool something_done = false;

        switch( _state )
        {
            case s_starting:
            case s_sending:
            {
                something_done = send_async();
                if( _output_message.is_sent() )  _state = s_waiting;
                
                break;
            }

            case s_waiting:
            {
                _session->_connection->receive_async_start( &_input_message );
            }

            case s_receiving:
            {
                assert( &_input_message == _session->connection()->_input_message_builder._input_message );

                something_done = _session->connection()->receive_async();

                if( something_done               )  _state = s_receiving;
                if( _input_message.is_complete() )  _state = s_received;

                break;
            }

            case s_received:
            {
                if( is_callback_message() )
                {
                    if( original_state == s_starting )  // execute_callback_message() soll nicht in start(), sondern nur in async_continue() gerufen werden.
                    {
                        set_async_next_gmtime( (time_t)0 );     // 2006-05-24: Async_manager soll async_continue_() wieder aufrufen, damit wir in den else-Zweig kommen!
                    }
                    else
                    {
                        _session->connection()->execute_callback( _session, &_input_message, &_output_message );

                        _state = s_sending;
                        _input_message.clear();
                        something_done = true;
                    }
                }
                else
                {
                    _state = s_finished;
                }

                break;
            }

            case s_finished:
                return false;

            default: 
                throw_xc( "Simple_operation::async_continue", as_string(_state) );
        }

        result |= something_done;

        if( !something_done )  
        {
            if( !( flags & cont_wait ) )  break;
            _session->_connection->wait();
        }

    }

    return result;
}

//------------------------------------------------------------------Simple_operation::async_finish_

void Simple_operation::async_finish_()
{
    //if( this != _session->current_operation() )  throw_xc( "Z-REMOTE-111", "Simple_operation::async_finish", _name );

    while( !async_finished() )  async_continue( cont_wait );
}

//-------------------------------------------------------------Simple_operation::async_check_error_

void Simple_operation::async_check_error_()
{
    //if( this != _session->current_operation() )  throw_xc( "Z-REMOTE-111", "Simple_operation::async_check_error", _name );

    if( _input_message.peek_char() == msg_error ) 
    {
        _input_message.read_char();

        string code;
        string name;
        string what;

        while( !_input_message.end() )
        {
            string value = _input_message.read_string();
            
            if( string_begins_with( value, "name=" ) )  name = value.c_str() + 5;
            else
            if( string_begins_with( value, "code=" ) )  code = value.c_str() + 5;
            else
            if( string_begins_with( value, "what=" ) )  what = value.c_str() + 5;
        }

        what += ", method=";
      //if( _object )  what += _object->obj_name(), what += '.';     (Im ungünstigen Fehlerfall kann das Objekt vielleicht schon weg sein.)
        what += _method;
        what + "()";

        Xc x;
        x.set_name( name );
        x.set_code( code );
        x.set_what( what );
        throw_xc( x );
    }
    else
    if( _input_message.peek_char() != msg_answer )  
        throw_xc( "Z-REMOTE-107", pid() );
}

//---------------------------------------------------------------------------Simple_operation::send

void Simple_operation::send()
{ 
    _output_message.send(); 
}

//---------------------------------------------------------------------Simple_operation::send_async

bool Simple_operation::send_async()
{ 
    return _output_message.send_async(); 
}

//--------------------------------------------------------------------Simple_operation::async_kill_

bool Simple_operation::async_kill_()
{
    if( _session->connection() ) {  _session->connection()->kill_process();  return true; }
    return false;
}

//--------------------------------------------------------------Simple_operation::async_state_text_

string Simple_operation::async_state_text_() const
{ 
    string text = "Simple_operation(";
    if( async_finished() && async_has_error() )  text +="ERROR,";
    text += "state=",  text += state_name();
  //if( _object )  text += ",object=" + _object->obj_name();       (Im ungünstigen Fehlerfall kann das Objekt vielleicht schon weg sein.)
    text += ",method=",  text += _method;
    if( !_debug_text.empty() )  text += '(', text += _debug_text, text += ')';
    text += ")"; 
    return text;
}

//---------------------------------------------------------------------Simple_operation::state_name

string Simple_operation::state_name() const
{
    switch( _state )
    {
        case s_none      : return "none";
        case s_starting  : return "starting";
        case s_sending   : return "sending";
        case s_waiting   : return "waiting";
        case s_receiving : return "receiving";
        case s_received  : return "received";
        case s_finished  : return "finished";
        default: return as_string(_state);
    }
}

//------------------------------------------------------------------------------------Proxy::~Proxy

Proxy::~Proxy()
{
    try
    {
        release();
    }
    catch( exception& x ) { Z_LOG( "ERROR in " << Z_FUNCTION << ", calling release(): " << x.what() << "\n" ); }
}

//-----------------------------------------------------------------------------------Proxy::release

void Proxy::release()
{
    if( _object_id._value != 0 )
    {
        if( _connection && _connection->_is_async )  
        {
            release_finish();
            throw_xc( "Z-REMOTE-129", _title, "IUnknown::Release" );  // 2006-06-13
        }

        release__start() -> async_finish();
        release__end();
    }
}

//----------------------------------------------------------------------------Proxy::release_finish

void Proxy::release_finish()
{
    _session->_object_table.remove( _object_id );
    _object_id = 0;
}

//----------------------------------------------------------------------------Proxy::release__start

Async_operation* Proxy::release__start()
{
    if( _session->connection()->has_error() )
    {
        _no_operation = true;
        _sync_operation = Z_NEW(Sync_operation);
        return _sync_operation;
    }


    ptr<Simple_operation> operation = Z_NEW( Simple_operation( _session, this, "Release" ) );

    operation->_output_message.write_char( msg_object );
    operation->_output_message.write_int64( _object_id._value );
    operation->_output_message.write_char( cmd_release );
    operation->_output_message.finish();

    operation->start();

    return operation;
}

//------------------------------------------------------------------------------Proxy::release__end

void Proxy::release__end()
{
    if( !_no_operation )
    {
        ptr<Simple_operation> operation = _session->_connection->pop_operation( this, "Release" );

        operation->_input_message.read_char();     // msg_answer
        operation->_input_message.finish();
    }

    release_finish();
}

//----------------------------------------------------------------------------Proxy::QueryInterface

STDMETHODIMP Proxy::QueryInterface( const IID& iid, void** result )
{
    //return Object::QueryInterface( iid, result );

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

/*
    Wir brachen QueryInterface für VBScript-Iteratoren (NewEnum bei spooler_task.params(), Variable_set in spooler_com.cxx).
    Die implementieren wir aber noch nicht, denn VBScript will außerdem den TypeInfo abfragen und der gelieferte Iterator scheint kein Objekt zu sein. usw.
    Ich breche ab. Gegen einen Tag Berechnung setze ich fort.

    Die Aufrufe von QueryInterace() müssen gefiltert werden, denn VBSript macht davon sehr viele (fragt nach z.B: IDispatchEx und so).
    Das bremst. Also klemmen wir den ganzen Aufruf erstmal ab. Obwohl er funktioniert.

    Joacim 14.1.2004

        if( _connection->_is_async )  throw_xc( "Z-REMOTE-129", _title, string_from_bstr( names[0] ) );  // 2006-06-13

               QueryInterface__start( iid ) -> async_finish();
        return QueryInterface__end( (IUnknown**)result );
*/
    }
}

//----------------------------------------------------------------------------QueryInterface__start

Simple_operation* Proxy::QueryInterface__start( const IID& iid )
{
    ptr<Simple_operation> op = Z_NEW( Simple_operation( _session, this, "QueryInterface" ) );

    op->_output_message.write_char ( msg_object );

    op->_output_message.write_int64( _object_id._value );
    op->_output_message.write_char ( cmd_queryinterface );
    op->_output_message.write_guid ( iid );

    op->_output_message.finish();
    op->start();

    return op;
}

//------------------------------------------------------------------------------QueryInterface__end

STDMETHODIMP Proxy::QueryInterface__end( IUnknown** result )
{
    HRESULT hr;

    ptr<Simple_operation> op = _session->connection()->pop_operation( this, "QueryInterface" );

    op->_input_message.read_char();     // msg_answer
    
    hr = op->_input_message.read_int32();
    if( FAILED( hr ) )   return hr;

    op->_input_message.read_iunknown().move_to( result );

    op->_input_message.finish();

    return hr;
}

//-----------------------------------------------------------------------------Proxy::GetIDsOfNames

STDMETHODIMP Proxy::GetIDsOfNames( const IID& iid, OLECHAR** names, uint names_count, LCID lcid, DISPID* result )
{
    HRESULT hr;

    try
    {
        if( names_count == 1 )  
        {
            Dispid_map::iterator it = _dispid_map.find( names[0] );     // LCID wird nicht berücksichtigt
            if( it != _dispid_map.end() )
            {
                *result = it->second;
                return S_OK;
            }
        }

        if( _connection->_is_async )  throw_xc( "Z-REMOTE-129", _title, string_from_bstr( names[0] ) );  // 2006-06-13

             GetIDsOfNames__start( iid, names, names_count, lcid ) -> async_finish();
        hr = GetIDsOfNames__end  ( names_count, result );


        if( names_count == 1 )  _dispid_map[ names[0] ] = *result;      // LCID wird nicht berücksichtigt
    }
    catch( const exception& x ) { hr = Com_set_error(x); }

    return hr;
}

//----------------------------------------------------------------------Proxy::GetIDsOfNames__start

Simple_operation* Proxy::GetIDsOfNames__start( const IID& iid, OLECHAR** names, uint names_count, LCID lcid )
{
    ptr<Simple_operation> op = Z_NEW( Simple_operation( _session, this, "GetIDsOfNames" ) );

    if( names_count > 0 )  op->_debug_text = string_from_ole( names[0] );

    op->_output_message.write_char ( msg_object );

    op->_output_message.write_int64( _object_id._value );
    op->_output_message.write_char ( cmd_getidsofnames );
    op->_output_message.write_guid ( iid );
    op->_output_message.write_int32( lcid );
    op->_output_message.write_int32( names_count );

    //Z_LOG2("zschimmer", Z_FUNCTION << " names[0]=" << string_from_ole( names[0] ) << "\n" );
    for( int i = 0; i < (int)names_count; i++ )  op->_output_message.write_string( names[i] );

    op->_output_message.finish();
    op->start();

    return op;
}

//------------------------------------------------------------------------Proxy::GetIDsOfNames__end

STDMETHODIMP Proxy::GetIDsOfNames__end( uint names_count, DISPID* result )
{
    HRESULT hr;

    ptr<Simple_operation> op = _session->connection()->pop_operation( this, "GetIDsOfNames" );

    op->_input_message.read_char();     // msg_answer
    hr = op->_input_message.read_int32();
    if( FAILED( hr ) )   return hr;

    for( int i = 0; i < (int)names_count; i++ )  result[i] = op->_input_message.read_int32();
    op->_input_message.finish();

    return hr;
}

//------------------------------------------------------------------------------------Proxy::Invoke

STDMETHODIMP Proxy::Invoke( DISPID dispid, const IID& iid, LCID lcid, WORD flags, DISPPARAMS* dispparams, VARIANT* result, EXCEPINFO* excepinfo, UINT* errarg )
{
    HRESULT hr;

    try
    {
        if( _connection->_is_async )  throw_xc( "Z-REMOTE-129", _title, dispid );  // 2006-06-13

             Invoke__start( dispid, iid, lcid, flags, dispparams ) -> async_finish();
        hr = Invoke__end  ( result, excepinfo, errarg );
    }
    catch( const exception& x ) { hr = Com_set_error( x ); }

    return hr;
}

//--------------------------------------------------------------------Proxy::Invoke_from_any_thread

STDMETHODIMP Proxy::Invoke_from_any_thread( DISPID dispid, const IID& iid, LCID lcid, WORD flags, DISPPARAMS* dispparams, VARIANT* result, EXCEPINFO* excepinfo, UINT* errarg )
{
    HRESULT hr;

    if (_connection->_new_error) {
        Z_LOG("Proxy::Invoke_from_any_thread(): _connection->_new_error\n");
        return E_FAIL;
    }

    try
    {
        if( _connection->_is_async )  throw_xc( "Z-REMOTE-129", _title, dispid );  // 2006-06-13

             Invoke__start( dispid, iid, lcid, flags, dispparams, Connection::diffthr_thread_allowed ) -> async_finish();
        hr = Invoke__end  ( result, excepinfo, errarg );
    }
    catch( const exception& x ) { hr = Com_set_error( x ); }

    return hr;
}

//-----------------------------------------------------------------------------Proxy::Invoke__start

Simple_operation* Proxy::Invoke__start( DISPID dispid, const IID& iid, LCID lcid, WORD flags, DISPPARAMS* dispparams,
                                        Connection::Different_thread_allowed different_thread_allowed )
{
    //if( _proxy_class_descriptor )  throw_xc( "Z-REMOTE-127", _proxy_class_descriptor->title() );


    ptr<Simple_operation> op = Z_NEW( Simple_operation( _session, this, "Invoke" ) );

    op->_output_message.write_char ( msg_object );
    op->_output_message.write_int64( _object_id._value );

    op->_output_message.write_char ( cmd_invoke );
    op->_output_message.write_int32( dispid );
    op->_output_message.write_guid ( iid );
    op->_output_message.write_int32( lcid );
    op->_output_message.write_int32( flags );
    op->_output_message.write_dispparams( *dispparams );
    op->_output_message.finish();

    op->start( different_thread_allowed );

    return op;
}

//-------------------------------------------------------------------------------Proxy::Invoke__end

STDMETHODIMP Proxy::Invoke__end( VARIANT* result, EXCEPINFO* excepinfo, UINT* errarg )
{
    HRESULT hr;

    ptr<Simple_operation> op = _session->connection()->pop_operation( this, "Invoke" );

    op->_input_message.read_char();     // msg_answer!

    hr = op->_input_message.read_int32();

    if( !FAILED(hr) )
    {
        if( result ) 
        {
            op->_input_message.read_variant( result );
        }
        else
        {
            Variant dummy;
            op->_input_message.read_variant( &dummy );
        }
    }
    else
    {
        *errarg = op->_input_message.read_int32();
        op->_input_message.read_excepinfo( excepinfo );
    }

    return hr;
}

//--------------------------------------------------------------------------------------Proxy::call

Variant Proxy::call( const string& method, const Variant& p1, const Variant& p2, const Variant& p3, const Variant& p4, const Variant& p5 )
{
    call__start( method, p1, p2, p3, p4, p5 ) -> async_finish();
    return call__end();
}

//-------------------------------------------------------------------------------Proxy::call__start

Simple_operation* Proxy::call__start( const string& method, const Variant& p1, const Variant& p2, const Variant& p3, const Variant& p4, const Variant& p5 )
{
    ptr<Simple_operation> operation = Z_NEW( Simple_operation( _session, this, "call" ) );

    operation->_debug_text = method;

    operation->_output_message.write_char  ( msg_object );
    operation->_output_message.write_int64 ( _object_id._value );
    operation->_output_message.write_char  ( cmd_call   );
    operation->_output_message.write_string( method     );

    {
        int n = 0;
        if( V_VT(&p1) != VT_ERROR  &&  V_ERROR(&p1) != DISP_E_PARAMNOTFOUND )  n = 1;
        if( V_VT(&p2) != VT_ERROR  &&  V_ERROR(&p2) != DISP_E_PARAMNOTFOUND )  n = 2;
        if( V_VT(&p3) != VT_ERROR  &&  V_ERROR(&p3) != DISP_E_PARAMNOTFOUND )  n = 3;
        if( V_VT(&p4) != VT_ERROR  &&  V_ERROR(&p4) != DISP_E_PARAMNOTFOUND )  n = 4;
        if( V_VT(&p5) != VT_ERROR  &&  V_ERROR(&p5) != DISP_E_PARAMNOTFOUND )  n = 5;

        Dispparams dispparams;

        dispparams.set_arg_count( n );

        if( n >= 1 )  dispparams[0] = p1;
        if( n >= 2 )  dispparams[1] = p2;
        if( n >= 3 )  dispparams[2] = p3;
        if( n >= 4 )  dispparams[3] = p4;
        if( n >= 5 )  dispparams[4] = p5;

        operation->_output_message.write_dispparams( dispparams );

#       ifdef _DEBUG
            operation->_debug_text += '(';
            for( uint i = 0; i < dispparams.cArgs; i++ )  operation->_debug_text += debug_string_from_variant(dispparams[i]).substr(0,100) + ',';
            operation->_debug_text += ')';
#       endif
    }

    operation->_output_message.finish();

    operation->start();

    return operation;
}

//---------------------------------------------------------------------------------Proxy::call__end

Variant Proxy::call__end()
{
    Variant result;

    ptr<Simple_operation> operation = _session->_connection->pop_operation( this, "call" );

    operation->_input_message.read_char();     // msg_answer!

    HRESULT hr = operation->_input_message.read_int32();
    if( FAILED(hr) )  throw_com( hr, operation->async_state_text().c_str() );

    operation->_input_message.read_variant( &result );
    operation->_input_message.finish();

    return result;
}

//--------------------------------------------------------------------Proxy::check_connection_error

void Proxy::check_connection_error()
{
    if( _session && _session->_connection )  _session->_connection->async_check_error( Z_FUNCTION );
}

//--------------------------------------------------------------------------Proxy::name_from_dispid

const Bstr& Proxy::name_from_dispid( DISPID dispid )
{
    Z_FOR_EACH( Dispid_map, _dispid_map, d )
    {
        if( d->second == dispid )  return d->first;
    }

    throw_xc( "Z-REMOTE-125", as_string(dispid), obj_name() );
}

//------------------------------------------------------------------------------Proxy::set_property

void Proxy::set_property( const string& name, const Variant& )
{
    throw_xc( "Z-REMOTE-126", _title, name );
}

//-----------------------------------------------------------------------------------Server::Server

Server::Server() 
: 
    _zero_(this+1)
{
}

//---------------------------------------------------------------------------Server::register_class

void Server::register_class( const CLSID& clsid, Create_instance_function* f )
{ 
    _class_register[clsid] = Class_entry( f ); 
}

//------------------------------------------------------------------------Server::get_class_factory

Create_instance_function* Server::get_class_factory( const CLSID& clsid )
{
    Class_register::iterator it = _class_register.find( clsid );
    return it == _class_register.end()? NULL : it->second._function;
}

//-----------------------------------------------------------------Server::get_class_object_or_null

Object* Server::get_class_object_or_null( const CLSID& clsid )
{ 
    Object* result = NULL;

    Class_register::iterator it = _class_register.find( clsid );
    if( it != _class_register.end() )  result = it->second._class_object;

    return result;
}

//-----------------------------------------------------------------------------------Session::close

void Session::close()
{
    close__start() -> async_finish();
    close__end();
}

//-----------------------------------------------------------------------------Session::server_loop

void Session::server_loop(int keep_alive_timeout)
{
    Connection::In_exclusive_mode in_exclusive_mode ( _connection );       // Ruft _connection->enter_exclusive_mode();
    ptr<Keep_alive_thread> keep_alive_thread = keep_alive_timeout < INT_MAX? Z_NEW(Keep_alive_thread(this, keep_alive_timeout)) : NULL;
    try {
        if (keep_alive_thread) {
            keep_alive_thread->start();
        }
        while(1)
        {
            Input_message  input_message  ( this );
            Output_message output_message ( this );

            bool eof = false;
            _connection->receive( &input_message, &eof );
            if( eof )  break;

            bool connection_lost = false;
            _connection->execute( this, &input_message, &output_message );

            send_response( &output_message, &connection_lost );

            if( connection_lost )  break;
        }
        if (keep_alive_thread) {
            keep_alive_thread->stop();
        }
    } catch (exception&) {
        if (keep_alive_thread) {
            keep_alive_thread->stop();
        }
        throw;
    }

    // ~In_exclusive_mode() ruft _connection->leave_exclusive_mode();
}

//-----------------------------------------------------------------------------------Server::server

void Server::server( int server_port )
{
#ifdef Z_WINDOWS
    __assume( server_port );
    throw_xc( "NO-FORK" );
#else
    SOCKET listen_socket;
    int    ret;


    ptr<Connection_manager> connection_manager = Z_NEW( Connection_manager );      // Brauchen wir den?
    
    _connection = Z_NEW( Connection( connection_manager ) );
    
    listen_socket = socket( AF_INET, SOCK_STREAM, 0 );
    if( listen_socket == SOCKET_ERROR )  throw_socket( errno, "socket" );

    set_linger( listen_socket );


    sockaddr_in addr;
    memset( &addr, 0, sizeof addr );
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons( server_port );


    ret = ::bind( listen_socket, (sockaddr*)&addr, sizeof addr );
    if( ret == SOCKET_ERROR )  throw_socket( errno, "bind" );

    ret = ::listen( listen_socket, 5 );
    if( ret == SOCKET_ERROR )  throw_socket( errno, "listen" );


    while(1)
    {
        sockaddrlen_t len;
        sockaddr_in peer_addr;
        memset( &peer_addr, 0, sizeof peer_addr );

        len = sizeof peer_addr;

        _connection->_socket = ::accept( listen_socket, (sockaddr*)&peer_addr, &len );
        if( _connection->_socket == SOCKET_ERROR )  throw_socket( errno, "accept" );

        _connection->_peer = peer_addr;

        int pid = fork();
        switch( pid )
        {
            case -1:
                fprintf( stderr, "fork() returns ERRNO-%d %s\n", errno, strerror(errno) );
                _connection->close_socket( &_connection->_socket );
                //_connection->_socket = SOCKET_ERROR;
                break;

            case 0:
            {
                zschimmer::main_pid = getpid();
                //setsid();  // Brauchen wir das? Damit soll dieser Prozess nicht sterben, wenn der Superserver stirbt. ps zeigt dann leider nicht mehr die schöne Baumstruktur

                Session session(this, _connection, (jobject)NULL);
                session.server_loop();
                session.close();
                break;
            }

            default:
            {
                fprintf( stderr, "Process %d has been started for %s\n", pid, _connection->_peer.as_string().c_str() );
                //?closesocket( _connection->_socket );
                _connection->_socket = SOCKET_ERROR;
            }
        }
    }

    _connection->close();
# endif
}

//----------------------------------------------------------------------------Server::simple_server

void Server::simple_server(const Host_and_port& controller_address, jobject injectorJ, int keep_alive_timeout)
{
    Z_LOGI2("Z-REMOTE-118", Z_FUNCTION << " " << controller_address << "\n");
    ptr<Connection_manager> connection_manager = Z_NEW( Connection_manager );      // Brauchen wir den?
    
    _connection = Z_NEW( Connection( connection_manager, this ) );

    if( controller_address.port() != 0 )  _connection->connect( controller_address );
                                    else  _connection->_socket = as_int( getenv( socket_environment_name.c_str() ) );
   
    Session session(this, _connection, injectorJ);

    session.server_loop(keep_alive_timeout);
    session.close();

    _connection->close__start() -> async_finish();
    _connection->close__end();
    Z_LOG2("Z-REMOTE-118", Z_FUNCTION << " okay\n");
}

//-------------------------------------------------------------------------------Server::stdin_data

string Server::stdin_data()
{
    return _has_stdin_data? _stdin_data :
                            File( STDIN_FILENO ).read_all();
}

//-------------------------------------------------------------------------------------Server::main

int Server::main( int argc, char** argv )
{
    // für Testzwecke auskommentieren. Prozess stoppt hier -> IDE: Extras|An den Prozess anhängen
    //MessageBox(NULL,( "object_server pid=" + as_string(getpid()) ).c_str(),"",0);
    //Z_DEBUG_ONLY( Sleep((connect_timeout*10)*1000); )
    //Z_DEBUG_ONLY( Sleep(10*1000); )

    bool   server          = false;
    int    server_port     = 9000;
    string controller_ip;
    int    controller_port = 0;
    int keep_alive_timeout = INT_MAX;


    for( int i = 1; i < argc; i++ )
    {
        if( string_begins_with( argv[i], "-controller=" ) ) 
        {
            string value = argv[i] + 12;
            size_t colon = value.find( ':' );
            
            if( colon == string::npos )  controller_port = as_int( value );
            else
            {
                controller_ip.assign( value.data(), colon ); 
                controller_port = as_int( value.substr( colon + 1 ) );
            }
        }
        else
        if( string( argv[i] ) == "-server" )  server = true;
        else
        if (string_begins_with(argv[i], "-keep-alive=")) {
            string value = argv[i] + 12;
            keep_alive_timeout = as_int(value);
        }

    }

    if( server )
    {
        this->server( server_port );
    }
    else
    {
        simple_server(Host_and_port(controller_ip, controller_port), (jobject)NULL, keep_alive_timeout);
    }

    return 0;
}

//-------------------------------------------------------------------------------------------------
 
} //namespace object_server
} //namespace com
} //namespace zschimmer
