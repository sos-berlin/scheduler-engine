#include "spooler.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__async__CppCall.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__Api_process_configurationC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__processclass__agent__CppHttpRemoteApiProcessClient.h"

typedef javaproxy::com::sos::scheduler::engine::kernel::processclass::agent::CppHttpRemoteApiProcessClient CppHttpRemoteApiProcessClientJ;

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------------------------const
    
const int connection_retry_time = 60;

Process_class_subsystem    ::Class_descriptor    Process_class_subsystem    ::class_descriptor ( &typelib, "Spooler.Process_classes", Process_class_subsystem::_methods );
Process_class_configuration::Class_descriptor    Process_class_configuration::class_descriptor ( &typelib, "Spooler.Process_class"  , Process_class          ::_methods );

//------------------------------------------------------------------------------Abstract_process
// Ein Prozess, in dem ein Module oder eine Task ablaufen kann.
// Kann auch ein Thread sein.

struct Abstract_process : virtual Process {
    protected: Abstract_process(Spooler* spooler, Prefix_log* log, const Api_process_configuration& c) :
        _spooler(spooler),
        _configuration(c),
        _process_id(spooler->get_next_process_id()),
        _prefix_log(Z_NEW(Prefix_log(this))),
        _log(log? log : +_prefix_log )
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
        process_element.setAttribute_optional("remote_scheduler", _configuration._remote_scheduler_address);
        return process_element;
    }

    public: Process_id process_id() const { 
        return _process_id; 
    }

    public: Spooler* const _spooler;
    public: Api_process_configuration const _configuration;
    private: Process_id const _process_id;
    private: ptr<Prefix_log> const _prefix_log;
    private: ptr<Prefix_log> const _log;
};


struct Async_tcp_operation;


struct Abstract_api_process : virtual Api_process, virtual Abstract_process {
    struct Close_operation : Async_operation {
        enum State { s_initial, s_closing_session, s_closing_remote_process, s_finished };

        public: Close_operation(Abstract_api_process* p, bool run_independently) : 
            _state(s_initial),
            _close_session_operation(NULL),
            _process(p),
            _hold_self(run_independently? this : NULL)
        {}

        public: ~Close_operation() {}

        public: bool async_continue_(Async_operation::Continue_flags) {
            return _process->continue_close_operation(this);
        }

        public: bool async_finished_() const {
            return _state == s_finished;
        }

        public: string async_state_text_() const {
            S result;
            result << "Abstract_api_process::Close_operation " << string_from_state(_state);
            if (_close_session_operation)
                result << " " << _close_session_operation->async_state_text();
            if (_process)
                result << " " << _process->async_state_text();
            return result;
        }

        public: static string string_from_state(State state) {
            switch (state) {
                case s_initial:                 return "initial";
                case s_closing_session:         return "closing_session";
                case s_closing_remote_process:  return "closing_remote_process";
                case s_finished:                return "finished";
                default:                        return S() << "State(" << state << ")";
            }
        }

        friend struct Abstract_api_process;

        private: State _state;
        private: ptr<Abstract_api_process> _process;
        private: Async_operation* _close_session_operation;
        private: ptr<Close_operation> _hold_self;              // Objekt hält sich selbst, wenn es selbstständig, ohne Antwort, den Abstract_process schließen soll
    };

    protected: Abstract_api_process(Spooler* spooler, Prefix_log* log, const Api_process_configuration& conf) :
        Abstract_process(spooler, log, conf),
        _registered_process_handle((Process_handle)0),
        _exit_code(0),
        _termination_signal(0)
    {}

    public: ~Abstract_api_process() {
        if (_close_operation) 
            _close_operation->set_async_manager(NULL);
        if (_registered_process_handle)
            _spooler->unregister_process_handle(_registered_process_handle);
    }

    public: void close_async() {
        if (!_close_operation) {
            if (!is_terminated())
                emergency_kill();
            close__start(true);
        }
    }

    public: Async_operation* close__start(bool run_independently) {
        assert(!_close_operation);
        _close_operation = Z_NEW(Close_operation(this, run_independently));
        _close_operation->set_async_next_gmtime((time_t)0);
        _close_operation->set_async_manager(_spooler->_connection_manager);
        //_close_operation->async_continue();
        return _close_operation;
    }

    public: void close__end() {
        if (_close_operation) {
            _close_operation->set_async_manager(NULL);
            _close_operation = NULL;
        }
        _session = NULL;
    }

    public: void close_session() { 
        if (_session) {
            _session->close__start()->async_finish();
            _session->close__end();
            _session = NULL;
        }
    }

    public: void start() {
        if (connection() != NULL) assert(0), z::throw_xc(Z_FUNCTION);
        do_start();
        #ifdef Z_WINDOWS
            if (_spooler)
                connection()->set_event(&_spooler->_scheduler_event);
        #endif
        log()->set_prefix(obj_name());
        _session = Z_NEW(object_server::Session(connection()));
        _session->set_connection_has_only_this_session();
        _running_since = Time::now();
    }
    
    protected: virtual void do_start() = 0;

    protected: void fill_connection(object_server::Connection* connection) {
        connection->set_stdin_data(stdin_xml(connection));
        if (_configuration._controller_address) 
            connection->set_controller_address(_configuration._controller_address);
    }

    private: string stdin_xml(object_server::Connection* connection) const {
        xml::Xml_string_writer stdin_xml_writer;
        stdin_xml_writer.set_encoding(string_encoding);
        stdin_xml_writer.write_prolog();
        stdin_xml_writer.begin_element("task_process");
        {
            stdin_xml_writer.set_attribute_optional( "include_path"          , _spooler->include_path() );
            stdin_xml_writer.set_attribute_optional( "java_options"          , _spooler->java_subsystem()->java_vm()->options() +" "+
                _spooler->settings()->_job_java_options);
            stdin_xml_writer.set_attribute_optional( "java_class_path"       , spooler()->settings()->_job_java_classpath + Z_PATH_SEPARATOR +
                _spooler->java_subsystem()->java_vm()->class_path());
            stdin_xml_writer.set_attribute_optional( "java_work_dir"         , _spooler->java_work_dir() );
            stdin_xml_writer.set_attribute_optional( "scheduler.directory"   , _spooler->directory() );      // Für Com_spooler_proxy::get_Directory
            stdin_xml_writer.set_attribute_optional( "scheduler.log_dir"     , _spooler->_log_directory );   // Für Com_spooler_proxy::get_Log_dir
            stdin_xml_writer.set_attribute_optional( "scheduler.include_path", _spooler->_include_path );    // Für Com_spooler_proxy::get_Include_path
            stdin_xml_writer.set_attribute_optional( "scheduler.ini_path"    , _spooler->_factory_ini );     // Für Com_spooler_proxy::get_Ini_path
            if (object_server::Connection_to_own_server_process* c = dynamic_cast<object_server::Connection_to_own_server_process*>(connection)) {
                stdin_xml_writer.set_attribute("stdout_path", c->stdout_path());
                stdin_xml_writer.set_attribute("stderr_path", c->stderr_path());
            }
            if (_configuration._log_stdout_and_stderr)
                stdin_xml_writer.set_attribute("log_stdout_and_stderr", "yes");
            if (_configuration._environment) {
                xml::Document_ptr dom_document = _configuration._environment->dom("environment", "variable");
                stdin_xml_writer.write_element( dom_document.documentElement() );
            }
        }
        stdin_xml_writer.end_element("task_process");
        stdin_xml_writer.flush();
        return stdin_xml_writer.to_string();
    }

    public: bool async_continue() {
        return connection()? connection()->async_continue() : false;
    }

    protected: virtual bool is_non_close_async_operation_active() const {
        return false;
    }

    protected: virtual void emergency_kill() {
        log()->warn(message_string("SCHEDULER-850", obj_name()));
        try {
            kill(Z_SIGKILL);
        } catch (exception& x) { 
            log()->warn(x.what()); 
        }
    }

    public: int pid() const { 
        return connection()? connection()->pid() : 0;
    }

    public: bool is_terminated() {
        return !connection() || connection()->process_terminated();
    }

    public: int exit_code() {
        if (connection())
            _exit_code = connection()->exit_code();
        return _exit_code;
    }

    public: int termination_signal() {
        if (connection())
            _termination_signal = connection()->termination_signal();
        return _termination_signal;
    }

    public: xml::Element_ptr dom_element(const xml::Document_ptr& document, const Show_what& show_what) {
        xml::Element_ptr process_element = Abstract_process::dom_element(document, show_what);
        if (connection()) {
            process_element.setAttribute( "pid", connection()->pid() );
            process_element.setAttribute( "operations", connection()->operation_count() );
            process_element.setAttribute( "callbacks", connection()->callback_count() );
        }
        process_element.setAttribute( "running_since", _running_since.xml_value() );
        if (Async_operation* operation = connection()? connection()->current_super_operation() : NULL)
            process_element.setAttribute("operation", operation->async_state_text());
        return process_element;
    }

    public: string obj_name() const {
        return "Api_process " + short_name();
    }

    public: virtual string short_name() const {
        return connection() ? connection()->short_name() : "";
    }

    private: bool continue_close_operation(Abstract_api_process::Close_operation*);

    protected: virtual bool on_session_closed(Abstract_api_process::Close_operation*) {
        return false;
    }

    protected: virtual void on_closing_remote_process() {}

    public: bool is_remote_host() const {
        return !_configuration._remote_scheduler_address.empty(); 
    }       

    public: object_server::Session* session() { 
        return _session; 
    }

    public: double async_next_gmtime() { 
        return connection() ? connection()->async_next_gmtime() : time::never_double; 
    }

    public: Scheduler_object::Type_code scheduler_type_code() const { 
        return Abstract_process::scheduler_type_code(); 
    }

    public: Spooler* spooler() const { 
        return Abstract_process::spooler(); 
    }

    public: Prefix_log* log() const { 
        return Abstract_process::log(); 
    }

    public: Process_id process_id() const { 
        return Abstract_process::process_id(); 
    }

    protected: virtual string async_state_text() const {
        return "";
    }

    private: ptr<object_server::Session> _session;                // Wir haben immer nur eine Session pro Verbindung
    protected: Process_handle _registered_process_handle;
    private: int _exit_code;
    private: int _termination_signal;
    private: Time _running_since;
    private: ptr<Close_operation> _close_operation;
};


struct Dummy_process : Abstract_process {
    public: Dummy_process(Spooler* spooler, Prefix_log* log, const Api_process_configuration& conf) :
        Abstract_process(spooler, log, conf) 
    {}

    public: bool async_continue() {
        return false;
    }

    public: bool is_started() {
        z::throw_xc(Z_FUNCTION);
    }

    public: object_server::Connection* connection() const {
        return NULL;
    }

    public: double async_next_gmtime() { 
        return time::never_double; 
    }

    public: string obj_name() const {
        return short_name();
    }

    public: string short_name() const {
        return "Dummy_process";
    }
};


struct Standard_local_api_process : Local_api_process, virtual Abstract_api_process {
    Standard_local_api_process(Spooler* spooler, Prefix_log* log, const Api_process_configuration& conf) :
        Abstract_process(spooler, log, conf),
        Abstract_api_process(spooler, log, conf),
        _is_killed(false)
    {}

    protected: void do_start() {
        _connection = new_connection();
        _connection->start_process(parameters());
        _registered_process_handle = _connection->process_handle();
        _spooler->register_process_handle(_registered_process_handle);
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
             parameters.push_back(object_server::Parameter("param", "-log=" + filtered_log_categories_as_string() + " >+" + log_filename()));
        parameters.push_back(object_server::Parameter("program", _spooler->_my_program_filename));
        if (_spooler->settings()->_classic_agent_keep_alive_duration < INT_MAX) {
            parameters.push_back(object_server::Parameter("keep-alive", as_string(_spooler->settings()->_classic_agent_keep_alive_duration)));
        }
        return parameters;
    }

    private: string filtered_log_categories_as_string() const {
        string s = log_categories_as_string();
        string stderr_string = "stderr";   // Log-Kategorie zum Debuggen (der String ist zum Suchen)
        return trim(replace_regex((" "+ s +" "), " "+ stderr_string +" ", " "));   // "stderr" nicht an API-Prozess, weil der das nur in eine Datei schreibt, statt auf die IDE-Konsole 
    }

    public: bool is_started() {
        return _connection != NULL;  // Beim Aufruf stets true
    }

    public: bool kill(int unix_signal) {
        if (!_is_killed && _connection) {
            if (int pid = _connection->pid()) {
                if (unix_signal != Z_SIGKILL) {
                    system_interface::kill_with_unix_signal(pid, unix_signal);
                    return false;
                } else {
                    bool killed = _connection->kill_process();
                    _is_killed = killed;
                    return killed;
                }
            }
        }
        return false;
    }

    public: File_path stdout_path() {
        return _connection? _connection->stdout_path() : File_path();
    }

    public: File_path stderr_path() {
        return _connection? _connection->stderr_path() : File_path();
    }

    public: bool try_delete_files(Has_log* log) {
        if (_connection)
            return _connection->try_delete_files( log );
        else
            return true;
    }

    public: list<File_path> undeleted_files() {
        return _connection? _connection->undeleted_files() : list<File_path>();
    }

    public: object_server::Connection* connection() const {
        return +_connection;
    }
    
    private: ptr<object_server::Connection_to_own_server_process> _connection;
    private: bool _is_killed;
};


struct Thread_api_process : Abstract_api_process {
    struct Com_server_thread : object_server::Connection_to_own_server_thread::Server_thread {
        Com_server_thread(object_server::Connection_to_own_server_thread* c, int keep_alive_duration) : 
            object_server::Connection_to_own_server_thread::Server_thread(c),
            _keep_alive_duration(keep_alive_duration)
        {
            set_thread_name("scheduler::Thread_api_process::Com_server_thread");
        }

        int thread_main() {
            Z_LOGI2("Z-REMOTE-118", Z_FUNCTION << "\n");
            int result;
            Com_initialize com_initialize;
            try {
                _object_server = Z_NEW(Object_server);  // Bis zum Ende des Threads stehenlassen, wird von anderem Thread benutzt: Abstract_process::pid()
                _object_server->set_stdin_data(_connection->stdin_data());
                _server = +_object_server;
                result = run_server(_keep_alive_duration);
            } catch (exception& x) {
                string msg = S() << "ERROR in Com_server_thread: " << x.what() << "\n";
                Z_LOG2("scheduler", msg);
                cerr << msg;
                result = 0;
            }
            Z_LOG2("Z-REMOTE-118", Z_FUNCTION << " okay\n");
            return result;
        }

        ptr<Object_server> _object_server;
        int const _keep_alive_duration;
    };

    Thread_api_process(Spooler* spooler, Prefix_log* log, const Api_process_configuration& conf) :
        Abstract_process(spooler, log, conf),
        Abstract_api_process(spooler, log, conf)
    {}

    protected: void do_start() {
        Z_LOGI2("Z-REMOTE-118", Z_FUNCTION << "\n");
        _connection = _spooler->_connection_manager->new_connection_to_own_server_thread();
        fill_connection(_connection);
        int keep_alive_duration = _spooler->settings()->_classic_agent_keep_alive_duration;
        _com_server_thread = Z_NEW(Com_server_thread(_connection, keep_alive_duration));
        _connection->start_thread(_com_server_thread);
        _spooler->log()->debug9(message_string("SCHEDULER-948", _connection->short_name()));  // pid wird auch von Task::set_state(s_starting) mit log_info protokolliert
        Z_LOG2("Z-REMOTE-118", Z_FUNCTION << " okay\n");
    }
    
    public: bool is_started() {
        return _connection != NULL;  // Beim Aufruf stets true
    }

    protected: int pid() const {
        int result = 0;
        if (_com_server_thread && _connection) {
            if (ptr<Object_server> object_server = _com_server_thread->_object_server) {
                Object* o = object_server->get_class_object_or_null(spooler_com::CLSID_Remote_module_instance_server);
                if (Com_remote_module_instance_server::Class_data* class_data = dynamic_cast<Com_remote_module_instance_server::Class_data*>(o)) {
                    result = class_data->_remote_instance_pid;
                }
            }
        }
        return result;
    }

    public: bool kill(int unix_signal) {
        if (_connection) {
            if (pid_t pid = this->pid()) {
                if (unix_signal != Z_SIGKILL) {
                    system_interface::kill_with_unix_signal(pid, unix_signal);
                } else {
                    log()->warn(message_string("SCHEDULER-281"));
                    Message_string m ( "SCHEDULER-709" );
                    system_interface::try_kill_process_with_descendants_immediately(pid, log(), &m, Z_FUNCTION );
                    try_kill_process_immediately(pid);   // Für kind_remote kind_process (Process_module_instance)
                }
            }
        }
        return false;
    }

    public: object_server::Connection* connection() const {
        return +_connection;
    }
    
    private: ptr<object_server::Connection_to_own_server_thread> _connection;
    private: ptr<Com_server_thread> _com_server_thread;
};

struct Tcp_remote_api_process;

struct Async_tcp_operation : Async_operation {
    enum State {
        s_not_connected,
        s_connecting,
        s_starting,
        s_running,
        s_signalling,
        s_closing,
        s_closed
    };

    static string state_name(State);

    Async_tcp_operation(Tcp_remote_api_process*);
    ~Async_tcp_operation();

    virtual bool async_continue_(Continue_flags f);
    
    virtual bool async_finished_() const { 
        return _state == s_running || _state == s_closed; 
    }

    virtual string async_state_text_() const;
    void signal_remote_task(int unix_signal);
    void close_remote_task(bool kill = false);

    public: State _state;
    public: Tcp_remote_api_process* _process;
};


struct Abstract_remote_api_process : Abstract_api_process {
    Abstract_remote_api_process(Spooler* spooler, Prefix_log* log, const Api_process_configuration& conf) :
        Abstract_process(spooler, log, conf),
        Abstract_api_process(spooler, log, conf)
    {}

    protected: virtual Ip_address ip_interface() const = 0;

    protected: void prepare_connection() {
        assert(!_connection);
        _connection = _spooler->_connection_manager->new_connection();
        _connection->listen_on_tcp_port(ip_interface());
    }

    public: object_server::Connection* connection() const {
        return _connection;
    }

    private: ptr<object_server::Connection> _connection;
};


struct Tcp_remote_api_process : Abstract_remote_api_process {
    Tcp_remote_api_process(Spooler* spooler, Prefix_log* log, const Api_process_configuration& conf) :
        Abstract_process(spooler, log, conf),
        Abstract_remote_api_process(spooler, log, conf),
        _remote_scheduler_address(Host_and_port(conf._remote_scheduler_address)),
        _remote_process_id(0),
        _remote_pid(0),
        _is_killed(false),
        _keep_alive_duration(spooler->settings()->_classic_agent_keep_alive_duration == 1/*Test*/? 0.01 : spooler->settings()->_classic_agent_keep_alive_duration)
    {}

    ~Tcp_remote_api_process() {
        if (_async_tcp_operation) {
            _async_tcp_operation->set_async_manager( NULL );
            _async_tcp_operation = NULL;
        }
        if (_xml_client_connection)
            _xml_client_connection->set_async_manager(NULL);
    }

    protected: Ip_address ip_interface() const { 
        return INADDR_ANY;
    }

    protected: void do_start() {
        prepare_connection();
        connection()->set_remote_host(_remote_scheduler_address.host());
        _async_tcp_operation = Z_NEW( Async_tcp_operation( this ) );
        _async_tcp_operation->async_wake();
        _async_tcp_operation->set_async_manager( _spooler->_connection_manager );
    }

    public: bool is_started() {
        return true; // Funktioniert so nicht (sowieso wollen wir http nutzen): _async_tcp_operation->_state == Async_tcp_operation::s_running;
    }

    protected: virtual void emergency_kill() {
        // Nicht möglich, weil kill() asynchron über TCP geht.
    }

    public: bool kill(int unix_signal) {
        if (!connection()) 
            return false;
        else
        if (unix_signal != Z_SIGKILL) {
            if (_async_tcp_operation && _async_tcp_operation->_state == Async_tcp_operation::s_running) {
                _async_tcp_operation->signal_remote_task(unix_signal);
            }
            return true;
        }
        else 
        if (!_is_killed) {
            if (_async_tcp_operation && _async_tcp_operation->_state != Async_tcp_operation::s_closed)
                if (_async_tcp_operation->_state == Async_tcp_operation::s_signalling) z::throw_xc("SCHEDULER-468", "kill", "SIGTERM is in process - please try again");
                _async_tcp_operation->close_remote_task(true);
            _is_killed = true;
            return true;
        } else
            return false;
    }

    private: void keep_alive() {
        if (_async_tcp_operation && _async_tcp_operation->_state == Async_tcp_operation::s_running) {
            bool sent = _xml_client_connection->send_keep_alive_space();  // Send a space which an old Agent will read as a prefix of the next XML command.
            if (sent) {
                log()->info(Message_string("SCHEDULER-727"));
            }
        }
    }

    public: string obj_name() const {
        return "Tcp_remote_api_process " + short_name();
    }

    public: string short_name() const {
        string result = Abstract_remote_api_process::short_name();
        if (_remote_pid)
            result += ",remote_pid=" + _remote_pid;
        return result;
    }

    protected: virtual string async_state_text() const {
        return _async_tcp_operation ? _async_tcp_operation->async_state_text() : "";
    }

    protected: bool on_session_closed(Abstract_api_process::Close_operation* op) {
        if (_async_tcp_operation) {
            _async_tcp_operation->close_remote_task();
            _async_tcp_operation->set_async_parent(op);
            return true;
        } else
            return false;
    }

    protected: void on_closing_remote_process() {
        if (_async_tcp_operation)
            _async_tcp_operation->set_async_parent(NULL);
    }

    protected: bool is_non_close_async_operation_active() const {
        return _async_tcp_operation && !_async_tcp_operation->async_finished() || Abstract_api_process::is_non_close_async_operation_active();
    }

    public: bool async_remote_start_continue(Async_operation::Continue_flags);

    public: string remote_scheduler_address() {
        return _remote_scheduler_address.as_string();
    }

    friend struct Async_tcp_operation;

    private: Host_and_port const _remote_scheduler_address;
    private: ptr<Async_tcp_operation> _async_tcp_operation;
    private: ptr<Xml_client_connection> _xml_client_connection;
    private: Process_id _remote_process_id;
    private: pid_t _remote_pid;
    private: bool _is_killed;
    private: Duration const _keep_alive_duration;
};


struct Http_remote_api_process;
DEFINE_SIMPLE_CALL(Http_remote_api_process , Start_remote_task_callback)
DEFINE_SIMPLE_CALL(Http_remote_api_process, Waiting_callback)

struct Http_remote_api_process : Abstract_remote_api_process {
    Http_remote_api_process(Process_class* process_class, Prefix_log* log, const Api_process_configuration& conf) :
        Abstract_process(process_class->spooler(), log, conf),
        Abstract_remote_api_process(process_class->spooler(), log, conf),
        _process_class(process_class),
        _waiting_callback(Z_NEW(Waiting_callback(this))),
        _start_remote_task_callback(Z_NEW(Start_remote_task_callback(this))),
        _is_started(false)
    {}

    ~Http_remote_api_process() {
        if (_clientJ) {
            _process_class->typed_java_sister().removeCppHttpRemoteApiProcessClient(_clientJ);
        }
    }

    protected: Ip_address ip_interface() const { 
        return Ip_address::localhost;  // TCP is only used to connect with Scala coded tunnel
    }

    protected: void do_start() {
        prepare_connection();
        _clientJ = _process_class->typed_java_sister().startCppHttpRemoteApiProcessClient(
            _configuration.java_proxy_jobject(), connection()->tcp_port(), _waiting_callback->java_sister(), _start_remote_task_callback->java_sister());
    }

    public: void on_call(const Waiting_callback& call) {
        if (::javaproxy::java::lang::Object exception = (::javaproxy::java::lang::Object)call.value()) {
            log()->warn(Message_string("SCHEDULER-488", obj_name(), (string)(exception.toString())));   // "Remote JobScheduler unreachable"
        } else {
            log()->warn(Message_string("SCHEDULER-489"));   // "Waiting"
        }
    }

    public: void on_call(const Start_remote_task_callback& call) {
        assert(&call == +_start_remote_task_callback);
        try {
            _remote_scheduler = (StringJ)((TryJ)call.value()).get();   // get() wirft Exception, wenn call.value() ein Failure ist
            _is_started = true;
        }
        catch (exception& x) {
            _start_exception = x;
        }
        _spooler->signal();
    }

    public: bool is_started() {
        return _is_started;
    }

    public: void check_exception() {
        if (_start_exception) throw *_start_exception;
    }

    protected: void emergency_kill() {
        // Nicht möglich, weil kill() asynchron über HTTP geht.
    }

    public: bool kill(int unix_signal) {
        Z_LOG2("scheduler", Z_FUNCTION << " kill " << unix_signal << "\n");
        _spooler->cancel_call(_start_remote_task_callback);
        if (unix_signal == Z_SIGKILL) {
            _clientJ.closeRemoteTask(true);
            return true;
        } else {
            return _clientJ.killRemoteTask(unix_signal);
        }
    }

    protected: string async_state_text() const {
        return obj_name();
    }

    public: string obj_name() const {
        return _clientJ? (string)_clientJ.toString() : (string)"Http_remote_api_process";
    }

    protected: void on_closing_remote_process() {
        if (_clientJ) {
            _clientJ.closeRemoteTask(false);
        }
    }

    public: string remote_scheduler_address() {
        return _remote_scheduler;
    }

    private: Process_class* const _process_class;
    private: CppHttpRemoteApiProcessClientJ _clientJ;
    private: ptr<Waiting_callback> const _waiting_callback;
    private: ptr<Start_remote_task_callback> const _start_remote_task_callback;
    private: bool _is_started;
    private: Xc_copy _start_exception;
    private: string _remote_scheduler;
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

//---------------------------------------------------Abstract_api_process::continue_close_operation

bool Abstract_api_process::continue_close_operation(Abstract_api_process::Close_operation* op)
{
    bool something_done = false;
    if (op->_state == Close_operation::s_initial) {
        if (_session) {
            op->_close_session_operation = _session->close__start();
            if (!op->_close_session_operation->async_finished()) {
                op->_close_session_operation->set_async_parent(op);
                op->_close_session_operation->set_async_manager(_spooler->_connection_manager);
            }
            something_done = true;
        }
        op->_state = Close_operation::s_closing_session;
    }
    if (op->_state == Close_operation::s_closing_session) {
        #ifdef Z_WINDOWS
            if (op->_close_session_operation)
                op->_close_session_operation->async_continue();       // Falls wir wegen Prozess-Event aufgerufen worden sind
        #endif
        if (!op->_close_session_operation || op->_close_session_operation->async_finished()) {
            if (op->_close_session_operation) {
                op->_close_session_operation->set_async_parent( NULL );
                op->_close_session_operation = NULL;
                _session->close__end();
                _session = NULL;
                something_done = true;
            }
            something_done |= on_session_closed(op);
            op->_state = Close_operation::s_closing_remote_process;
        }
    }
    if (op->_state == Close_operation::s_closing_remote_process) {
        if (!is_non_close_async_operation_active()) {
            on_closing_remote_process();
            op->_state = Close_operation::s_finished;
            ptr<Abstract_api_process> process = op->_process;
            op->_process = NULL;
            if (op->_hold_self) {   // run_independently
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

//---------------------------------------------------Async_remote_operation::Async_remote_operation
    
Async_tcp_operation::Async_tcp_operation(Tcp_remote_api_process* p)
:                        
    _state(s_not_connected), 
    _process(p) 
{
    _process->_spooler->_process_count++;       // Jeder Prozess hat zwei Verbindungen: Zum Prozess und Xml_client_connection zum Scheduler
}

//--------------------------------------------------Async_remote_operation::~Async_remote_operation
    
Async_tcp_operation::~Async_tcp_operation()
{
    --_process->_spooler->_process_count;

    if( _process && _process->_xml_client_connection && _process->_xml_client_connection->async_parent() == this )  
    {
        _process->_xml_client_connection->set_async_parent( NULL );
    }
}

//-----------------------------------Standard_remote_api_process::Async_remote_operation::state_name
    
string Async_tcp_operation::state_name( State state )
{
    string result;

    switch( state )
    {
        case s_not_connected:   result = "not_connected";   break;
        case s_connecting:      result = "connecting";      break;
        case s_starting:        result = "starting";        break;
        case s_running:         result = "running";         break;
        case s_signalling:      result = "signalling";      break;
        case s_closing:         result = "closing";         break;
        case s_closed:          result = "closed";          break;
        default:                result = "Async_remote_operation-" + as_string( state );
    }

    return result;
}


bool Async_tcp_operation::async_continue_(Continue_flags f) { 
    return _process->async_remote_start_continue(f); 
}

//--------------------------------------------------------Async_remote_operation::async_state_text_

string Async_tcp_operation::async_state_text_() const
{
    S result;
    result << "Async_remote_operation " << state_name( _state );
    if( _process )  result << " " << _process->obj_name();
    return result;
}


ptr<Api_process> Api_process::new_process(Spooler* spooler, Prefix_log* log, const Api_process_configuration& configuration) {
    if (configuration._is_thread) {
        ptr<Thread_api_process> result = Z_NEW(Thread_api_process(spooler, log, configuration));
        return +result;
    } else {
        ptr<Standard_local_api_process> result = Z_NEW(Standard_local_api_process(spooler, log, configuration));
        return +result;
    }
}


bool Tcp_remote_api_process::async_remote_start_continue(Async_operation::Continue_flags continue_flags)
{
    bool something_done = true;     // spooler.cxx ruft async_continue() auf

    if( _xml_client_connection )  _xml_client_connection->async_check_exception();

    switch( _async_tcp_operation->_state )
    {
        case Async_tcp_operation::s_not_connected:
        {
            _xml_client_connection = Z_NEW(Xml_client_connection(_spooler, _remote_scheduler_address));
            _xml_client_connection->set_async_parent( _async_tcp_operation );
            _xml_client_connection->set_async_manager( _spooler->_connection_manager );
            _xml_client_connection->set_wait_for_connection( connection_retry_time );
            _xml_client_connection->connect();

            something_done = true;
            _async_tcp_operation->_state = Async_tcp_operation::s_connecting;
        }

        case Async_tcp_operation::s_connecting:
        {
            if( _xml_client_connection->state() != Xml_client_connection::s_connected )  break;

            xml::Xml_string_writer xml_writer;
            xml_writer.set_encoding( string_encoding );
            xml_writer.write_prolog();
            xml_writer.begin_element( "remote_scheduler.start_remote_task" );
            xml_writer.set_attribute( "tcp_port", connection()->tcp_port() );
            if (!_configuration._has_api)  xml_writer.set_attribute( "kind", "process" );
            if (!rtrim(_configuration._java_options).empty())
                xml_writer.set_attribute_optional("java_options", _configuration._java_options);
            if (!rtrim(_configuration._java_classpath).empty())
                xml_writer.set_attribute_optional("java_classpath", _configuration._java_classpath);
            xml_writer.end_element( "remote_scheduler.start_remote_task" );
            xml_writer.close();
            _xml_client_connection->send( xml_writer.to_string() );

            something_done = true;
            _async_tcp_operation->_state = Async_tcp_operation::s_starting;
        }

        case Async_tcp_operation::s_starting:
        {
            xml::Document_ptr dom_document = _xml_client_connection->fetch_received_dom_document();
            if( !dom_document )  break;
                                                              
            Z_LOG2( "scheduler", Z_FUNCTION << " XML response: " << dom_document.xml_string() << "\n");

            //if( _spooler->_validate_xml )  _spooler->_schema.validate( dom_document );

            _remote_process_id = dom_document.select_element_strict( "spooler/answer/process" ).int_getAttribute( "process_id", 0 );
            assert(_remote_process_id);
            _remote_pid        = dom_document.select_element_strict( "spooler/answer/process" ).int_getAttribute( "pid", 0 );

            _spooler->log()->debug9( message_string( "SCHEDULER-948", connection()->short_name() ) );  // pid wird auch von Task::set_state(s_starting) mit log_info protokolliert
            something_done = true;
            _async_tcp_operation->_state = Async_tcp_operation::s_running;
            _async_tcp_operation->set_async_delay(_keep_alive_duration.as_double());
            break;
        }

        case Async_tcp_operation::s_running:
        {
            if (!_keep_alive_duration.is_eternal()) {
                if (continue_flags & Async_operation::cont_next_gmtime_reached) {
                    keep_alive();
                    _async_tcp_operation->set_async_delay(_keep_alive_duration.as_double());
                }
            }
            break;
        }

        case Async_tcp_operation::s_signalling: {
            if( xml::Document_ptr dom_document = _xml_client_connection->fetch_received_dom_document() )  
            {
                Z_LOG2( "zschimmer", Z_FUNCTION << " XML response " << dom_document.xml_string() << "\n");
                something_done = true;
                _async_tcp_operation->_state = Async_tcp_operation::s_running;
                _async_tcp_operation->set_async_delay(_keep_alive_duration.as_double());
            }
            break;
        }

        case Async_tcp_operation::s_closing:
        {
            if( xml::Document_ptr dom_document = _xml_client_connection->fetch_received_dom_document() )  
            {
                Z_LOG2( "zschimmer", Z_FUNCTION << " XML response " << dom_document.xml_string() << "\n");
                //_spooler->log()->debug9( message_string( "SCHEDULER-948", _connection->short_name() ) );  // pid wird auch von Task::set_state(s_starting) mit log_info protokolliert
                something_done = true;
                _async_tcp_operation->_state = Async_tcp_operation::s_closed;
            }

            break;
        }

        default:
            break;
    }

    return something_done;
}

//--------------------------------------------------------Async_remote_operation::close_remote_task

void Async_tcp_operation::signal_remote_task(int unix_signal) {
    if (_state == s_running) {
        if( _process->_xml_client_connection  &&  _process->_xml_client_connection->is_send_possible() ) {
            try {
                assert(_process->_remote_process_id);
                S xml;
                xml << "<remote_scheduler.remote_task.kill process_id='" << _process->_remote_process_id << "' signal='SIGTERM'/>";
                _process->_xml_client_connection->send( xml );
                _state = s_signalling;
            }
            catch( exception& x )  { _process->log()->warn( x.what() ); }
        }
    }
}

void Async_tcp_operation::close_remote_task( bool kill )
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
            catch( exception& x )  { _process->log()->warn( x.what() ); }
        }
    }
}

//-----------------------------------------Process_class_configuration::Process_class_configuration

Process_class_configuration::Process_class_configuration( Scheduler* scheduler, const string& name )
: 
    Idispatch_implementation( &class_descriptor ),
    file_based<Process_class,Process_class_folder,Process_class_subsystem>( scheduler->process_class_subsystem(), static_cast< spooler_com::Iprocess_class* >( this ), type_process_class ),
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

void Process_class_configuration::set_remote_scheduler_address(const string& remote_scheduler)
{
    _remote_scheduler_address = remote_scheduler;
    if (_remote_scheduler_address.find(':') == string::npos) {  
        Host_and_port hp = _remote_scheduler_address;
        if (hp._host && hp._port == 0) {  // Eigenen Port übernehmen
            hp._port = _spooler->_tcp_port;
            _remote_scheduler_address = hp.as_string();
        }
    }
}

//------------------------------------------------------------Process_class_configuration::obj_name

string Process_class_configuration::obj_name() const
{
    S result;

    result << "Process_class " << path().without_slash();

    return result;
}

//---------------------------------------------------------------------------Process_class::set_dom

void Process_class::set_dom( const xml::Element_ptr& element )
{
    if( !element )  return;
    if( !element.nodeName_is( "process_class" ) )  z::throw_xc( "SCHEDULER-409", "process_class", element.nodeName() );

    string name = element.getAttribute( "name" );
    if( name != "" )  set_name( name );         // Leere Name steht für die Default-Prozessklasse

    set_max_processes((int)element.uint_getAttribute("max_processes", _max_processes));
    set_remote_scheduler_address(element.getAttribute("remote_scheduler", _remote_scheduler_address));
    typed_java_sister().processConfigurationDomElement(element);
}

//---------------------------------------------------------Process_class_configuration::dom_element

xml::Element_ptr Process_class_configuration::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    xml::Element_ptr result = document.createElement( "process_class" );
        
    fill_file_based_dom_element( result, show_what );
    result.setAttribute         ( "max_processes"   , _max_processes );
    result.setAttribute_optional("remote_scheduler", _remote_scheduler_address);

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
        string remote_scheduler = string_from_bstr(remote_scheduler_bstr);
        if (is_http_or_multiple(_remote_scheduler_address)) 
            return E_ACCESSDENIED;
        else
        if (is_http_or_multiple(remote_scheduler)) 
            return E_INVALIDARG;
        else {
            set_remote_scheduler_address(remote_scheduler);
        }
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
    _zero_(this+1),
    _typed_java_sister(java_sister())
{
}

//--------------------------------------------------------------------Process_class::~Process_class
    
Process_class::~Process_class()
{
    Z_DEBUG_ONLY( assert( _process_set.empty() ) );
    Z_DEBUG_ONLY(assert(_requestor_list.empty()));

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
    check_max_processes(configuration.max_processes());

    // Jetzt ändern. Es sollte keine Exception auftreten.
    set_max_processes(configuration.max_processes());
    set_remote_scheduler_address(configuration.remote_scheduler_address());
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
    return _process_set.empty() && _file_order_source_count == 0;
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
    _typed_java_sister.replaceWith(replacement()->_typed_java_sister);
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

    check_then_notify_a_process_is_available();
}

//-----------------------------------------------------------------------Process_class::add_process

void Process_class::add_process(Process* process)
{
    _process_set.insert( process );
    _process_set_version++;
}

void Process_class::remove_process(Process* process)
{
    Process_set::iterator it = _process_set.find( process );
    if( it == _process_set.end() )  z::throw_xc( Z_FUNCTION );

    _process_set.erase( it ); 
    _process_set_version++;

    if( is_to_be_removed()  &&  can_be_removed_now() )
    {
        check_for_replacing_or_removing();
    }
    else
        check_then_notify_a_process_is_available();
}

void Process_class::remove_file_order_source() {
    assert(_file_order_source_count > 0);
    _file_order_source_count--;
    if (is_to_be_removed() && can_be_removed_now()) {
        check_for_replacing_or_removing();
    }
}

//-----------------------------------------------------------------------Process_class::new_process

Process* Process_class::new_process(const Api_process_configuration& conf, Prefix_log* log)
{
    assert_is_active();
    ptr<Process> process;
    if (conf._is_shell_dummy) {
        ptr<Dummy_process> p = Z_NEW(Dummy_process(_spooler, log, conf));
        process = +p;
    } else {
        Api_process_configuration my_conf = conf;
        my_conf._remote_scheduler_address = my_conf._remote_scheduler_address.empty()? _remote_scheduler_address : my_conf._remote_scheduler_address;
        if (is_http_or_multiple(my_conf._remote_scheduler_address)) {
            ptr<Http_remote_api_process> p = Z_NEW(Http_remote_api_process(this, log, my_conf));
            process = +p;
        } else 
        if (!my_conf._remote_scheduler_address.empty()) {       
            ptr<Tcp_remote_api_process> p = Z_NEW(Tcp_remote_api_process(spooler(), log, my_conf));
            process = +p;
        } else {
            ptr<Api_process> p = Api_process::new_process(_spooler, log, my_conf);
            process = +p;
        }
    }
    add_process(process);
    return process;
}


bool Process_class::is_http_or_multiple(const string& remote_scheduler_address) const {
    return remote_scheduler_address.find("://") != string::npos ||  // "[classic:]http[s]://"
        typed_java_sister().hasMoreAgents();                        // hasMoreAgents() setzt HTTP voraus. 
}

//-------------------------------------------------------Process_class::select_process_if_available

Process* Process_class::select_process_if_available(const Api_process_configuration& api_process_configuration, Prefix_log* log)
{
    if (!is_to_be_removed()  &&                                    // remove_process() könnte sonst Process_class löschen.
        file_based_state() == File_based::s_active &&
        _process_set.size() < _max_processes
         && _spooler->_process_count < scheduler::max_processes)
    {
        return new_process(api_process_configuration, log);
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

    // Warten Jobs auf einen freien Prozess? 
    // Dann liefern wir nur true, wenn dieser Job der erste in der Warteschlange ist.
    return _requestor_list.empty() || *_requestor_list.begin() == for_job;
}

void Process_class::enqueue_requestor(Process_class_requestor* requestor) {
    _requestor_list.push_back(requestor);
}

void Process_class::remove_requestor(Process_class_requestor* requestor) {
    if (!_requestor_list.empty()) {
        Process_class_requestor* head = *_requestor_list.begin();
        _requestor_list.remove(requestor);
        if (!_requestor_list.empty() && head != *_requestor_list.begin()) {
            check_then_notify_a_process_is_available();
        }
    }
}

void Process_class::check_then_notify_a_process_is_available() {
    if (Process_class_requestor* o = waiting_requestor_or_null()) {
        o->notify_a_process_is_available();
    }
}

Process_class_requestor* Process_class::waiting_requestor_or_null()
{
    if (file_based_state() == File_based::s_active && !is_to_be_removed()) {
        if (!_requestor_list.empty()) {
            Z_FOR_EACH_CONST(Requestor_list, _requestor_list, i) {
                Process_class_requestor* requestor = *i;
                if (dynamic_cast<Task*>(requestor)) return requestor;  // Hanging tasks first!
            }
            return *_requestor_list.begin();
        }
    }
    return NULL;
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

    if (!_requestor_list.empty()) {
        xml::Element_ptr waiting_tasks_element = document.createElement("waiting_tasks");
        result.appendChild( waiting_tasks_element );
        FOR_EACH(Requestor_list, _requestor_list, i) {
            if (Task* task = dynamic_cast<Task*>(*i)) {
                xml::Element_ptr task_element = document.createElement("task");
                task_element.setAttribute("job", task->job()->path());
                task_element.setAttribute("task", task->id());
                waiting_tasks_element.appendChild(task_element);
            }
        }
    }

    if (!_requestor_list.empty()) {
        xml::Element_ptr waiting_jobs_element = document.createElement( "waiting_jobs" );
        result.appendChild( waiting_jobs_element );
        FOR_EACH(Requestor_list, _requestor_list, i) {  //waiting_jobs_element.appendChild( (*j)->dom_element( document, show_standard ) );
            if (Job* job = dynamic_cast<Job*>(*i)) {
                xml::Element_ptr job_element = document.createElement( "job" );
                job_element.setAttribute( "job", job->name() );
                waiting_jobs_element.appendChild( job_element );
            }
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
