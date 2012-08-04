// $Id: com_remote.h 13454 2008-03-06 12:24:46Z jz $

#ifndef __Z_COM_REMOTE_H
#define __Z_COM_REMOTE_H


#include "com.h"
#include "z_com_server.h"       // Com_class_descriptor
#include "file.h"
#include "z_com.h"
#include "z_sockets.h"
#include "threads.h"
#include "async.h"
#include "async_socket.h"
#include "Login.h"

namespace zschimmer {

//-------------------------------------------------------------------------------------------------

namespace com {
namespace object_server {

const int    initial_message_size   = 2000;
const int    max_message_size       = 1000*1024*1024;   // Prüfung, falls unsinnige Bytes ankommen
const int    max_dispids_per_method = 1;                // Parameternamen bei GetIDsOfNames() nicht implementiert

struct Server;
struct Connection;
struct Connection_manager;
struct Session;
struct Proxy;
struct Simple_operation;
struct Output_message;
struct Input_message;

//-----------------------------------------------------------------------Connection_reset_exception
// Connection_reset_exception wird vom Scheduler geprüft, um Fehlermeldungen aufgrund 
// eines Kills zu erkennen. Wenn der Kill (z.B. ignore_signals="SIGTERM") den Job nicht stoppen soll,
// dann soll auch die vorherige Fehlermeldung wegen Verbindungsabbruchs den Job nicht stoppen.
// Nur Verbindungsfehler, die durch ein Signal (kill, Absturz) hervorgerufen werden können,
// sollten hiermit abgedeckt werden.
// Ebenso, wenn der Prozess sich unerwartet beendet hat (_process_lost).

struct Connection_reset_exception : Xc
{
    static const string&   exception_name;

    Connection_reset_exception( const string& code );
    Connection_reset_exception( const string& code, int pid );
};

//------------------------------------------------------------------------------------Message_class

enum Message_class
{
    msg_none            = 'N',      // Nachricht bezieht sich auf keine Session oder Objekt
    msg_session         = 'S',      // Nachricht bezieht sich auf eine Session (bezeichnet durch die folgenden Bytes)
    msg_object          = 'O',      // Nachricht bezieht sich auf ein Objekt (bezeichnet durch die folgenden Bytes)
    msg_answer          = 'A',      // Nachricht ist eine Antwort
    msg_error           = 'E'       // Nachricht ist eine Fehlerantwort
};

//--------------------------------------------------------------------------------------Message_cmd

enum Message_cmd
{
  //cmd_login           = 'L',      // Wird als erste Nachricht vom Server an den Controller geschickt und nicht beantwortet 
    cmd_createinstance  = 'C',
    cmd_release         = 'R',
    cmd_queryinterface  = 'Q',
    cmd_getidsofnames   = 'G',
    cmd_invoke          = 'I',
    cmd_call            = 'A'
};

//-----------------------------------------------------------------------------------------Dispname

struct Dispname
{
    bool                        equals                  ( OLECHAR**, int names_count, LCID );

    LCID                       _lcid;
  //vector<string>             _names;
    string                     _name;
};

//---------------------------------------------------------------------------------------Dispid_map
/*
struct Dispid_map
{
    void                        add                     ( OLECHAR**, LCID, DISPID );
    bool                        get                     ( OLECHAR**, LCID, DISPID* );


    typedef std::map< Dispname, DISPID >  Map;

    Map                        _map;
};
*/
//---------------------------------------------------------------------------------------Parameters

typedef std::pair< string, string >  Parameter;             // Name, Wert
typedef std::list< Parameter >       Parameters;            

//-------------------------------------------------------------------------------------------Buffer

struct Buffer
{
    Buffer( int size ) 
    : 
        _buffer_size( size ), 
        _buffer( new Byte[ _buffer_size ] ) 
    {
        if( !_buffer )  throw_xc( "Z-4007" );
    }

    ~Buffer()
    {
        delete[] _buffer;
    }

    int             _buffer_size;
    Byte*           _buffer;
};

//------------------------------------------------------------------------------------Input_message

struct Input_message
{
    struct Builder
    {
                                Builder                 ( Input_message* m = NULL )                 : _zero_(this+1), _input_message(m) {}


        void                    start                   ( Input_message* m )                        { _input_message = m; _length_bytes_read = 0; _is_complete = false; }
        void                    add_data                ( const Byte* data, int length );
        bool                    is_complete             ()                                          { return _is_complete; }
        
        Fill_zero              _zero_;
        Input_message*         _input_message;
        Byte                   _length_bytes [4];
        int                    _length_bytes_read;
        int                    _length;
        bool                   _is_complete;
    };


                                Input_message           ( Session* s )                              : _zero_(this+1),_session(s) {}

    void                        clear                   ()                                          { _index = 0; 
                                                                                                      _data.erase(); 
                                                                                                      _is_answer = false; 
                                                                                                      _is_complete = false; 
                                                                                                    }

    bool                        end                     ()                                          { return (uint)_index >= _data.length(); }
    void                        need_bytes              ( int byte_count );
    int                         length                  () const                                    { return _data.length() - 4; }

    char                        operator []             ( int index ) const;
    char                        peek_char               ();
    char                        read_char               ();
    int16                       read_int16              ();
    int32                       read_int32              ();
    int64                       read_int64              ();
    double                      read_double             ();
    void                        read_guid               ( GUID* );
    GUID                        read_guid               ()                                          { GUID guid; read_guid( &guid ); return guid; }
    ptr<IUnknown>               read_iunknown           ();
    ptr<IDispatch>              read_idispatch          ();
    string                      read_string             ();
    void                        read_bstr               ( BSTR* );
    BSTR                        read_bstr               ()                                          { BSTR bstr;  read_bstr(&bstr);  return bstr; }
    SAFEARRAY*                  read_safearray          ();
    void                        read_variant            ( VARIANT* );
    void                        read_bytes              ( void*, int length );
    void                        read_dispparams         ( Dispparams* );
    void                        read_excepinfo          ( EXCEPINFO* );
    void                        finish                  ();
    bool                        is_complete             ()                                          { return _is_complete; }
    
    int                         pid                     () const;


    Fill_zero                  _zero_;
    Session*                   _session;
    int                        _index;
    bool                       _is_complete;
    bool                       _is_answer;
    string                     _data;
};

//---------------------------------------------------------------------------------------Connection

struct Connection : Object,
                    My_thread_only      // Erstmal nur für einen Thread zulassen.
{
    enum Different_thread_allowed { diffthr_not_allowed = 0, diffthr_thread_allowed };


    struct Connect_operation : Async_operation
    {
        enum State
        {
            s_initial,
            s_waiting_for_connection,
            s_writing_to_stdin,
            s_ok
        };


                                Connect_operation       ( Connection* );
        Z_GNU_ONLY(             Connect_operation       (); )                                

        virtual bool            async_continue_         ( Continue_flags );
        virtual bool            async_finished_         () const                                    { return _connection->connected(); }
        virtual bool            async_has_error_        () const                                    { return _connection->_last_errno != 0 || _connection->_timedout | _connection->_process_lost; }
        virtual void            async_check_error_      ();
        virtual string          async_state_text_       () const;
        virtual bool            async_kill_             ();
        virtual void            async_abort_            ();
        virtual int             async_pid               ()                                          { return _connection->pid(); }

        int                     pid                     ()                                          { return async_pid(); }


        Fill_zero              _zero_;
        State                  _state;
        Connection*            _connection;
        time_t                 _timeout;
        size_t                 _stdin_written;
    };


    struct In_exclusive_mode
    {
                                In_exclusive_mode       ( Connection* c )                           : _connection(c) { _connection->enter_exclusive_mode( __FUNCTION__ ); }
                               ~In_exclusive_mode       ()                                          { _connection->leave_exclusive_mode( __FUNCTION__ ); }

        Connection* const      _connection;
    };


                                Connection              ( Connection_manager*, Server* = NULL );
    virtual                    ~Connection              ();

    void                        close                   ();
    virtual Async_operation*    close__start            ();
    virtual void                close__end              ();

    Async_operation*            current_operation       ()                                          { return _operation_stack.empty()? (Async_operation*)+_my_operation 
                                                                                                                                     : (Async_operation*)+_operation_stack.top(); }
    Async_operation*            current_super_operation ();
    bool                        in_operation            ()                                          { return !_operation_stack.empty(); }

    void                        listen_on_tcp_port      ( const Ip_address& my_ip_address );
    int                         tcp_port                () const                                    { return _tcp_port; }

    virtual bool                kill_process            ()                                          { return false; }
    virtual bool                process_terminated      ()                                          { return false; }

    void                        connect                 ( const Host_and_port& controller );


    void                    set_controller_address      ( const Host_and_port& a )                  { _controller_address = a; }
    void                    set_remote_host             ( const Host& h )                           { _remote_host = h; }
    void                    set_stdin_data              ( const string& data )                      { _stdin_data = data; _has_stdin_data = true; }
    bool                    has_stdin_data              () const                                    { return _has_stdin_data; }
    string                      stdin_data              () const                                    { return _stdin_data; }
    void                    set_event                   ( Event* );

    void                    set_async                   ();
    bool                        is_async                ()                                          { return _is_async; }

    bool                        connected               () const                                    { return _socket != SOCKET_ERROR; }

    Server*                     server                  () const                                    { return _server; }

    bool                        async_continue          ();
    void                        async_check_error       ( const string& text = "" );
    double                      async_next_gmtime       ();

    void                        check_async             ( Async_operation* );                       // Warnung ausgeben, wenn asynchroner Betrieb verlangt ist
    void                        check_connection_error  ();

    string                      state_text              ();

    
    Connect_operation*          connect_server__start   ();
    void                        connect_server__end     ();

    pid_t                       pid                     () const                                    { return _pid; }
    virtual Process_handle      process_handle          ()                                          { return 0; }

#   ifdef Z_WINDOWS
        virtual windows::Event* process_event           ()                                          { return NULL; }
#   endif


    bool                        has_error               () const                                    { return _last_errno != 0  ||  _killed  ||  _broken; }

    // Statistik
  //time_t                      running_since           () const                                    { return _running_since; }
    int                         operation_count         () const                                    { return _operation_count; }
    int                         callback_count          () const                                    { return _callback_count; }

    int                         last_errno              () const                                    { return _last_errno; }
    bool                        timedout                () const                                    { return _timedout; }
    virtual int                 exit_code               ()                                          { return 0; }
    virtual int                 termination_signal      ()                                          { return 0; }

    virtual string              short_name              () const;
    string                      obj_name                () const;

  protected:
    friend struct Server;
    friend struct Session;
    friend struct Proxy;
    friend struct Simple_operation;
    friend struct Output_message;
    friend struct Input_message;


    int                         send_async              ( const void*, int );

    void                        receive                 ( Input_message*, bool* eof = NULL );
    void                        receive_async_start     ( Input_message* );
    bool                        receive_async           ( bool* eof = NULL );
    int                         read                    ( void* buffer, int size, bool* eof = NULL );
    int                         read_async              ( void* buffer, int size, bool* eof = NULL );
    void                        wait                    ();
    int                         close_socket            ( SOCKET* );
    void                        check_socket_error      ( int errn, const string& );

    void                        execute                 ( Session*, Input_message*, Output_message* );
    void                        execute_callback        ( Session*, Input_message*, Output_message* );
    void                        enter_exclusive_mode    ( const char* debug_text );
    void                        leave_exclusive_mode    ( const char* debug_text );
    void                        assert_right_thread     ();

    void                        push_operation          ( Simple_operation*, Different_thread_allowed );
    ptr<Simple_operation>       pop_operation           ( const IDispatch*, const char* method );

    void                        update_select_fdsets    ();


    Fill_zero                  _zero_;
    pid_t                      _pid;
    Mutex                      _exclusive_io_mutex;
    Thread_id                  _in_use_by_thread_id;
  //ptr<Socket_operation>      _listen_socket;
  //ptr<Socket_operation>      _socket;
    Host                       _remote_host;
    Host_and_port              _peer;
    bool                       _wsastartup_called;
    int                        _tcp_port;
    SOCKET                     _listen_socket;
    SOCKET                     _socket;
    bool                       _select_thread_started;
    Host_and_port              _controller_address;
    string                     _stdin_data;
    bool                       _has_stdin_data;
  //SOCKET                     _stdin_of_process;
    Event*                     _event;
    Input_message::Builder     _input_message_builder;
  //Thread::Id                 _thread_id;
    std::stack< ptr<Simple_operation> >   _operation_stack;        // n > 1 bei Callback
    int                        _callback_nesting;
    ptr<Async_operation>       _my_operation;
    bool                       _killed;

    bool                       _is_async;
    bool                       _new_error;
    bool                       _timedout;
    bool                       _process_lost;
    bool                       _close_abnormally;

  //time_t                     _running_since;
    int                        _operation_count;
    int                        _callback_count;

    Server*                    _server;                             // NULL, wenn Client-Seite
    ptr<Connection_manager>    _manager;

  public:
    int                        _last_errno;
    bool                       _broken;
};

//-----------------------------------------------------------------Connection_to_own_server_process

struct Connection_to_own_server_process : Connection
{
    struct Wait_for_process_termination : Async_operation
    {
                                Wait_for_process_termination( Connection_to_own_server_process* );
        Z_GNU_ONLY(             Wait_for_process_termination(); )                                

        virtual bool            async_continue_         ( Continue_flags );
        virtual bool            async_finished_         () const                                    { return _connection->pid() == 0; }
        virtual bool            async_has_error_        () const                                    { return _connection->_last_errno != 0; }
        virtual void            async_check_error_      ();
        virtual string          async_state_text_       () const;
        virtual bool            async_kill_             ();
        virtual int             async_pid               ()                                          { return _connection->pid(); }

        int                     pid                     ()                                          { return async_pid(); }

        Fill_zero              _zero_;
        Connection_to_own_server_process* _connection;
    };



                                Connection_to_own_server_process( Connection_manager* m )           : Connection( m ), _zero_(this+1) {}
                               ~Connection_to_own_server_process();

    void                        start_process           ( const Parameters& );
    bool                        call_waitpid            ( bool wait );
    void                        open_stdout_stderr_files();
    void                    set_login                   (Login* o)                                  { _login = o; }
    void                    set_priority                ( const string& priority )                  { _priority = priority; }
    void                    set_environment_string      ( const string& env )                       { _environment_string = env;  _has_environment = true; }
    file::File_path             stdout_path             ()                                          { return _stdout_file.path(); }
    file::File_path             stderr_path             ()                                          { return _stderr_file.path(); }
    bool                        try_delete_files        ( Has_log* = NULL );
    std::list<file::File_path>  undeleted_files         ();
    int                         exit_code               ()                                          { return (int)_exit_code; }
    int                         termination_signal      ()                                          { return _termination_signal; }

    virtual Async_operation*    close__start            ();
    virtual void                close__end              ();

    virtual bool                kill_process            ();
    virtual bool                process_terminated      ();
    virtual string              short_name              () const;
    string                      obj_name                () const;


    Fill_zero                  _zero_;
    ptr<Login>                 _login;
    string                     _priority;
    string                     _environment_string;
    bool                       _has_environment;
    file::File                 _stdin_file;
    file::File                 _stdout_file;
    file::File                 _stderr_file;
    int                        _termination_signal;     // Nur Unix

#   ifdef Z_WINDOWS

        Process_handle          process_handle          ()                                          { return _process_handle; }
        windows::Event*         process_event           ()                                          { return &_process_handle; }
        
        windows::Event         _process_handle;
        DWORD                  _exit_code;

#    else

        Process_handle          process_handle          ()                                          { return _pid; }
        
        int                    _exit_code;

#   endif

#   ifdef Z_HPUX
        void                    set_ld_preload          ( const string& value )                     { _ld_preload = value; }
        string                 _ld_preload;
#   endif
};

//------------------------------------------------------------------Connection_to_own_server_thread

struct Connection_to_own_server_thread : Connection
{
    struct Server_thread : Thread
    {
                                Server_thread           ( Connection_to_own_server_thread* );
                               ~Server_thread           ();

      //int                     thread_main             ();
        int                     run_server              ();
        Connection_to_own_server_thread* connection     () const                                    { return _connection; }

      protected:
        Fill_zero              _zero_;
        Connection_to_own_server_thread* _connection;
        ptr<Server>            _server;
    };

    struct Wait_for_thread_termination : Async_operation
    {
                                Wait_for_thread_termination( Connection_to_own_server_thread* );
        Z_GNU_ONLY(             Wait_for_thread_termination(); )                                

        virtual bool            async_continue_         ( Continue_flags );
        virtual bool            async_finished_         () const                                    { return _connection->_thread == NULL; }
        virtual bool            async_has_error_        () const                                    { return _connection->_last_errno != 0; }
        virtual string          async_state_text_       () const;

        Fill_zero              _zero_;
        Connection_to_own_server_thread* _connection;
    };



                                Connection_to_own_server_thread( Connection_manager* m )            : Connection( m ), _zero_(this+1) {}
                               ~Connection_to_own_server_thread();

    bool                        process_terminated      ()                                          { return _thread == NULL; }

    void                        start_thread            ( Server_thread* );
    bool                        call_waitpid            ( bool wait );

    virtual Async_operation*    close__start            ();
    virtual void                close__end              ();

    int                         thread_id               () const                                    { return _thread? _thread->thread_id() : 0; }
    virtual string              short_name              () const;
    string                      obj_name                () const;


    Fill_zero                  _zero_;
    ptr<Thread>                _thread;
};

//----------------------------------------------------------------------------------------Object_id

struct Object_id;

string string_from_object_id( Object_id id );


struct Object_id
{
                                Object_id               ( int64 value = 0 )                         : _value(value) {}
    int64                      _value;

    bool                        operator <              ( const Object_id& o ) const                { return _value <  o._value; }
    bool                        operator ==             ( const Object_id& o ) const                { return _value == o._value; }

    void                        obj_print               ( ostream* s ) const;
    friend ostream&             operator <<             ( ostream& s, const Object_id& o )          { s << string_from_object_id( o );  return s; }
    friend inline size_t        hash_value              ( Object_id o )                             { return (size_t)( o._value ^ ( o._value >> 32 ) ); }
};

//typedef int64                   Object_id;

//-------------------------------------------------------------------------------------Object_entry
/*
    REFERENZZÄHLUNG

    "Orginal" ist das echte Objekt.
    "Proxy" ist dessen Stellvertreter auf der anderen Seite.

    Wenn ein Zeiger auf ein Original zur anderen Seite übertragen wird:
        Beim Aufruf
            Hin
                Original noch nicht in _object_table eingetragen?
                    Original eintragen
                    Referenz des Originals hochzählen
                    Der anderen Seite mitteilen, dass der Zeiger neu ist (is_new)
                    ---
                    Die andere Seite richtet einen Proxy ein (is_new)
                    Referenz des Proxys hochzählen

                sonst
                    Referenz des Originals hochzählen?
                    Der anderen Seite mitteilen, dass der Zeiger bekannt ist
                    ---
                    Die andere Seite hat bereits einen Proxy
                    Referenz des Proxys hochzählen?

            Zurück
                Referenz des Proxys herunterzählen, bei 0 freigegeben
                Der Original-Seite mitteilen, wenn Proxy freigegeben ist (Original-Seite zählt dann seinen Referenzzähler runter)

        Beim Rücksprung
            Original noch nicht in _object_table eingetragen?
                Original eintragen
                Referenz des Originals hochzählen
                Der anderen Seite mitteilen, dass der Zeiger neu ist (is_new)
                ---
                Die andere Seite richtet einen Proxy ein (is_new)
                Referenz des Proxys hochzählen

            sonst
                Referenz des Originals hochzählen?
                Der anderen Seite mitteilen, dass der Zeiger bekannt ist
                ---
                Die andere Seite hat bereits einen Proxy
                Referenz des Proxys hochzählen?

    
    Eine Routine gibt den letzten Zeiger auf ein Objekt frei.
    Dieser Zeiger ist nicht mit dem aktuellen Aufruf übergeben worden, sondern schon vorher.
    Die andere Seite muss darüber benachrichtigt werden.
    b) :-) Sofort bei Release
    a) :-( Jeder Antwort wird also eine Liste der Objekte beigelegt, die die andere Seite nicht mehr kennt.


*/

struct Object_entry
{
                                Object_entry            ()                                          : _zero_(this+1) {}
                                Object_entry            ( const Object_entry& );
                               ~Object_entry            ();

    void                        close                   ();
    void                        set_debug_string        ();
    void                        obj_print               ( ostream* s ) const;
    friend ostream&             operator <<             ( ostream& s, const Object_entry& o )       { o.obj_print( &s );  return s; }

    Fill_zero                  _zero_;
    Object_id                  _id;
    IUnknown*                  _iunknown;               // AddRef() und Release() nur bei _is_owner
    bool                       _table_is_owner;         // Die Objekttabelle ist der (stellvertretende) Eigentümer des Objektes. Der richtige Eigentümer ist auf der anderen Seite
    bool                       _is_proxy;
    Z_DEBUG_ONLY( string       _debug_string; )

  private:
    void                        operator =              ( const Object_entry& );
};

//-------------------------------------------------------------------------------------Object_table

struct Object_table
{
                               ~Object_table            ()                                          { clear(); }

    void                        clear                   ();
    void                        remove                  ( Object_id );
  //ptr<IUnknown>               get_object              ( Session*, Object_id, bool is_new, bool become_owner, const string& title = "" );
    ptr<IUnknown>               get_object              ( Session*, Object_id );
    void                        add_proxy               ( Session*, Object_id, Proxy* proxy );
    Object_id                   get_object_id           ( IUnknown*, bool* is_new = NULL, bool become_owner = false );
    bool                        empty                   () const                                    { return _objects.empty(); }
    void                        obj_print               ( ostream* s ) const;
    friend ostream&             operator <<             ( ostream& s, const Object_table& o )       { o.obj_print( &s );  return s; }

    typedef std::map< Object_id, Object_entry >  Map;
    Map                        _objects;
    int                        _next_id;
};

//------------------------------------------------------------------------------------------Session

struct Session : Object
{
    typedef int64               Id;


                                Session                 ( Connection* con, Id id = 0 )                  : _zero_(this+1), _id(id), _connection(con) {}
                                Session                 ( Server* server, Connection* con, Id id = 0 )  : _zero_(this+1), _server(server), _id(id), _connection(con) {}
                               ~Session                 ();

    void                        close                   ();

    Async_operation*            close__start            ();
    void                        close__end              ()                                          { return; }

    void                        execute                 ( Input_message*, Output_message* );
    void                        server_loop             ();

    Connection*                 connection              ()                                          { return _connection; }

    void                    set_connection_has_only_this_session()                                  { _connection_has_only_this_session = true; }

    Async_operation*            connect_server__start   ()                                          { return  _connection->connect_server__start(); }
    void                        connect_server__end     ()                                          { _connection->connect_server__end(); }

  //HRESULT                     create_instance         ( const CLSID& clsid, IUnknown* outer, DWORD, COSERVERINFO*, uint count, MULTI_QI* query_interfaces );
    Simple_operation*           create_instance__start  ( const CLSID& clsid, IUnknown* outer, DWORD, COSERVERINFO*, uint count, MULTI_QI* query_interfaces );
    HRESULT                     create_instance__end    ( uint count, MULTI_QI* query_interfaces );


    Async_operation*            current_operation       ()                                          { return _connection? _connection->current_operation() : NULL; }

    Server*                     server                  () const                                    { return _server; }
    int                         pid                     () const                                    { return _connection? _connection->pid() : 0; }

  //Object*                     get_class_storage       ( const CLSID& clsid )                      { return get_container_element( _class_storage, clsid, NULL ); }
  //void                        set_class_storage       ( const CLSID& clsid, Object* o )           { _class_storage[ clsid ] = o; }


  private:
    friend struct Server;
    friend struct Proxy;
    friend struct Output_message;
    friend struct Input_message;
    friend struct Simple_operation;

    Z_GNU_ONLY(                 Session                 (); )

    void                        send_response           ( Output_message*, bool* connection_lost = NULL );
  //void                        execute_and_reply       ( Input_message* );
    HRESULT                     execute_create_instance ( const CLSID& clsid, IUnknown* outer, DWORD context, 
                                                          COSERVERINFO* coserverinfo, unsigned long count, MULTI_QI* query_interfaces );

    void                        execute_object          ( IUnknown*, Input_message*, Output_message* );
    void                        execute_object_queryinterface( IUnknown*, Input_message*, Output_message* );
    void                        execute_object_getidsofnames( IDispatch*, Input_message*, Output_message* );
    void                        execute_object_invoke   ( IDispatch*, Input_message*, Output_message* );
    void                        execute_object_call     ( IDispatch*, Input_message*, Output_message* );

    
    ptr<IUnknown>               object_from_id          ( Object_id );


    Fill_zero                  _zero_;
    Server*                    _server;
    Id                         _id;
    ptr<Connection>            _connection;
    Object_table               _object_table;
    bool                       _connection_has_only_this_session;

  //typedef stdext::hash_map<CLSID,ptr<Object>> Class_storage;
  //Class_storage              _class_storage;
};

//--------------------------------------------------------------------------------------------Proxy

Z_DEFINE_GUID( IID_Iproxy, 0xfeee4702, 0x6c1b, 0x11d8, 0x81, 0x03, 0x00, 0x04, 0x76, 0xee, 0x8a, 0xfb );      // {feee4702-6c1b-11d8-8103-000476ee8afb}

struct Proxy : idispatch_base_implementation< IDispatch >  //Object
{
                                Proxy                   ()                                          : _zero_(this+1) {}
                               ~Proxy                   ();

    void                    set_session                 ( Session* s )                              { _session = s;  _connection = s->connection(); }
    Session*                    session                 () const                                    { return _session; }
    void                    set_object_id               ( Object_id id )                            { _object_id = id; }
  //void                    set_table_is_owner          ( bool b )                                  { _table_is_owner = b; }
    void                    set_title                   ( const string& title )                     { _title = title; }

    virtual void                set_property            ( const string& name, const Variant& value );

    STDMETHODIMP                QueryInterface          ( const IID&, void** );
    Simple_operation*           QueryInterface__start   ( const IID& );
    STDMETHODIMP                QueryInterface__end     ( IUnknown** );

    STDMETHODIMP                GetIDsOfNames           ( REFIID, OLECHAR**, UINT names_count, LCID, DISPID* );
    Simple_operation*           GetIDsOfNames__start    ( REFIID, OLECHAR**, UINT names_count, LCID          );
    STDMETHODIMP                GetIDsOfNames__end      (                    UINT names_count,       DISPID* );

    STDMETHODIMP                Invoke                  ( DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT* );
    STDMETHODIMP                Invoke_from_any_thread  ( DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT* );
    Simple_operation*           Invoke__start           ( DISPID, REFIID, LCID, WORD, DISPPARAMS*                              , Connection::Different_thread_allowed = Connection::diffthr_not_allowed  );
    STDMETHODIMP                Invoke__end             (                                          VARIANT*, EXCEPINFO*, UINT* );

    string                      title                   ()                                          { return _title; }
    bool                        kill_process            ()                                          { return _session && _session->connection()? _session->connection()->kill_process() : false; }

    void                        release                 ();
    void                        release_finish          ();
    Async_operation*            release__start          ();
    void                        release__end            ();

    Variant                     call                    ( const string& method, 
                                                          const Variant& = missing_variant,
                                                          const Variant& = missing_variant,
                                                          const Variant& = missing_variant,
                                                          const Variant& = missing_variant,
                                                          const Variant& = missing_variant );

    Simple_operation*           call__start             ( const string& method, 
                                                          const Variant& = missing_variant,
                                                          const Variant& = missing_variant,
                                                          const Variant& = missing_variant,
                                                          const Variant& = missing_variant,
                                                          const Variant& = missing_variant );

    Variant                     call__end               ();

    int                         pid                     () const                                    { return _session? _session->pid() : 0; }

    void                        check_connection_error  ();

    const Bstr&                 name_from_dispid        ( DISPID );
    string                      obj_name                () const                                    { return "Proxy"; }


  private:
    friend struct               Session;


    Fill_zero                  _zero_;
    ptr<Session>               _session;
    ptr<Connection>            _connection;
  //bool                       _table_is_owner;
    bool                       _no_operation;
    Object_id                  _object_id;
    string                     _title;

    typedef std::map< com::Bstr, DISPID >  Dispid_map;
    Dispid_map                 _dispid_map;             // Cache für DISPIDs
};

//-----------------------------------------------------------------------proxy_with_local_methods<>

template< class CLASS, class INTERFACE >
struct proxy_with_local_methods : idispatch_implementation< CLASS, INTERFACE >,
                                  Proxy
{
    typedef proxy_with_local_methods                                        Proxy_with_local_methods;
    typedef typename proxy_with_local_methods::Idispatch_implementation     Idispatch_implementation;
    typedef typename proxy_with_local_methods::Class_descriptor             Class_descriptor;


    void*                       operator new                ( size_t size, Z_new_type, const char* name, int lineno )   { return z_malloc( size, name, lineno ); }
    void                        operator delete             ( void* p, Z_new_type, const char*, int )                   { z_free(p); }  // Für VC++ 6: Sonst warning C4291
    void                        operator delete             ( void* p )                                                 { z_free(p); }


    proxy_with_local_methods( typename proxy_with_local_methods::Class_descriptor* c )
    : 
        Idispatch_implementation( c )
    {}


    STDMETHODIMP_(ULONG) AddRef()
    { 
        return Idispatch_implementation::AddRef(); 
    }


    STDMETHODIMP_(ULONG) Release()
    { 
        return Idispatch_implementation::Release(); 
    }


    STDMETHODIMP GetIDsOfNames( const IID& iid, OLECHAR** names, uint names_count, LCID lcid, DISPID* result )
    {
        HRESULT hr = Idispatch_implementation::GetIDsOfNames( iid, names, names_count, lcid, result );
        
        if( hr == DISP_E_UNKNOWNNAME )
        {
            hr = Proxy::GetIDsOfNames( iid, names, names_count, lcid, result );
        }

        return hr;
    }


    STDMETHODIMP Invoke( DISPID dispid, const IID& iid, LCID lcid, WORD flags, DISPPARAMS* dispparams, VARIANT* result, EXCEPINFO* excepinfo, UINT* errarg )
    {
        HRESULT hr = Idispatch_implementation::Invoke( dispid, iid, lcid, flags, dispparams, result, excepinfo, errarg );
        
        if( hr == DISP_E_MEMBERNOTFOUND )
        {
            hr = Proxy::Invoke( dispid, iid, lcid, flags, dispparams, result, excepinfo, errarg );
        }

        return hr;
    }
};

//---------------------------------------------------------------------------------------Properties

typedef std::map< string, com::Variant >  Properties;

//------------------------------------------------------------------------Reference_with_properties

struct Reference_with_properties : Object
{
                                Reference_with_properties( const CLSID& proxy_clsid, IDispatch* idispatch )  : _idispatch(idispatch), _proxy_clsid(proxy_clsid) {}

    void                    set_proxy_clsid             ( const CLSID& clsid )                          { _proxy_clsid = clsid; }
    void                    set_property                ( const string& name, const Variant& value )    { _properties[name] = value; }


    IDispatch*                 _idispatch;
    CLSID                      _proxy_clsid;

    Properties                 _properties;
};

//-----------------------------------------------------------------------Ireference_with_properties

Z_DEFINE_GUID( IID_Ihas_reference_with_properties, 0xfeee4703, 0x6c1b, 0x11d8, 0x81, 0x03, 0x00, 0x04, 0x76, 0xee, 0x8a, 0xfb );    // {feee4703-6c1b-11d8-8103-000476ee8afb}

struct __declspec_uuid( "feee4703-6c1b-11d8-8103-000476ee8afb" ) Ihas_reference_with_properties : IUnknown
{
    friend inline const GUID& __uuidof_                 ( Ihas_reference_with_properties* )         { return IID_Ihas_reference_with_properties; }

    //virtual STDMETHODIMP        get_reference_with_properties   ( const IID& iid, ptr<z::com::object_server::Reference_with_properties>* ) = 0;
    virtual ptr<object_server::Reference_with_properties> get_reference_with_properties() = 0;
};

//-----------------------------------------------------------------------------------Output_message

struct Output_message
{
                                Output_message          ( Session* s )                              : _zero_(this+1), _session(s) { _data.reserve( initial_message_size ); _data.resize( 4 ); }

    void                        send                    ();
    bool                        send_async              ();

    void                        clear                   ()                                          { _data.erase(); _data.resize( 4 ); 
                                                                                                      _data_bytes_written = 0;
                                                                                                      _is_answer = false; 
                                                                                                      _is_sent = false; 
                                                                                                    }
    void                        reserve                 ( int size )                                { _data.reserve( size ); }
    void                        write_char              ( char c )                                  { _data += c; }
    void                        write_bytes             ( const void* p, int len )                  { _data.append( (const char*)p, len ); }
    void                        write_int16             ( int16 );
    void                        write_int32             ( int32 );
    void                        write_uint32            ( uint32 u )                                { write_int32( (int32)u ); }
    void                        write_int64             ( int64 );
    void                        write_string            ( const string& );
    void                        write_string            ( const OLECHAR* str )                      { write_string( str, wcslen( str ) ); }
    void                        write_string            ( const OLECHAR*, int length );
    void                        write_double            ( double );
    void                        write_bstr              ( const BSTR bstr )                         { write_string( bstr, SysStringLen( bstr ) ); }
  //void                        write_safearray         ( const VARIANT& );
    void                        write_safearray         ( const SAFEARRAY* );
    void                        write_guid              ( const GUID& );
    void                        write_iunknown          ( IUnknown* );
    void                        write_variant           ( const VARIANT& );
    void                        write_dispparams        ( const DISPPARAMS& );
    void                        write_excepinfo         ( const EXCEPINFO& );
    void                        finish                  ();
    bool                        is_sent                 ()                                          { return _is_sent; }

    int                         pid                     () const                                    { return _session? _session->pid() : 0; }


    Fill_zero                  _zero_;
    Session*                   _session;
    string                     _data;    
    int                        _data_bytes_written;
    bool                       _is_answer;
    bool                       _is_sent;
};

//---------------------------------------------------------------------------------Simple_operation

struct Simple_operation : Async_operation
{
    enum State
    {
        s_none,
        s_starting,
        s_sending,
        s_waiting,
        s_receiving,
        s_received,
        s_finished
    };

                                Simple_operation        ( Session*, IDispatch*, const string& method, const string& debug_text = "" );
    Z_GNU_ONLY(                 Simple_operation        (); )                                
                               ~Simple_operation        ();

    void                        close                   ();
    void                        start                   ( Connection::Different_thread_allowed = Connection::diffthr_not_allowed );

    virtual bool                async_continue_         ( Continue_flags );
    virtual bool                async_finished_         () const                                    { return _state == s_finished; }
    virtual void                async_finish_           ();
    virtual bool                async_has_error_        () const                                    { return _state == s_finished  &&  _input_message.length() >= 1  &&  _input_message[0] == msg_error; }
    virtual void                async_check_error_      ();
    virtual string              async_state_text_       () const;
    virtual bool                async_kill_             ();
    int                         async_pid               () const                                    { return _session? _session->pid() : 0; }
    int                         pid                     () const                                    { return _session? _session->pid() : 0; }

    string                      state_name              () const;
    bool                        is_callback_message     ()                                          { return _input_message.peek_char() == msg_object; }

  //bool                        receive_async           ();
    void                        send                    ();
    bool                        send_async              ();



    Fill_zero _zero_;
    
    State                      _state;
    const string               _method;
    const IDispatch*           _object;
    ptr<Session>               _session;
    Output_message             _output_message;
    Input_message              _input_message;
    string                     _debug_text;             // Für Debug-Meldungen
    bool                       _on_stack;               //                                          Connection::pop_operation() rufen!
    int                        _callback_nesting;       // Von Connection::_callback_nesting
  //Mutex_guard                _connection_mutex_guard;
    bool                       _different_thread_is_allowed;    // Nur vorsichtshalber, Thread-Fähigkeit müsste getestet werden

    Fill_end _end_;
};

//-------------------------------------------------------------------------------Connection_manager

struct Connection_manager : Socket_manager
{
                                Connection_manager      ();
                               ~Connection_manager      ();

    ptr<Connection>                       new_connection();
    ptr<Connection_to_own_server_process> new_connection_to_own_server_process();
    ptr<Connection_to_own_server_thread>  new_connection_to_own_server_thread();

    void                        add_connection          ( Connection* c )                           { _connection_list.push_back( c ); }
    void                        remove_connection       ( Connection* );

    typedef std::list< Connection* >  Connection_list;
    Connection_list            _connection_list;
    Buffer                     _common_buffer;
};

//-------------------------------------------------------------------------Create_instance_function

typedef HRESULT                 Create_instance_function( Session*, ptr<Object>*, const IID&, ptr<IUnknown>* result);

//-------------------------------------------------------------------------------------------Server

struct Server : Object
{
    struct Class_entry
    {
                                     Class_entry        ( Create_instance_function* f = NULL )      : _function(f) {}

        Create_instance_function*   _function;
        ptr<Object>                 _class_object;
    };


                                Server                  ();


  //int                         main                    ( int, char** );
    int                         main                    ( int, char** );
    void                        simple_server           ( const Host_and_port& controller );
    void                        server                  ( int server_port );
    
    void                        register_class          ( const CLSID&, Create_instance_function* );
    Create_instance_function*   get_class_factory       ( const CLSID& );

  //void                        execute                 ( Input_message*, Output_message* );

    Object*                     get_class_object_or_null( const CLSID& );
    void                        set_class_object        ( const CLSID&, Object* );
    
    void                    set_stdin_data              ( const string& data )                      { _stdin_data = data; _has_stdin_data = true; }
    string                      stdin_data              ();


    Fill_zero                  _zero_;
    typedef std::map< CLSID, Class_entry >  Class_register;
    Class_register             _class_register;

    ptr<Connection>            _connection;
    string                     _stdin_data;
    bool                       _has_stdin_data;
};

//----------------------------------------------------------com_create_instance_in_separate_process

//HRESULT                         com_create_instance_in_separate_process( const CLSID& clsid, IUnknown* outer, DWORD context, const IID& iid, void** result, 
//                                                                         int* pid, const Parameters& = Parameters() );

//-------------------------------------------------------------------------------------------------

} //namespace object_server
} //namespace com
} //namespace zschimmer


#endif
