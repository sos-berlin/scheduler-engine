#include "spooler.h"

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------------------------const
    
const int connection_retry_time = 60;

Process_class_subsystem    ::Class_descriptor    Process_class_subsystem    ::class_descriptor ( &typelib, "Spooler.Process_classes", Process_class_subsystem::_methods );
Process_class_configuration::Class_descriptor    Process_class_configuration::class_descriptor ( &typelib, "Spooler.Process_class"  , Process_class          ::_methods );

//---------------------------------------------------------------------------------Abstract_process
// Ein Prozess, in dem ein Module oder eine Task ablaufen kann.
// Kann auch ein Thread sein.

struct Abstract_process : virtual Process {
    protected: Abstract_process(Spooler* spooler, const Process_configuration& c) :
        _spooler(spooler),
        _configuration(c),
        _process_id(spooler->get_next_process_id()),
        _log(Z_NEW(Prefix_log(this)))
    {}

    public: virtual ~Abstract_process() {}

    public: Type_code scheduler_type_code() const { 
        return Scheduler_object::type_process; 
    }

    public: Spooler* spooler() const { 
        return _spooler; 
    }

    public: Prefix_log* log() const { 
        return _log; 
    }

    public: xml::Element_ptr dom_element(const xml::Document_ptr& document, const Show_what&) {
        xml::Element_ptr process_element = document.createElement( "process" );
        process_element.setAttribute_optional("job", _configuration._job_name);
        if (_configuration._task_id) {
            process_element.setAttribute("task", _configuration._task_id);
            process_element.setAttribute("task_id", _configuration._task_id);          // VERALTET!
        }
        process_element.setAttribute_optional("remote_scheduler", _configuration._remote_scheduler.as_string());
        return process_element;
    }

    public: Spooler* const _spooler;
    public: Process_configuration const _configuration;
    public: Process_id const _process_id;
    public: ptr<Prefix_log> const _log;
};


struct Async_remote_operation;

struct Abstract_startable_process : Abstract_process {
    struct Close_operation : Async_operation {
        enum State { s_initial, s_closing_session, s_closing_remote_process, s_finished };

                                    Close_operation         (Abstract_startable_process*, bool run_independently);
                                   ~Close_operation         ();

        // Async_operation:
        bool                        async_continue_         ( Continue_flags );
        bool                        async_finished_         () const;
        string                      async_state_text_       () const;

        static string               string_from_state       ( State );

      private:
        friend struct               Abstract_startable_process;

        State                      _state;
        ptr<Abstract_startable_process> _process;
        Async_operation*           _close_session_operation;
        ptr<Close_operation>       _hold_self;              // Objekt hält sich selbst, wenn es selbstständig, ohne Antwort, den Abstract_process schließen soll
    };

    Abstract_startable_process(Spooler* spooler, const Process_configuration& conf) :
        Abstract_process(spooler, conf),
        _process_handle_copy(NULL),
        _is_killed(false),
        _exit_code(0),
        _termination_signal(0),
        _remote_process_id(0),
        _remote_pid(0)
    {}

    ~Abstract_startable_process();

    void                        close_async                 ();
    Async_operation*            close__start                ( bool run_independently = false );
    void                        close__end                  ();
    bool                     is_closing                     ()                                      { return _close_operation != NULL; }
    bool                        continue_close_operation    (Abstract_startable_process::Close_operation*);


    bool                        is_started                  ()                                      { return _connection != NULL; }

    void                        fill_connection             ( object_server::Connection* );
    object_server::Session*     session                     ()                                      { return _session; }
    bool                        async_continue              ();
    double                      async_next_gmtime           ()                                      { return _connection? _connection->async_next_gmtime() : time::never_double; }
    void                        close_session      ();
    Process_id                  process_id                  () const                                { return _process_id; }
    int                         pid                         () const;                               // Bei kind_process die PID des eigentlichen Prozesses, über Connection_to_own_server_thread
    bool                     is_terminated                  ();
    bool                        kill                        ();
    int                         exit_code                   ();
    int                         termination_signal          ();
    File_path                   stderr_path                 ();
    File_path                   stdout_path                 ();
    bool                        try_delete_files            ( Has_log* );
    std::list<file::File_path>  undeleted_files             ();
    bool                        is_remote_host              () const;
    object_server::Connection*  connection                  () const                                { return _connection; }

    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    string                      obj_name                    () const;
    string                      short_name                  () const;

    public: void start() {
        if (is_started()) assert(0), z::throw_xc(Z_FUNCTION);
        do_start();
        #ifdef Z_WINDOWS
            if (_spooler)
                _connection->set_event(&_spooler->_scheduler_event);
        #endif
        _log->set_prefix(obj_name());
        _session = Z_NEW(object_server::Session(_connection));
        _session->set_connection_has_only_this_session();
        _running_since = Time::now();
    }
    
    protected: virtual void do_start() = 0;

  public:
    ptr<object_server::Connection> _connection;             // Verbindung zum Prozess
    ptr<object_server::Session>    _session;                // Wir haben immer nur eine Session pro Verbindung
    Process_handle             _process_handle_copy;
    bool                       _is_killed;
    int                        _exit_code;
    int                        _termination_signal;
    Time                       _running_since;
    Process_id                 _remote_process_id;
    pid_t                      _remote_pid;
    ptr<Xml_client_connection>  _xml_client_connection;
    ptr<Close_operation>       _close_operation;
    ptr<Async_remote_operation> _async_remote_operation;    // Nur für Standard_remote_process
};


struct Dummy_process : Abstract_startable_process {
    public: Dummy_process(Spooler* spooler, const Process_configuration& conf) :
        Abstract_startable_process(spooler, conf) 
    {}

    protected: void do_start() {
        z::throw_xc(Z_FUNCTION);
    }

    public: xml::Element_ptr dom_element(const xml::Document_ptr& document, const Show_what& show_what) {
        return Abstract_process::dom_element(document, show_what);
    }
};


struct Local_process : Abstract_startable_process {
    Local_process(Spooler* spooler, const Process_configuration& conf) :
        Abstract_startable_process(spooler, conf)
    {}

    protected: void do_start() {
        ptr<object_server::Connection_to_own_server_process> connection = new_connection();
        _connection = +connection;
        connection->start_process(parameters());
        _process_handle_copy = _connection->process_handle();
        _spooler->register_process_handle(_process_handle_copy);
        _spooler->log()->debug9(message_string("SCHEDULER-948", _connection->short_name()));  // pid wird auch von Task::set_state(s_starting) mit log_info protokolliert
    }

    private: ptr<object_server::Connection_to_own_server_process> new_connection() {
        ptr<object_server::Connection_to_own_server_process> connection = _spooler->_connection_manager->new_connection_to_own_server_process();
        connection->open_stdout_stderr_files();      //Nicht in einem remote_scheduler (File_logger übernimmt stdout): 
        fill_connection(connection);
        #ifdef Z_HPUX
            connection->set_ld_preload( static_ld_preload );
        #endif
        connection->set_priority(_configuration._priority);
        connection->set_login(_configuration._login);
        return connection;
    }

    private: object_server::Parameters parameters() {
        object_server::Parameters parameters;
        if (!_spooler->_sos_ini.empty())
            parameters.push_back(object_server::Parameter("param", "-sos.ini=" + _spooler->_sos_ini));   // Muss der erste Parameter sein! (für sos_main0()).
        if (!_spooler->_factory_ini.empty())
            parameters.push_back(object_server::Parameter("param", "-ini=" + _spooler->_factory_ini));
        parameters.push_back(object_server::Parameter("param", "-java-options=" + _configuration._java_options + " " + spooler()->settings()->_job_java_options));
        parameters.push_back(object_server::Parameter("param", "-java-classpath=" + _configuration._java_classpath + Z_PATH_SEPARATOR + spooler()->settings()->_job_java_classpath));
        parameters.push_back(object_server::Parameter("param", "-O"));
        if (!_configuration._job_name.empty())
            parameters.push_back(object_server::Parameter("param", "-job=" + _configuration._job_name));
        if (_configuration._task_id)
            parameters.push_back(object_server::Parameter("param", "-task-id=" + as_string(_configuration._task_id)));
        if (!log_filename().empty())
             parameters.push_back(object_server::Parameter("param", "-log=" + log_categories_as_string() + " >+" + log_filename()));
        parameters.push_back(object_server::Parameter("program", _spooler->_my_program_filename));
        return parameters;
    }
};


struct Thread_process : Abstract_startable_process {
    struct Com_server_thread : object_server::Connection_to_own_server_thread::Server_thread
    {
        typedef object_server::Connection_to_own_server_thread::Server_thread Base_class;

        Com_server_thread(object_server::Connection_to_own_server_thread*);
        int thread_main();

        ptr<Object_server> _object_server;
    };

    Thread_process(Spooler* spooler, const Process_configuration& conf) :
        Abstract_startable_process(spooler, conf) 
    {}

    protected: void do_start() {
        Z_LOGI2("Z-REMOTE-118", Z_FUNCTION << "\n");
        ptr<object_server::Connection_to_own_server_thread> connection = _spooler->_connection_manager->new_connection_to_own_server_thread();
        fill_connection(connection);
        _com_server_thread = Z_NEW(Com_server_thread(connection));
        connection->start_thread(_com_server_thread);
        _connection = +connection;
        _spooler->log()->debug9(message_string("SCHEDULER-948", _connection->short_name()));  // pid wird auch von Task::set_state(s_starting) mit log_info protokolliert
        Z_LOG2("Z-REMOTE-118", Z_FUNCTION << " okay\n");
    }
    
    protected: int pid() const;

    private: ptr<Com_server_thread> _com_server_thread;
};

struct Standard_remote_process;

struct Async_remote_operation : Async_operation {
    enum State {
        s_not_connected,
        s_connecting,
        s_starting,
        s_running,
        s_closing,
        s_closed
    };

    static string state_name(State);

    Async_remote_operation(Standard_remote_process*);
    ~Async_remote_operation();

    virtual bool async_continue_(Continue_flags f);
    
    virtual bool async_finished_() const { 
        return _state == s_running || _state == s_closed; 
    }

    virtual string async_state_text_() const;
    void close_remote_task(bool kill = false);


    State _state;
    Standard_remote_process* _process;
};


struct Standard_remote_process : Abstract_startable_process {
    Standard_remote_process(Spooler* spooler, const Process_configuration& conf) :
        Abstract_startable_process(spooler, conf) 
    {}

    ~Standard_remote_process() {
        if (_async_remote_operation) {
            _async_remote_operation->set_async_manager( NULL );
            _async_remote_operation = NULL;
        }
    }

    protected: void do_start() {
        async_remote_start();
    }

    public: bool kill() {
        if (!_is_killed && _connection) {
            // Async_operation (vorhandene oder neue, besser neue)
            // mit <kill_task> starten. Nur, wenn noch nicht gestartet
            // if( !_kill_task_operation )   _kill_task_operation = Z_NEW( Kill_task_operation );
            // <remote_scheduler.task.close pid=" _remote_pid
            if (_async_remote_operation && _async_remote_operation->_state != Async_remote_operation::s_closed)
                _async_remote_operation->close_remote_task(true);
            _is_killed = true;
            return true;
        } else
            return false;
    }


    private: void async_remote_start();
    public: bool async_remote_start_continue(Async_operation::Continue_flags);
};

//----------------------------------------------------------------Process_class_subsystem::_methods

const Com_method Process_class_subsystem::_methods[] =
{ 
#ifdef COM_METHOD
    COM_PROPERTY_GET( Process_class_subsystem,  1, Java_class_name      , VT_BSTR    , 0 ),
    COM_PROPERTY_GET( Process_class_subsystem,  0, Process_class        , VT_DISPATCH, 0, VT_BSTR ),
    COM_PROPERTY_GET( Process_class_subsystem,  2, Process_class_or_null, VT_DISPATCH, 0, VT_BSTR ),
    COM_METHOD      ( Process_class_subsystem,  3, Create_process_class , VT_DISPATCH, 0 ),
    COM_METHOD      ( Process_class_subsystem,  4, Add_process_class    , VT_EMPTY   , 0, VT_DISPATCH ),
#endif
    {}
};

//------------------------------------------------------------Process_class_configuration::_methods

const Com_method Process_class_configuration::_methods[] =
{ 
#ifdef COM_METHOD
    COM_PROPERTY_GET( Process_class_configuration,  1, Java_class_name     , VT_BSTR    , 0 ),
    COM_PROPERTY_PUT( Process_class_configuration,  2, Name                ,              0, VT_BSTR ),
    COM_PROPERTY_GET( Process_class_configuration,  2, Name                , VT_BSTR    , 0 ),
    COM_PROPERTY_PUT( Process_class_configuration,  3, Remote_scheduler    ,              0, VT_BSTR ),
    COM_PROPERTY_GET( Process_class_configuration,  3, Remote_scheduler    , VT_BSTR    , 0 ),
    COM_PROPERTY_PUT( Process_class_configuration,  4, Max_processes       ,              0, VT_INT ),
    COM_PROPERTY_GET( Process_class_configuration,  4, Max_processes       , VT_INT    , 0 ),
    COM_METHOD      ( Process_class_configuration,  5, Remove              , VT_EMPTY   , 0 ),
#endif
    {}
};

//-------------------------------------Abstract_startable_process::Close_operation::Close_operation

Abstract_startable_process::Close_operation::Close_operation(Abstract_startable_process* p, bool run_independently)
: 
    _state(s_initial),
    _close_session_operation(NULL),
    _process(p)
{
    if( run_independently )
    {
        _hold_self = this;
    }
}

//------------------------------------Abstract_startable_process::Close_operation::~Close_operation
    
Abstract_startable_process::Close_operation::~Close_operation()
{
}

//-------------------------------------Abstract_startable_process::Close_operation::async_continue_

bool Abstract_startable_process::Close_operation::async_continue_(Async_operation::Continue_flags)
{
    return _process->continue_close_operation( this );
}

//---------------------------------------------Abstract_startable_process::continue_close_operation

bool Abstract_startable_process::continue_close_operation(Abstract_startable_process::Close_operation* op)
{
    bool something_done = false;

    if( op->_state == Close_operation::s_initial )
    {
        if( _session )
        {
            op->_close_session_operation = _session->close__start();
    
            if( !op->_close_session_operation->async_finished() )
            {
                op->_close_session_operation->set_async_parent( op );
                op->_close_session_operation->set_async_manager( _spooler->_connection_manager );
            }

            something_done = true;
        }

        op->_state = Close_operation::s_closing_session;
    }

    if( op->_state == Close_operation::s_closing_session )
    {
#       ifdef Z_WINDOWS
            if( op->_close_session_operation )  op->_close_session_operation->async_continue();       // Falls wir wegen Prozess-Event aufgerufen worden sind
#       endif

        if( !op->_close_session_operation  ||  op->_close_session_operation->async_finished() )
        {
            if( op->_close_session_operation )
            {
                op->_close_session_operation->set_async_parent( NULL );
                op->_close_session_operation = NULL;
                _session->close__end();
                _session = NULL;

                something_done = true;
            }

            if( _async_remote_operation )
            {
                _async_remote_operation->close_remote_task();
                _async_remote_operation->set_async_parent( op );
                something_done = true;
            }

            op->_state = Close_operation::s_closing_remote_process;
        }
    }

    if( op->_state == Close_operation::s_closing_remote_process )
    {
        if( !_async_remote_operation || _async_remote_operation->async_finished() )
        {
            if( _async_remote_operation )  _async_remote_operation->set_async_parent( NULL );

            op->_state = Close_operation::s_finished;

            ptr<Abstract_startable_process> process = op->_process;
            op->_process = NULL;

            if( op->_hold_self )    // run_independently
            {
                process->close__end();
                process->_close_operation = NULL;
                op->_hold_self = NULL;
                // this ist jetzt ungültig!
            }

            something_done = true;
        }
    }

    return something_done;
}

//-------------------------------------Abstract_startable_process::Close_operation::async_finished_

bool Abstract_startable_process::Close_operation::async_finished_() const
{
    return _state == s_finished;
}

//-----------------------------------Abstract_startable_process::Close_operation::async_state_text_

string Abstract_startable_process::Close_operation::async_state_text_() const
{
    S result;

    result << "Abstract_startable_process::Close_operation ";
    result << string_from_state( _state );
    if( _close_session_operation )  result << " " << _close_session_operation->async_state_text();
    if( _process  &&  _process->_async_remote_operation )  result << " " << _process->_async_remote_operation->async_state_text();

    return result;
}

//-----------------------------------Abstract_startable_process::Close_operation::string_from_state
    
string Abstract_startable_process::Close_operation::string_from_state( State state )
{
    switch( state )
    {
        case s_initial:                 return "initial";
        case s_closing_session:         return "closing_session";
        case s_closing_remote_process:  return "closing_remote_process";
        case s_finished:                return "finished";
        default:                        return S() << "State(" << state << ")";
    }
}

//---------------------------------------------------Async_remote_operation::Async_remote_operation
    
Async_remote_operation::Async_remote_operation(Standard_remote_process* p)
:                        
    _state(s_not_connected), 
    _process(p) 
{
    _process->_spooler->_process_count++;       // Jeder Prozess hat zwei Verbindungen: Zum Prozess und Xml_client_connection zum Scheduler
}

//--------------------------------------------------Async_remote_operation::~Async_remote_operation
    
Async_remote_operation::~Async_remote_operation()
{
    --_process->_spooler->_process_count;

    if( _process && _process->_xml_client_connection && _process->_xml_client_connection->async_parent() == this )  
    {
        _process->_xml_client_connection->set_async_parent( NULL );
    }
}

//-----------------------------------Standard_remote_process::Async_remote_operation::state_name
    
string Async_remote_operation::state_name( State state )
{
    string result;

    switch( state )
    {
        case s_not_connected:   result = "not_connected";   break;
        case s_connecting:      result = "connecting";      break;
        case s_starting:        result = "starting";        break;
        case s_running:         result = "running";         break;
        case s_closing:         result = "closing";         break;
        case s_closed:          result = "closed";          break;
        default:                result = "Async_remote_operation-" + as_string( state );
    }

    return result;
}


bool Async_remote_operation::async_continue_(Continue_flags f) { 
    return _process->async_remote_start_continue(f); 
}

//--------------------------------------------------------Async_remote_operation::async_state_text_

string Async_remote_operation::async_state_text_() const
{
    S result;
    result << "Async_remote_operation " << state_name( _state );
    if( _process )  result << " " << _process->obj_name();
    return result;
}

//---------------------------------------------Thread_process::Com_server_thread::Com_server_thread

Thread_process::Com_server_thread::Com_server_thread( object_server::Connection_to_own_server_thread* c ) 
: 
    Base_class(c)
{
    set_thread_name( "scheduler::Thread_process::Com_server_thread" );
}

//---------------------------------------------------Thread_process::Com_server_thread::thread_main
    
int Thread_process::Com_server_thread::thread_main()
{
    Z_LOGI2("Z-REMOTE-118", Z_FUNCTION << "\n");
    int result = 0;

    Com_initialize com_initialize;

    try
    {
        _object_server = Z_NEW( Object_server() );  // Bis zum Ende des Threads stehenlassen, wird von anderem Thread benutzt: Abstract_process::pid()
        _object_server->set_stdin_data( _connection->stdin_data() );
        _server = +_object_server;

        result = run_server();
    }
    catch( exception& x )
    {
        S s; 
        s << "ERROR in Com_server_thread: " << x.what() << "\n";
        Z_LOG2( "scheduler", s );
        cerr << s;
    }

    Z_LOG2("Z-REMOTE-118", Z_FUNCTION << " okay\n");
    return result;
}


ptr<Process> Process::new_process(Spooler* spooler, const Process_configuration& configuration) {
    if(configuration._remote_scheduler) {
        ptr<Standard_remote_process> result = Z_NEW(Standard_remote_process(spooler, configuration));
        return +result;
    } else
    if (configuration._is_thread) {
        ptr<Thread_process> result = Z_NEW(Thread_process(spooler, configuration));
        return +result;
    } else {
        ptr<Local_process> result = Z_NEW(Local_process(spooler, configuration));
        return +result;
    }
}

//------------------------------------------Abstract_startable_process::~Abstract_startable_process

Abstract_startable_process::~Abstract_startable_process()
{
    if (_close_operation) {
        _close_operation->set_async_manager(NULL);
        _close_operation = NULL;
    }

    if( _process_handle_copy )  _spooler->unregister_process_handle( _process_handle_copy );
    if( _xml_client_connection )  _xml_client_connection->set_async_manager( NULL );
}

//----------------------------------------------------------Abstract_startable_process::close_async

void Abstract_startable_process::close_async()
{
    if( !_close_operation )
    {
        if( !_async_remote_operation  &&  !is_terminated() )
        {
            log()->warn( message_string( "SCHEDULER-850", obj_name()) );

            try
            {
                kill();     // Nicht bei _async_remote_operation aufrufen, dass eine TCP-Nachricht starten würde
            }
            catch( exception& x ) { _log->warn( x.what() ); }
        }


        close__start( true );
    }
}

//---------------------------------------------------------Abstract_startable_process::close__start

Async_operation* Abstract_startable_process::close__start( bool run_independently )
{
    assert(!_close_operation);
    _close_operation = Z_NEW( Close_operation( this, run_independently ) );
    _close_operation->set_async_next_gmtime((time_t)0);
    _close_operation->set_async_manager( _spooler->_connection_manager );
    //_close_operation->async_continue();

    return _close_operation;
}

//-----------------------------------------------------------Abstract_startable_process::close__end

void Abstract_startable_process::close__end()
{
    if (_close_operation) _close_operation->set_async_manager(NULL);
    _close_operation = NULL;
    _session = NULL;
}

//---------------------------------------------------------Abstract_startable_process::close_session

void Abstract_startable_process::close_session()
{ 
    if( _session )
    {
        _session->close__start() -> async_finish();
        _session->close__end();
        _session = NULL;
    }
}

//------------------------------------------------------Abstract_startable_process::fill_connection

void Abstract_startable_process::fill_connection( object_server::Connection* connection )
{
    xml::Xml_string_writer stdin_xml_writer;

    stdin_xml_writer.set_encoding( string_encoding );
    stdin_xml_writer.write_prolog();

    stdin_xml_writer.begin_element( "task_process" );
    {
        stdin_xml_writer.set_attribute_optional( "include_path"          , _spooler->include_path() );
        stdin_xml_writer.set_attribute_optional( "java_options"          , _spooler->java_subsystem()->java_vm()->options() +" "+
            _spooler->settings()->_job_java_options);
        stdin_xml_writer.set_attribute_optional( "java_class_path"       , spooler()->settings()->_job_java_classpath + Z_PATH_SEPARATOR +
            _spooler->java_subsystem()->java_vm()->class_path());
        stdin_xml_writer.set_attribute_optional( "javac"                 , _spooler->java_subsystem()->java_vm()->javac_filename() );
        stdin_xml_writer.set_attribute_optional( "java_work_dir"         , _spooler->java_work_dir() );
        stdin_xml_writer.set_attribute_optional( "scheduler.directory"   , _spooler->directory() );      // Für Com_spooler_proxy::get_Directory
        stdin_xml_writer.set_attribute_optional( "scheduler.log_dir"     , _spooler->_log_directory );   // Für Com_spooler_proxy::get_Log_dir
        stdin_xml_writer.set_attribute_optional( "scheduler.include_path", _spooler->_include_path );    // Für Com_spooler_proxy::get_Include_path
        stdin_xml_writer.set_attribute_optional( "scheduler.ini_path"    , _spooler->_factory_ini );     // Für Com_spooler_proxy::get_Ini_path

        if( object_server::Connection_to_own_server_process* c = dynamic_cast<object_server::Connection_to_own_server_process*>( connection ) )
        {
            stdin_xml_writer.set_attribute( "stdout_path", c->stdout_path() );
            stdin_xml_writer.set_attribute( "stderr_path", c->stderr_path() );
        }
        
        if (_configuration._log_stdout_and_stderr)
            stdin_xml_writer.set_attribute("log_stdout_and_stderr", "yes");

        if (_configuration._environment) {
            xml::Document_ptr dom_document = _configuration._environment->dom("environment", "variable");
            stdin_xml_writer.write_element( dom_document.documentElement() );
        }
    }

    stdin_xml_writer.end_element( "task_process" );
    stdin_xml_writer.flush();

    connection->set_stdin_data( stdin_xml_writer.to_string() );

    if (_configuration._controller_address) 
        connection->set_controller_address(_configuration._controller_address);
}

//------------------------------------------------------Standard_remote_process::async_remote_start

void Standard_remote_process::async_remote_start()
{
    ptr<object_server::Connection> c = _spooler->_connection_manager->new_connection();

    c->set_remote_host(_configuration._remote_scheduler._host);
    c->listen_on_tcp_port( INADDR_ANY );


    _connection = +c;

    _async_remote_operation = Z_NEW( Async_remote_operation( this ) );
    _async_remote_operation->async_wake();
    _async_remote_operation->set_async_manager( _spooler->_connection_manager );
}

//---------------------------------------------Standard_remote_process::async_remote_start_continue

bool Standard_remote_process::async_remote_start_continue( Async_operation::Continue_flags )
{
    bool something_done = true;     // spooler.cxx ruft async_continue() auf

    if( _xml_client_connection )  _xml_client_connection->async_check_exception();

    switch( _async_remote_operation->_state )
    {
        case Async_remote_operation::s_not_connected:
        {
            _xml_client_connection = Z_NEW( Xml_client_connection( _spooler, _configuration._remote_scheduler ) );
            _xml_client_connection->set_async_parent( _async_remote_operation );
            _xml_client_connection->set_async_manager( _spooler->_connection_manager );
            _xml_client_connection->set_wait_for_connection( connection_retry_time );
            _xml_client_connection->connect();

            something_done = true;
            _async_remote_operation->_state = Async_remote_operation::s_connecting;
        }

        case Async_remote_operation::s_connecting:
        {
            if( _xml_client_connection->state() != Xml_client_connection::s_connected )  break;

            xml::Xml_string_writer xml_writer;
            xml_writer.set_encoding( string_encoding );
            xml_writer.write_prolog();
            xml_writer.begin_element( "remote_scheduler.start_remote_task" );
            xml_writer.set_attribute( "tcp_port", _connection->tcp_port() );
            if (!_configuration._has_api)  xml_writer.set_attribute( "kind", "process" );
            if (!rtrim(_configuration._java_options).empty())
                xml_writer.set_attribute_optional("java_options", _configuration._java_options);
            if (!rtrim(_configuration._java_classpath).empty())
                xml_writer.set_attribute_optional("java_classpath", _configuration._java_classpath);
            xml_writer.end_element( "remote_scheduler.start_remote_task" );
            xml_writer.close();
            _xml_client_connection->send( xml_writer.to_string() );

            something_done = true;
            _async_remote_operation->_state = Async_remote_operation::s_starting;
        }

        case Async_remote_operation::s_starting:
        {
            xml::Document_ptr dom_document = _xml_client_connection->fetch_received_dom_document();
            if( !dom_document )  break;
                                                              
            Z_LOG2( "scheduler", Z_FUNCTION << " XML response: " << dom_document.xml_string() );

            //if( _spooler->_validate_xml )  _spooler->_schema.validate( dom_document );

            _remote_process_id = dom_document.select_element_strict( "spooler/answer/process" ).int_getAttribute( "process_id", 0 );
            assert(_remote_process_id);
            _remote_pid        = dom_document.select_element_strict( "spooler/answer/process" ).int_getAttribute( "pid", 0 );

            _spooler->log()->debug9( message_string( "SCHEDULER-948", _connection->short_name() ) );  // pid wird auch von Task::set_state(s_starting) mit log_info protokolliert
            something_done = true;
            _async_remote_operation->_state = Async_remote_operation::s_running;
        }

        case Async_remote_operation::s_running:    
        {
            if (_configuration._has_on_remote_task_running)
                _configuration._has_on_remote_task_running->on_remote_task_running();
            break;
        }

        case Async_remote_operation::s_closing:
        {
            if( xml::Document_ptr dom_document = _xml_client_connection->fetch_received_dom_document() )  
            {
                Z_LOG2( "zschimmer", Z_FUNCTION << " XML response " << dom_document.xml_string() );
                //_spooler->log()->debug9( message_string( "SCHEDULER-948", _connection->short_name() ) );  // pid wird auch von Task::set_state(s_starting) mit log_info protokolliert
                something_done = true;
                _async_remote_operation->_state = Async_remote_operation::s_closed;
            }

            break;
        }

        default:
            break;
    }

    return something_done;
}

//-------------------------------------------------------Abstract_startable_process::async_continue

bool Abstract_startable_process::async_continue()
{
    return _connection? _connection->async_continue() 
                      : false;
}

//--------------------------------------------------------Async_remote_operation::close_remote_task

void Async_remote_operation::close_remote_task( bool kill )
{
    if( _state <= s_connecting ) {
        if( _process->_xml_client_connection ) {
            _process->_xml_client_connection->close();
            _process->_xml_client_connection->set_async_parent( NULL );
            _process->_xml_client_connection = NULL;
            _state = s_closed;
        }
    }
    else
    if( _state > s_starting  &&  _state < s_closing ) {
        if( _process->_xml_client_connection  &&  _process->_xml_client_connection->is_send_possible() ) {
            try {
                assert(_process->_remote_process_id);
                S xml;
                xml << "<remote_scheduler.remote_task.close process_id='" << _process->_remote_process_id << "'";
                if( kill )  xml << " kill='yes'";
                xml << "/>";
                _process->_xml_client_connection->send( xml );
                _state = s_closing;
            }
            catch( exception& x )  { _process->_log->warn( x.what() ); }
        }
    }
}

//---------------------------------------------------------------------------Abstract_startable_process::kill

bool Abstract_startable_process::kill()
{
    bool result = false;

    if( !_is_killed  &&  _connection )
    {
        if( object_server::Connection_to_own_server_process* c = dynamic_cast<object_server::Connection_to_own_server_process*>( +_connection ) )
        {
            result = c->kill_process();
        }
        else
        if( dynamic_cast<object_server::Connection_to_own_server_thread*>( +_connection ) )
        {
            if( pid_t pid = this->pid() )
                try_kill_process_immediately( pid );   // Für kind_remote kind_process (Process_module_instance)
        }
    }

    if( result )  _is_killed = true;

    return result;
}

//------------------------------------------------------------------Abstract_startable_process::pid

int Abstract_startable_process::pid() const
{ 
    return _connection? _connection->pid() : 0;
}

//------------------------------------------------------------------------------Thread_process::pid

int Thread_process::pid() const
{ 
    int result = 0;

    if( _connection )
    {
        if( _com_server_thread )
        {
            if( ptr<Object_server> object_server = _com_server_thread->_object_server )
            {
                // Hmm, scheint der einzige Weg zu sein, an die Pid der Process_module_instance heranzukommen.

                Object* o = object_server->get_class_object_or_null( spooler_com::CLSID_Remote_module_instance_server );

                if( Com_remote_module_instance_server::Class_data* class_data = dynamic_cast<Com_remote_module_instance_server::Class_data*>( o ) )
                {
                    result = class_data->_remote_instance_pid;
                }
            }
        }
        else
        {
            result = Abstract_startable_process::pid();
        }
    }

    return result;
}

//--------------------------------------------------------Abstract_startable_process::is_terminated

bool Abstract_startable_process::is_terminated()
{
    return !_connection  ||  _connection->process_terminated();
}

//------------------------------------------------------------Abstract_startable_process::exit_code

int Abstract_startable_process::exit_code()
{
    if( _connection )  _exit_code = _connection->exit_code();

    return _exit_code;
}

//----------------------------------------------------Abstract_startable_process::termination_signal

int Abstract_startable_process::termination_signal()
{
    if( _connection )  _termination_signal = _connection->termination_signal();

    return _termination_signal;
}

//-------------------------------------------------------Abstract_startable_process::is_remote_host

bool Abstract_startable_process::is_remote_host() const
{ 
    return _configuration._remote_scheduler; 
}

//----------------------------------------------------------Abstract_startable_process::stdout_path

File_path Abstract_startable_process::stdout_path()
{
    File_path result;

    if( object_server::Connection_to_own_server_process* c = dynamic_cast< object_server::Connection_to_own_server_process* >( +_connection ) )
    {
        result = c->stdout_path();
    }

    return result;
}

//----------------------------------------------------------Abstract_startable_process::stderr_path

File_path Abstract_startable_process::stderr_path()
{
    File_path result;

    if( object_server::Connection_to_own_server_process* c = dynamic_cast< object_server::Connection_to_own_server_process* >( +_connection ) )
    {
        result = c->stderr_path();
    }

    return result;
}

//--------------------------------------------------Abstract_startable_process::delete_files__start

bool Abstract_startable_process::try_delete_files( Has_log* log )
{
    bool result = true;

    if( object_server::Connection_to_own_server_process* c = dynamic_cast< object_server::Connection_to_own_server_process* >( +_connection ) )
    {
        result = c->try_delete_files( log );
    }

    return result;
}

//------------------------------------------------------Abstract_startable_process::undeleted_files

list<File_path> Abstract_startable_process::undeleted_files()
{
    list<File_path> result;

    if( object_server::Connection_to_own_server_process* c = dynamic_cast< object_server::Connection_to_own_server_process* >( +_connection ) )
    {
        result = c->undeleted_files();
    }

    return result;
}

//----------------------------------------------------------Abstract_startable_process::dom_element

xml::Element_ptr Abstract_startable_process::dom_element(const xml::Document_ptr& document, const Show_what& show_what)
{
    xml::Element_ptr process_element = Abstract_process::dom_element(document, show_what);
    if (_connection) {
        process_element.setAttribute( "pid", _connection->pid() );
        process_element.setAttribute( "operations", _connection->operation_count() );
        process_element.setAttribute( "callbacks", _connection->callback_count() );
    }
    process_element.setAttribute( "running_since", _running_since.xml_value() );
    if (Async_operation* operation = _connection? _connection->current_super_operation() : NULL)
        process_element.setAttribute("operation", operation->async_state_text());
    return process_element;
}

//-------------------------------------------------------------Abstract_startable_process::obj_name

string Abstract_startable_process::obj_name() const
{
    return "Abstract_process " + short_name();
}

//-----------------------------------------------------------Abstract_startable_process::short_name

string Abstract_startable_process::short_name() const
{
    S result;

    if( _connection )  result << _connection->short_name();
    if( _remote_pid )  result << ",pid=" << _remote_pid;

    return result;
}

//-----------------------------------------Process_class_configuration::Process_class_configuration

Process_class_configuration::Process_class_configuration( Scheduler* scheduler, const string& name )
: 
    Idispatch_implementation( &class_descriptor ),
    file_based<Process_class_configuration,Process_class_folder,Process_class_subsystem>( scheduler->process_class_subsystem(), static_cast< spooler_com::Iprocess_class* >( this ), type_process_class ),
    _zero_(this+1),
    _max_processes(30)
{ 
    if( name != "" )  set_name( name );
}

//---------------------------------------------------Process_class_configuration::set_max_processes

void Process_class_configuration::set_max_processes( int max_processes )
{
    if( max_processes < 0 )  z::throw_xc( "SCHEDULER-420", "Process_class.max_processes", max_processes );
    check_max_processes( max_processes );
    
    _max_processes = max_processes;
}

//------------------------------------------------Process_class_configuration::set_remote_scheduler

void Process_class_configuration::set_remote_scheduler( const Host_and_port& remote_scheduler )
{
    check_remote_scheduler( remote_scheduler );

    _remote_scheduler = remote_scheduler;
    if( _remote_scheduler._host  &&  _remote_scheduler._port == 0 )  _remote_scheduler._port = _spooler->_tcp_port;
}

//------------------------------------------------------------Process_class_configuration::obj_name

string Process_class_configuration::obj_name() const
{
    S result;

    result << "Process_class " << path().without_slash();

    return result;
}

//-------------------------------------------------------------Process_class_configuration::set_dom

void Process_class_configuration::set_dom( const xml::Element_ptr& e )
{
    if( !e )  return;
    if( !e.nodeName_is( "process_class" ) )  z::throw_xc( "SCHEDULER-409", "process_class", e.nodeName() );

    string name = e.getAttribute( "name" );
    if( name != "" )  set_name( name );         // Leere Name steht für die Default-Prozessklasse

    set_max_processes   ( (int)e.uint_getAttribute( "max_processes"   , _max_processes ) );
    set_remote_scheduler(      e.     getAttribute( "remote_scheduler", _remote_scheduler.as_string() ) );
}

//---------------------------------------------------------Process_class_configuration::dom_element

xml::Element_ptr Process_class_configuration::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    xml::Element_ptr result = document.createElement( "process_class" );
        
    fill_file_based_dom_element( result, show_what );
    result.setAttribute         ( "max_processes"   , _max_processes );
    result.setAttribute_optional( "remote_scheduler", _remote_scheduler.as_string() );

    return result;
}

//------------------------------------------------------------Process_class_configuration::put_Name

STDMETHODIMP Process_class_configuration::put_Name( BSTR name_bstr )
{
    HRESULT hr = S_OK;

    try
    {
        set_name( string_from_bstr( name_bstr ) );
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//------------------------------------------------Process_class_configuration::put_Remote_scheduler

STDMETHODIMP Process_class_configuration::put_Remote_scheduler( BSTR remote_scheduler_bstr )
{
    HRESULT hr = S_OK;

    try
    {
        set_remote_scheduler( string_from_bstr( remote_scheduler_bstr ) );
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//---------------------------------------------------Process_class_configuration::put_Max_processes

STDMETHODIMP Process_class_configuration::put_Max_processes( int max_processes )
{
    HRESULT hr = S_OK;

    try
    {
        set_max_processes( max_processes );
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//--------------------------------------------------------------Process_class_configuration::Remove

STDMETHODIMP Process_class_configuration::Remove()
{
    HRESULT hr = S_OK;

    try
    {
        remove( File_based::rm_base_file_too );
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//----------------------------------------------------------------------Process_class::Process_class

Process_class::Process_class( Scheduler* scheduler, const string& name )
:
    Process_class_configuration( scheduler, name ),
    javabridge::has_proxy<Process_class>(spooler()),
    _zero_(this+1)
{
}

//--------------------------------------------------------------------Process_class::~Process_class
    
Process_class::~Process_class()
{
    Z_DEBUG_ONLY( assert( _process_set.empty() ) );
    Z_DEBUG_ONLY( assert( _waiting_jobs.empty() ) );

    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << " ERROR " << x.what() << "\n" ); }
}

//-----------------------------------------------------------------Process_class::set_configuration

void Process_class::set_configuration( const Process_class_configuration& configuration )
{
    if( normalized_path() != configuration.normalized_path() )  assert(0), z::throw_xc( Z_FUNCTION );

    // Erst die Warnungen
    check_max_processes   ( configuration.max_processes() );
    check_remote_scheduler( configuration.remote_scheduler() );

    // Jetzt ändern. Es sollte keine Exception auftreten.
    set_max_processes   ( configuration.max_processes() );
    set_remote_scheduler( configuration.remote_scheduler() );
}

//-----------------------------------------------------------------------------Process_class::close

void Process_class::close()
{
    Z_FOR_EACH( Process_set, _process_set, it )
    {
        if (Process* process = *it) {
            log()->warn( message_string( "SCHEDULER-871", process->obj_name() ) );      // Das sollte nicht passieren
        }
    }
}

//---------------------------------------------------------------------Process_class::on_initialize

bool Process_class::on_initialize()
{
    return true;
}

//---------------------------------------------------------------------------Process_class::on_load

bool Process_class::on_load()
{
    return true;
}

//-----------------------------------------------------------------------Process_class::on_activate

bool Process_class::on_activate()
{
    return true;
}

//----------------------------------------------------------------Process_class::can_be_removed_now

bool Process_class::can_be_removed_now()
{
    return _process_set.empty();
}

//----------------------------------------------------------------Process_class::prepare_to_replace

void Process_class::prepare_to_replace()
{
    assert( replacement() );
}

//---------------------------------------------------------------Process_class::can_be_replaced_now

bool Process_class::can_be_replaced_now()
{
    return true;
}

//--------------------------------------------------------------------Process_class::on_replace_now

Process_class* Process_class::on_replace_now()
{
    set_configuration( *replacement() );
    set_replacement( NULL );

    return this;
}

//---------------------------------------------------------------Process_class::check_max_processes

void Process_class::check_max_processes( int ) const
{
}

//-----------------------------------------------------------------Process_class::set_max_processes

void Process_class::set_max_processes( int max_processes )
{
    // Keine Exception bei Aufruf aus set_configuration() auslösen!

    if( _process_set.size() > max_processes )  log()->warn( message_string( "SCHEDULER-419", max_processes, _process_set.size() ) );

    _max_processes = max_processes;

    notify_a_process_is_idle();
}

//------------------------------------------------------------Process_class::check_remote_scheduler

void Process_class::check_remote_scheduler( const Host_and_port& ) const
{
}

//-----------------------------------------------------------------------Process_class::add_process

void Process_class::add_process(Process* process)
{
    _process_set.insert( process );
    _process_set_version++;
}

//--------------------------------------------------------------------------Spooler::remove_process

void Process_class::remove_process(Process* process)
{
    Process_set::iterator it = _process_set.find( process );
    if( it == _process_set.end() )  z::throw_xc( Z_FUNCTION );

    _process_set.erase( it ); 
    _process_set_version++;

    if( is_to_be_removed()  &&  can_be_removed_now() )
    {
        remove();
        // this ist ungültig!
    }
    else
    if( !_waiting_jobs.empty() )
    {
        Job* job = *_waiting_jobs.rbegin();
        job->notify_a_process_is_idle();
    }
}

//------------------------------------------------------------------------Process_class::new_process

Process* Process_class::new_process(const Process_configuration* c)
{
    assert_is_active();
    ptr<Process> process;
    if (!c) {
        ptr<Dummy_process> p = Z_NEW(Dummy_process(_spooler, Process_configuration()));
        process = +p;
    } else {
        Process_configuration conf = *c;
        conf._remote_scheduler = c->_remote_scheduler.is_empty()? _remote_scheduler : c->_remote_scheduler;
        ptr<Process> p = Process::new_process(_spooler, conf);
        process = +p;
    }
    add_process(process);
    return process;
}

//--------------------------------------------------------Process_class::select_process_if_available

Process* Process_class::select_process_if_available(const Process_configuration* process_configuration)
{
    if (!is_to_be_removed()  &&                                    // remove_process() könnte sonst Process_class löschen.
        file_based_state() == File_based::s_active &&
        _process_set.size() < _max_processes
         && _spooler->_process_count < scheduler::max_processes)
    {
        return new_process(process_configuration);
    }
    else
        return NULL;
}

//-----------------------------------------------------------------Process_class::process_available

bool Process_class::process_available( Job* for_job )
{ 
    if( file_based_state() != File_based::s_active )  return false;
    if( _process_set.size()      >= _max_processes )  return false;
    if( _spooler->_process_count >= scheduler::max_processes )  return false;

    if( _waiting_jobs.empty() )  return true;

    // Warten Jobs auf einen freien Prozess? 
    // Dann liefern wir nur true, wenn dieser Job der erste in der Warteschlange ist.
    return *_waiting_jobs.rbegin() == for_job;
}

//---------------------------------------------------------------Process_class::enqueue_waiting_job

void Process_class::enqueue_waiting_job( Job* job )
{
    _waiting_jobs.push_back( job );
}

//----------------------------------------------------------------Process_class::remove_waiting_job

void Process_class::remove_waiting_job( Job* waiting_job )
{
    _waiting_jobs.remove( waiting_job );

    if( _process_set.size() < _max_processes  &&  !_waiting_jobs.empty() )
    {
        Job* job = *_waiting_jobs.rbegin();
        job->notify_a_process_is_idle();
    }
}

//----------------------------------------------------------------------Process_class::need_process

bool Process_class::need_process()
{ 
    return !_waiting_jobs.empty(); 
}

//----------------------------------------------------------Process_class::notity_a_process_is_idle

void Process_class::notify_a_process_is_idle()
{
    if( !_waiting_jobs.empty() )  (*_waiting_jobs.begin())->notify_a_process_is_idle();
}

//-----------------------------------------------------------------------Process_class::dom_element

xml::Element_ptr Process_class::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    xml::Element_ptr result = Process_class_configuration::dom_element( document, show_what );

    result.setAttribute( "processes", (int)_process_set.size() );

    if( !_process_set.empty() )
    {
        xml::Element_ptr processes_element = result.append_new_element( "processes" );
        FOR_EACH( Process_set, _process_set, it )  processes_element.appendChild( (*it)->dom_element( document, show_what ) );
    }

    if( !_waiting_jobs.empty() )
    {
        xml::Element_ptr waiting_jobs_element = document.createElement( "waiting_jobs" );
        result.appendChild( waiting_jobs_element );

        FOR_EACH( Job_list, _waiting_jobs, j )  //waiting_jobs_element.appendChild( (*j)->dom_element( document, show_standard ) );
        {
            xml::Element_ptr job_element = document.createElement( "job" );
            job_element.setAttribute( "job", (*j)->name() );
            waiting_jobs_element.appendChild( job_element );
        }
    }

    return result;
}

//-----------------------------------------------------------------------Process_class::execute_xml

//xml::Element_ptr Process_class::execute_xml( Command_processor* command_processor, const xml::Element_ptr& element, const Show_what& )
//{
//    if( element.nodeName_is( "process_class.remove" ) ) 
//    {
//        remove( File_based::rm_base_file_too );
//    }
//    else
//        z::throw_xc( "SCHEDULER-409", "process_class.remove", element.nodeName() );
//
//    return command_processor->_answer.createElement( "ok" );
//}

//-------------------------------------------------------Process_class_folder::Process_class_folder

Process_class_folder::Process_class_folder( Folder* folder )
:
    typed_folder<Process_class>( folder->spooler()->process_class_subsystem(), folder, type_process_class_folder )
{
}

//------------------------------------------------------Process_class_folder::~Process_class_folder

Process_class_folder::~Process_class_folder()
{
}

//-------------------------------------------------Process_class_subsystem::Process_class_subsystem

Process_class_subsystem::Process_class_subsystem( Scheduler* scheduler )
:
    _zero_(this+1),
    Idispatch_implementation( &class_descriptor ),
    file_based_subsystem<Process_class>( scheduler, static_cast<Iprocess_classes*>( this ), type_process_class_subsystem )
{
}

//----------------------------------------------------Process_class_subsystem::subsystem_initialize

bool Process_class_subsystem::subsystem_initialize()
{
    set_subsystem_state( subsys_initialized );
    file_based_subsystem<Process_class>::subsystem_initialize();

    spooler()->root_folder()->process_class_folder()->add_process_class( Z_NEW(Process_class(spooler(), "")));     // Default-Prozessklasse ohne Namen

    return true;
}

//----------------------------------------------------------Process_class_subsystem::subsystem_load

bool Process_class_subsystem::subsystem_load()
{
    set_subsystem_state( subsys_loaded );
    file_based_subsystem<Process_class>::subsystem_load();
    return true;
}

//------------------------------------------------------Process_class_subsystem::subsystem_activate

bool Process_class_subsystem::subsystem_activate()
{
    set_subsystem_state( subsys_active );
    file_based_subsystem<Process_class>::subsystem_activate();

    return true;
}

//-------------------------------------------------------------------Process_class_subsystem::close

void Process_class_subsystem::close()
{
    set_subsystem_state( subsys_stopped );
    file_based_subsystem<Process_class>::close();
}

//----------------------------------------------------------Process_class_subsystem::async_continue

bool Process_class_subsystem::async_continue()
{
    bool something_done = false;

    FOR_EACH_FILE_BASED( Process_class, process_class )
    {
        int v = process_class->_process_set_version;
        FOR_EACH( Process_class::Process_set, process_class->_process_set, p )
        {
            something_done |= (*p)->async_continue();
            if (process_class->_process_set_version != v) break;
        }
    }

    return something_done;
}

//-------------------------------------------------------------Process_class_subsystem::execute_xml

//xml::Element_ptr Process_class_subsystem::execute_xml( Command_processor* command_processor, const xml::Element_ptr& element, const Show_what& show_what )
//{
//    xml::Element_ptr result;
//
//    if( element.nodeName_is( "process_class" ) ) 
//    {
//        //result = spooler()->root_folder()->process_class_folder()->execute_xml_process_class( command_processor, element );
//        spooler()->root_folder()->process_class_folder()->add_or_replace_file_based_xml( element );
//        result = command_processor->_answer.createElement( "ok" );
//    }
//    else
//    if( string_begins_with( element.nodeName(), "process_class." ) ) 
//    {
//        result = process_class( Absolute_path( root_path, element.getAttribute( "process_class" ) ) )->execute_xml( command_processor, element, show_what );
//    }
//    else
//        z::throw_xc( "SCHEDULER-113", element.nodeName() );
//
//    return result;
//}

//-------------------------------------------------------Process_class_subsystem::get_Process_class

STDMETHODIMP Process_class_subsystem::get_Process_class( BSTR path_bstr, spooler_com::Iprocess_class** result )
{
    HRESULT hr = S_OK;

    try
    {
        *result = process_class( Absolute_path( root_path, string_from_bstr( path_bstr ) ) );
        if( *result )  (*result)->AddRef();
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-----------------------------------------------Process_class_subsystem::get_Process_class_or_null

STDMETHODIMP Process_class_subsystem::get_Process_class_or_null( BSTR path_bstr, spooler_com::Iprocess_class** result )
{
    HRESULT hr = S_OK;

    try
    {
        *result = process_class_or_null( Absolute_path( root_path, string_from_bstr( path_bstr ) ) );
        if( *result )  (*result)->AddRef();
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//----------------------------------------------------Process_class_subsystem::Create_process_class

STDMETHODIMP Process_class_subsystem::Create_process_class( spooler_com::Iprocess_class** result )
{
    HRESULT hr = S_OK;

    try
    {
        ptr<Process_class> process_class = Z_NEW( Process_class( spooler() ) );
        //nicht nötig  process_class->set_folder_path( root_path );
        *result = process_class.take();
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-------------------------------------------------------Process_class_subsystem::Add_process_class

STDMETHODIMP Process_class_subsystem::Add_process_class( spooler_com::Iprocess_class* iprocess_class )
{
    HRESULT hr = S_OK;

    try
    {
        Process_class* process_class = dynamic_cast<Process_class*>( iprocess_class );
        if( !process_class )  return E_POINTER;

        Folder* folder = spooler()->root_folder();
        process_class->set_folder_path( folder->path() );
        process_class->initialize();

        Process_class* current_process_class = process_class_or_null( process_class->path() );
        if( current_process_class  &&  current_process_class->is_to_be_removed() )
        {
            current_process_class->replace_with( process_class );
        }
        else
        {
            folder->process_class_folder()->add_process_class( process_class );
            process_class->activate();
        }
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
