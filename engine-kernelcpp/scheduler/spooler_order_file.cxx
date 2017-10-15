// $Id: spooler_order_file.cxx 14125 2010-11-04 12:32:04Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

/*
    VERBESSERUNG:

    request_order() durch ein Abonnement ersetzen: 
    Job oder Task kann Aufträge abonnieren und das Abonnement auch wieder abbestellen.
    Dann wird das Verzeichnis außerhalb der <schedule/> nicht überwacht.

    Wir brauchen ein Verzeichnis der Abonnementen (struct Job/Task : Order_source_abonnent)
*/


#include "spooler.h"
#include "../javaproxy/com__sos__scheduler__engine__client__agent__CppFileOrderSourceClient.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__async__CppCall.h"

typedef ::javaproxy::com::sos::scheduler::engine::client::agent::CppFileOrderSourceClient CppFileOrderSourceClientJ;

using stdext::hash_set;
using stdext::hash_map;

namespace sos {
namespace scheduler {
namespace order {

using namespace job_chain;

//--------------------------------------------------------------------------------------------const

const Absolute_path             file_order_sink_job_path                  ( "/scheduler_file_order_sink" );
const int                       delay_after_error_default                 = INT_MAX;
const Duration                  file_order_sink_job_idle_timeout_default  = Duration(0);
const Duration minimum_delay = Duration(1);
const int                       max_tries                                 = 2;        // Nach Fehler machen wir sofort einen zweiten Versuch
const bool                      alert_when_directory_missing_default      = true;

#ifdef Z_WINDOWS
    const int   directory_file_order_source_repeat_default  = 60;
#else
    const int   directory_file_order_source_repeat_default  = 10;
#endif

DEFINE_SIMPLE_CALL(Directory_file_order_source, Directory_read_result_call)

//----------------------------------------------------------------------Directory_file_order_source

struct Directory_file_order_source : Directory_file_order_source_interface, Dependant
{
    enum State
    {
        s_none,
        s_order_requested
    };


    explicit                    Directory_file_order_source( Job_chain*, const xml::Element_ptr& );
                               ~Directory_file_order_source();

    // Async_operation:
    virtual Socket_event*       async_event             ()                                          { return &_notification_event; }
    virtual bool                async_continue_         ( Continue_flags );
    virtual bool                async_finished_         () const                                    { return false; }
    virtual string              async_state_text_       () const;

    // Order_source:
    void                        close                   ();
    xml::Element_ptr            dom_element             ( const xml::Document_ptr&, const Show_what& );
    void                        initialize              ();
    void                        activate                ();
    bool                        request_order           ( const string& cause );
    Order*                      fetch_and_occupy_order  (const Order::State&, Task* occupying_task, const Time& now, const string& cause);
    void                        withdraw_order_request  ();
    string                      obj_name                () const;
    void on_call(const Directory_read_result_call&);

    public: 
    bool on_requisite_loaded(File_based* file_based) {
        to_my_process_class(file_based);
        if (_is_active) {
            try_start_watching();
        }
        return true;
    }

    public: 
    bool on_requisite_to_be_removed(File_based* file_based) {
        to_my_process_class(file_based);
        stop_watching();
        return true;
    }

    private:
    bool check_and_handle_process_class_replacement() {
        assert(_is_watching);
        string r = process_class_remote_scheduler();
        if (r == _remote_scheduler) 
            return false;
        else {
            Z_LOG2("scheduler", Z_FUNCTION << " " << obj_name() << ": remote_scheduler=" << r << "\n");
            restart_watching();
            assert(r == _remote_scheduler);
            return true;
        } 
    }

    private:
    void restart_watching() {
        stop_watching();
        start_watching();
    }

    private:
    void try_start_watching() {
        bool complete = _process_class_path.empty() || spooler()->process_class_subsystem()->process_class_or_null(_process_class_path) != NULL;
        if (!complete) {
            Z_LOG2("scheduler", "<file_order_source> is missing process class " << _process_class_path << "\n");
        } else {
            start_watching();
        }
    }

    private: 
    void start_watching() {
        assert(!_is_watching);
        if (!_process_class_path.empty()) {
            spooler()->process_class_subsystem()->process_class(_process_class_path)->add_file_order_source();
        }
        if (Job_node* job_node = Job_node::try_cast(_next_node)) {
            if (Job* next_job = job_node->job_or_null()) {
                if (next_job->state() > Job::s_not_initialized) {
                    next_job->on_order_possibly_available();     // Der Job bestellt den nächsten Auftrag (falls in einer Periode)
                    _expecting_request_order = true;             // Auf request_order() warten
                }
            }
        }
        _remote_scheduler = process_class_remote_scheduler();
        if (_remote_scheduler != "") {
            _fileOrderSourceClientJ = CppFileOrderSourceClientJ::apply(_remote_scheduler, (string)_path, _regex_string, _repeat.millis(), _spooler->injectorJ());
        }
        _is_watching = true;
        repeat_after_delay(Duration(0));
    }

    private:
    void stop_watching() {
        if (_is_watching) {
            close_agent_connection();
            clear_new_files();
            if (!_process_class_path.empty()) {
                spooler()->process_class_subsystem()->process_class(_process_class_path)->remove_file_order_source();
            }
            _is_watching = false;
            set_async_next_gmtime(double_time_max);
        }
    }

    private:
    string process_class_remote_scheduler() {
        if (_process_class_path.empty()) 
            return "";
        else 
            return spooler()->process_class_subsystem()->process_class(_process_class_path)->remote_scheduler_address();
    }

    private:
    void close_agent_connection() {
        if (_fileOrderSourceClientJ.get_jobject()) {
            _fileOrderSourceClientJ.close();
            _fileOrderSourceClientJ = NULL;
            _in_directory_read_result_call = false;
        }
    }

    private: 
    Process_class* to_my_process_class(File_based* file_based) {
        assert(file_based->subsystem() == spooler()->process_class_subsystem());
        assert(file_based->normalized_path() == file_based->subsystem()->normalized_path(_process_class_path));
        Process_class* result = dynamic_cast<Process_class*>(file_based);
        assert(result);
        return result;
    }

    public: 
    Prefix_log* log() const { return Directory_file_order_source_interface::log(); }

  private:
    void                        send_mail               ( Scheduler_event_type, const exception* );
    void                        start_or_continue_notification( bool was_notified );
    void on_directory_read();
    void repeat_after_delay(const Duration& duration);
    void                        close_notification      ();
    void                        read_directory          (bool was_notified);
    void start_read_new_files_from_agent();
    void                        read_new_files_and_handle_deleted_files( const string& cause );
    bool                        read_new_files          ();
    void                        clean_up_blacklisted_files();
    Duration                    delay_after_error       ();
    void                        clear_new_files         ();
    void                        read_known_orders       ( String_set* known_orders );
    void get_blacklisted_files(String_set* result);
    void on_file_removed(Order*);
    bool has_new_file();

    Fill_zero                  _zero_;
    Absolute_path const _process_class_path;
    string _remote_scheduler;
    File_path                  _path;
    string                     _regex_string;
    Regex                      _regex;
    Time _last_continue;
    Duration                   _delay_after_error;
    Duration                   _repeat;
    bool _is_active;
    bool _is_watching;
    bool                       _expecting_request_order;
    Xc_copy                    _directory_error;
    bool                       _send_recovered_mail;
    Event                      _notification_event;             // Nur Windows
    Time                       _notification_event_time;        // Wann wir zuletzt die Benachrichtigung bestellt haben
    bool                       _alert_when_directory_missing;

    vector< ptr<zschimmer::file::File_info> > _new_files;
    int                        _new_files_index;
    int                        _new_files_count;        // _new_files.size() ohne NULL-Einträge
    Time                       _new_files_time;


    struct Bad_entry : Object
    {
                                Bad_entry               ( const File_path& p, const exception& x )  : _file_path(p), _error(x) {}

        File_path              _file_path;
        Xc_copy                _error;
    };

    typedef stdext::hash_map< string, ptr<Bad_entry> >  Bad_map;
    Bad_map                    _bad_map;

    CppFileOrderSourceClientJ _fileOrderSourceClientJ;
    ptr<Directory_read_result_call> const _directory_read_result_call;
    bool _in_directory_read_result_call;
    String_set _agent_request_known_files;
};

//------------------------------------------------------------------------------File_order_sink_job

struct File_order_sink_job : Internal_job {
    File_order_sink_job(Scheduler* scheduler) :
        Internal_job(scheduler, file_order_sink_job_path.without_slash(), new_internal_module(scheduler, "FileOrderSink"))
    {}
};

//-----------------------------------------------------------------------------init_file_order_sink

void init_file_order_sink( Scheduler* scheduler )
{
    ptr<File_order_sink_job> file_order_sink_job = Z_NEW( File_order_sink_job( scheduler ) );

    file_order_sink_job->set_visible( visible_no );
    file_order_sink_job->set_order_controlled();
    file_order_sink_job->set_idle_timeout( file_order_sink_job_idle_timeout_default );

    scheduler->root_folder()->job_folder()->add_job( +file_order_sink_job );

    // Der Scheduler führt Tasks des Jobs scheduler_file_order_sink in jedem Scheduler-Schritt aus,
    // damit sich die Aufträge nicht stauen (Der interne Job läuft nicht in einem eigenen Prozess)
    // Siehe Task_subsystem::step().
}

//------------------------------------------------------------------new_directory_file_order_source

ptr<Directory_file_order_source_interface> new_directory_file_order_source( Job_chain* job_chain, const xml::Element_ptr& element )
{
    ptr<Directory_file_order_source> result = Z_NEW( Directory_file_order_source( job_chain, element ) );
    return +result;
}

//-----------------------------------------Directory_file_order_source::Directory_file_order_source

Directory_file_order_source::Directory_file_order_source( Job_chain* job_chain, const xml::Element_ptr& element )
:
    //Idispatch_implementation( &class_descriptor ),
    Directory_file_order_source_interface( job_chain, type_directory_file_order_source ),
    _zero_(this+1),
    _delay_after_error(delay_after_error_default),
    _repeat(directory_file_order_source_repeat_default),
    _alert_when_directory_missing(alert_when_directory_missing_default),
    _directory_read_result_call(Z_NEW(Directory_read_result_call(this))),
    _process_class_path(job_chain->file_watching_process_class_path())
{
    if (_process_class_path != "") {
        add_requisite(Requisite_path(spooler()->process_class_subsystem(), _process_class_path));
    }
    _path = subst_env( element.getAttribute( "directory" ) );

    _regex_string = element.getAttribute( "regex" );
    if( _regex_string != "" )
    {
        _regex.compile( _regex_string );
    }

    _delay_after_error = Duration(element.int_getAttribute( "delay_after_error", int_cast(_delay_after_error.seconds())));

    if( element.getAttribute( "repeat" ) == "no" )  _repeat = Duration::eternal;
                                              else  _repeat = Duration(element.int_getAttribute( "repeat", int_cast(_repeat.seconds())));

    _next_state = normalized_state( element.getAttribute( "next_state", _next_state.as_string() ) );
    _alert_when_directory_missing = element.bool_getAttribute( "alert_when_directory_missing", _alert_when_directory_missing );
}

//----------------------------------------Directory_file_order_source::~Directory_file_order_source

Directory_file_order_source::~Directory_file_order_source()
{
    //if( _spooler->_connection_manager )  _spooler->_connection_manager->remove_operation( this );

    close();
}

//---------------------------------------------------------------Directory_file_order_source::close

void Directory_file_order_source::close()
{
    if (_process_class_path != "") {
        remove_requisite(Requisite_path(spooler()->process_class_subsystem(), _process_class_path));
    }
    stop_watching();
    close_notification();

    if( _next_node ) 
    {
        _next_node->unregister_order_source( this );
        _next_node = NULL;
    }

    _job_chain = NULL;   // close() wird von ~Job_chain gerufen, also kann Job_chain ungültig sein
    _is_active = false;
}

//-------------------------------------------------------------xml::Element_ptr Node::xml

xml::Element_ptr Directory_file_order_source::dom_element( const xml::Document_ptr& document, const Show_what& show )
{
    xml::Element_ptr element = document.createElement( "file_order_source" );

                                             element.setAttribute         ( "directory" , _path );
                                             element.setAttribute_optional( "regex"     , _regex_string );
        if( _notification_event._signaled )  element.setAttribute         ( "signaled"  , "yes" );
        if( !_next_state.is_missing() )      element.setAttribute         ( "next_state", debug_string_from_variant( _next_state ) );

        if (!delay_after_error().is_eternal())  element.setAttribute( "delay_after_error", delay_after_error().seconds() );
        if (!_repeat.is_eternal())              element.setAttribute( "repeat"           , _repeat.seconds());
        element.setAttribute( "alert_when_directory_missing", _alert_when_directory_missing );
        element.setAttribute_optional("remote_scheduler", _remote_scheduler);
        
        if( _directory_error )  append_error_element( element, _directory_error );

        if( _new_files_index < _new_files.size() )
        {
            xml::Element_ptr files_element = document.createElement( "files" );
            files_element.setAttribute( "snapshot_time", _new_files_time.xml_value() );
            files_element.setAttribute( "count"        , _new_files_count );

            if( show.is_set( show_order_source_files ) )
            {
                for( int i = _new_files_index, j = show._max_orders; i < _new_files.size() && j > 0; i++, j-- )
                {
                    zschimmer::file::File_info* f = _new_files[ i ];
                    if( f )
                    {
                        xml::Element_ptr file_element = document.createElement( "file" );
                        if (time_t t = f->last_write_time_or_zero()) file_element.setAttribute("last_write_time", xml_of_time_t(t));
                        file_element.setAttribute( "path"           , f->path() );

                        files_element.appendChild( file_element );
                    }
                }
            }

            element.appendChild( files_element );
        }

        if( _bad_map.size() > 0 )
        {
            xml::Element_ptr bad_files_element = document.createElement( "bad_files" );
            bad_files_element.setAttribute( "count", (int)_bad_map.size() );

            if( show.is_set( show_order_source_files ) )
            {
                Z_FOR_EACH( Bad_map, _bad_map, it )
                {
                    Bad_entry* bad_entry = it->second;

                    xml::Element_ptr file_element = document.createElement( "file" );
                    file_element.setAttribute( "path", bad_entry->_file_path );
                    append_error_element( file_element, bad_entry->_error );

                    bad_files_element.appendChild( file_element );
                }
            }

            element.appendChild( bad_files_element );
        }

    return element;
}

//---------------------------------------------------Directory_file_order_source::async_state_text_

string Directory_file_order_source::async_state_text_() const
{ 
    S result;
    
    result << "Directory_file_order_source(\"" << _path << "\"";
    if( _regex_string != "" )  result << ",\"" << _regex_string << "\"";
    if( _notification_event.signaled_flag() )  result << ",signaled!";
    result << ")";

    return result;
}

//--------------------------------------Directory_file_order_source::start_or_continue_notification
#ifdef Z_WINDOWS

void Directory_file_order_source::start_or_continue_notification( bool was_notified )
{
    // Windows XP:
    // Ein überwachtes lokales Verzeichnis kann entfernt (rd), aber nicht angelegt (mkdir) werden. Der Name ist gesperrt.
    // Aber ein überwachtes Verzeichnis im Netzwerk kann entfernt und wieder angelegt werden, 
    // ohne dass die Überwachung das mitbekommt. Sie signaliert keine Veränderung im neuen Verzeichnis, ist also nutzlos.
    // Deshalb erneuern wir die Verzeichnisüberwachung, wenn seit _repeat Sekunde kein Signal gekommen ist.

    try {
        if( !_notification_event.handle()  ||  Time::now() >= _notification_event_time + _repeat )
        {
            _notification_event_time = Time::now();

            Z_LOG2( "scheduler.file_order", "FindFirstChangeNotification( \"" << _path.path() << "\", FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );\n" );
            HANDLE h = FindFirstChangeNotification( _path.path().c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );
            if (h == INVALID_HANDLE_VALUE) {
                if (_alert_when_directory_missing)
                    z::throw_mswin( "FindFirstChangeNotification", _path.path() );
            } else {
                if( _notification_event.handle() )      // Signal retten. Eigentlich überflüssig, weil wir hiernach sowieso das Verzeichnis lesen
                {
                    _notification_event.wait( 0 );
                    if( _notification_event.signaled() )      
                    {
                        _notification_event.set_signaled();     
                        Z_LOG2( "scheduler.file_order", Z_FUNCTION << " Old directory watchers signal has been transfered to new watcher.\n" );
                    }

                    close_notification();
                }

                _notification_event.set_handle( h );
                _notification_event.set_name( "FindFirstChangeNotification " + _path );
        
                add_to_event_manager( _spooler->_connection_manager );
            }
        }
        else
        if( was_notified )
        {
            _notification_event_time = Time::now();

            Z_LOG2( "scheduler.file_order", "FindNextChangeNotification(\"" << _path << "\")\n" );
            BOOL ok = FindNextChangeNotification( _notification_event.handle() );
            if( !ok )  throw_mswin_error( "FindNextChangeNotification" );
        }
    } 
    catch (exception &x) {
        log()->debug(message_string("SCHEDULER-724", _path, x.what()));
    }
}

#endif
//--------------------------------------------------Directory_file_order_source::close_notification

void Directory_file_order_source::close_notification()
{
#   ifdef Z_WINDOWS
        if( _notification_event.handle() )
        {
            remove_from_event_manager();
            set_async_manager( _spooler->_connection_manager );   // remove_from_event_manager() für set_async_next_gmtime() rückgängig machen

            Z_LOG2( "scheduler.file_order", "FindCloseChangeNotification(\"" << _path << "\")\n" );
            FindCloseChangeNotification( _notification_event.handle() );
            _notification_event._handle = NULL;   // set_handle() ruft CloseHandle(), das wäre nicht gut
        }
#   endif
}

//----------------------------------------------------------Directory_file_order_source::initialize

void Directory_file_order_source::initialize()
{
    Order_source::initialize();     // Setzt _next_order_queue
    assert( _next_node );

    _next_node->register_order_source( this );
}

//-----------------------------------------------------------Directory_file_order_source::activaate

void Directory_file_order_source::activate()
{
    if (Job_node* job_node = Job_node::try_cast(_next_node)) {
        if (job_node->normalized_job_path() == file_order_sink_job_path)  z::throw_xc("SCHEDULER-342", _job_chain->obj_name());
    }
    set_async_next_gmtime( double_time_max );
    set_async_manager( _spooler->_connection_manager );
    _is_active = true;
    try_start_watching();
}

//-------------------------------------------------------Directory_file_order_source::request_order

bool Directory_file_order_source::request_order( const string& cause )
{
    Z_LOG2("scheduler.file_order", Z_FUNCTION << " cause=" << cause << "\n");
    if (!_is_watching)
        return false;
    else
    if (has_new_file())
        return true;
    else {
        if( _expecting_request_order 
         || async_next_gmtime_reached() )       // 2007-01-09 nicht länger: Das, weil die Jobs bei jeder Gelegenheit do_something() durchlaufen, auch wenn nichts anliegt (z.B. bei TCP-Verkehr)
        {
            Z_LOG2("scheduler.file_order", Z_FUNCTION << " async_wake()\n");
            async_wake();   // Veranlasst Aufruf von async_continue_()

            _expecting_request_order = false;
        }
        else
        {
            //Z_LOG2( "scheduler.file_order", Z_FUNCTION << " cause=" << cause << ", !async_next_gmtime_reached()\n" );
        }
        return false;
    }
}

//----------------------------------------------Directory_file_order_source::withdraw_order_request

void Directory_file_order_source::withdraw_order_request()
{
    Z_LOGI2( "zschimmer", Z_FUNCTION << " " << _path << "\n" );

    _expecting_request_order = true;
    set_async_next_gmtime( double_time_max );
}


void Directory_file_order_source::start_read_new_files_from_agent() {
    if (_in_directory_read_result_call) {
        Z_LOG2("scheduler", Z_FUNCTION << " Still waiting for agent's response\n");
    } else {
        Z_LOG2("scheduler.file_order", Z_FUNCTION << "\n");
        _agent_request_known_files.clear();
        if (_job_chain->is_distributed()) {
            read_known_orders(&_agent_request_known_files);
        } else {
            Z_FOR_EACH_CONST(Job_chain::Order_map, _job_chain->_order_map, i) _agent_request_known_files.insert(i->first);
        }
        String_set blacklisted_files;
        get_blacklisted_files(&blacklisted_files);
        Z_FOR_EACH_CONST(String_set, blacklisted_files, i) _agent_request_known_files.erase(*i);
        ListJ list;
        list = ArrayListJ::new_instance(int_cast(_agent_request_known_files.size()));
        Z_FOR_EACH_CONST(String_set, _agent_request_known_files, i) list.add((StringJ)*i);
        _fileOrderSourceClientJ.readFiles(list, _directory_read_result_call->java_sister());
        _in_directory_read_result_call = true;
    }
}


void Directory_file_order_source::on_call(const Directory_read_result_call& call) {
    Z_LOG2("scheduler.file_order", Z_FUNCTION << "\n");
    assert(&call == _directory_read_result_call);
    assert(_is_watching);
    if (_spooler->state() != Spooler::s_stopping) {
        Duration next_call_after = Duration(0.1);  // Little delay to prevent hot loop in case of error
        _in_directory_read_result_call = false;
        clear_new_files();
        bool replaced = check_and_handle_process_class_replacement();
        if (!replaced) {
            try {
                ListJ javaList = (ListJ)((TryJ)call.value()).get();   // Try.get() wirft Exception, wenn call.value() ein Failure ist
                int n = javaList.size();
                if (n == 0) {
                    next_call_after = _new_files_time + _repeat - Time::now();
                    if (next_call_after > Duration(1)) {
                        // Normally, Agent call should response after _repeat, so next_call_after should be zero.
                        Z_LOG2("scheduler", obj_name() << " No files received. Next call after " + next_call_after.as_string());
                    }
                }
                _new_files.reserve(n);
                for (int i = 0; i < n; i++) {
                    string path = (javaproxy::java::lang::String)javaList.get(i);
                    ptr<file::File_info> file_info = Z_NEW(file::File_info);
                    file_info->set_path(path);
                    //file_info->set_last_write_time(???);
                    _new_files.push_back(file_info);
                    _new_files_count++;
                }
                clean_up_blacklisted_files();
                _agent_request_known_files.clear();
                on_directory_read();
                _directory_error = NULL;
            } catch (exception& x) {
                _directory_error = x;
                log()->error(x.what());
            }
            _new_files_time  = Time::now();
        }
        repeat_after_delay(next_call_after);
    }
}

//------------------------------------------------------Directory_file_order_source::read_directory

void Directory_file_order_source::read_directory(bool was_notified)
{
    Z_LOG2("scheduler.file_order", Z_FUNCTION << "\n");
    for( int try_index = 1;; try_index++ )           // Nach einem Fehler machen wir einen zweiten Versuch, bevor wir eine eMail schicken
    {
        try
        {
#           ifdef Z_WINDOWS
                // Verzeichnisüberwachung starten oder fortsetzen,
                // bevor die Dateinamen gelesen werden, damit Änderungen während oder kurz nach dem Lesen bemerkt werden.
                // Das kann ein Ereignis zu viel geben. Aber besser als eins zu wenig.

                start_or_continue_notification( was_notified );
#           endif


            if( _new_files_index == _new_files.size() )     // Keine Dateien mehr im Puffer?
            {
                //read_new_files_and_handle_deleted_files( cause );
                read_new_files();
                clean_up_blacklisted_files();
                while( _new_files_index < _new_files.size()  &&  _new_files[ _new_files_index ] == NULL )  _new_files_index++;
            }

            if( _directory_error )
            {
                log()->info( message_string( "SCHEDULER-984", _path ) );

                if( _alert_when_directory_missing && _send_recovered_mail )
                {
                    _send_recovered_mail = false;
                    send_mail( evt_file_order_source_recovered, NULL );
                }

                _directory_error = NULL;
            }
        }
        catch( exception& x )
        {
            clear_new_files();

            if( _directory_error )
            {
                log()->debug( x.what() );      // Nur beim ersten Mal eine Warnung
            }
            else
            {
                if ( _alert_when_directory_missing )
                {
                    log()->warn( x.what() ); 

                    if( _spooler->_mail_on_error )
                    {
                        _send_recovered_mail = true; 
                        send_mail( evt_file_order_source_error, &x );
                    }
                }
                else
                {
                    log()->info( x.what() );
                }
            }

            _directory_error = x;

            close_notification();  // Schließen, sonst kann ein entferntes Verzeichnis nicht wieder angelegt werden (Windows blockiert den Namen)
        }

        break;
    }

    //_expecting_request_order = false;
}

//----------------------------------------------Directory_file_order_source::fetch_and_occupy_order

Order* Directory_file_order_source::fetch_and_occupy_order(const Order::State& fetching_state, Task* occupying_task, const Time& now, const string& cause)
{
    Order* result = NULL;

    String_set  known_orders;
    bool        known_orders_has_been_read = false;


    if (_is_watching) {
        while( !result  &&  _new_files_index < _new_files.size() )
        {
            if( zschimmer::file::File_info* new_file = _new_files[ _new_files_index ] )
            {
                File_path  path = new_file->path();
                ptr<Order> order;
                bool       was_in_job_chain = false;

                try
                {
                    if ((_remote_scheduler != "" || path.file_exists()) && !_job_chain->order_or_null(path))
                    {
                        if( !known_orders_has_been_read )   // Eine Optimierung: Damit try_place_in_job_chain() nicht bei jeder Datei feststellen muss, dass es bereits einen Auftrag in der Datenbank gibt.
                        {
                            if( _job_chain->is_distributed() )  read_known_orders( &known_orders );
                            known_orders_has_been_read = true;
                        }

                        order = _spooler->standing_order_subsystem()->new_order();

                        order->set_file_path(path, _remote_scheduler);
                        order->set_state(fetching_state);

                        bool ok = true;

                        if( ok )  ok = known_orders.find( order->string_id() ) == known_orders.end();     // Auftrag ist noch nicht bekannt?

                        if( ok )
                        {
                            was_in_job_chain = order->try_place_in_job_chain( _job_chain );
                            ok &= was_in_job_chain;
                            // !ok ==> Auftrag ist bereits vorhanden
                        }

                        if( ok  &&  order->is_distributed() ) 
                        {
                            ok = order->db_occupy_for_processing();

                            if (_remote_scheduler == "" && !path.file_exists())
                            {
                                // Ein anderer Scheduler hat vielleicht den Dateiauftrag blitzschnell erledigt
                                order->db_release_occupation(); 
                                ok = false;
                            }
                        }

                        if( ok )  order->occupy_for_task( occupying_task, now );

                        if( ok  &&  order->is_distributed() )  _job_chain->add_order( order );

                        if( ok )
                        {
                            result = order;
                            string written_at;
                            if (time_t t = new_file->last_write_time_or_zero()) written_at = "written at " + Time(t, Time::is_utc).as_string(_spooler->_time_zone_name, time::without_ms);
                            log()->info(message_string("SCHEDULER-983", order->obj_name(), written_at));
                        }

                        if( !ok )  
                        {
                            if( !was_in_job_chain )  order->remove_from_job_chain();
                            order->close();
                            //order->close( was_in_job_chain? Order::cls_dont_remove_from_job_chain : Order::cls_remove_from_job_chain );
                            order = NULL;
                        }
                    }

                    _new_files[ _new_files_index ] = NULL;
                    --_new_files_count;

                    if( _bad_map.find( path ) != _bad_map.end() )   // Zurzeit nicht denkbar, weil es nur zu lange Pfade betrifft
                    {
                        _bad_map.erase( path );
                        log()->info( message_string( "SCHEDULER-347", path ) );
                    }

                }
                catch( exception& x )   // Möglich bei für Order.id zu langem Pfad
                {
                    if( _bad_map.find( path ) == _bad_map.end() )
                    {
                        log()->error( x.what() );
                        z::Xc xx ( "SCHEDULER-346", path );
                        log()->warn( xx.what() );
                        _bad_map[ path ] = Z_NEW( Bad_entry( path, x ) );

                        xx.append_text( x.what() );
                        send_mail( evt_file_order_error, &xx );
                    }

                    if( order ) 
                    {
                        if( !was_in_job_chain )  order->remove_from_job_chain();
                        order->close();
                        //order->close( was_in_job_chain? Order::cls_dont_remove_from_job_chain : Order::cls_remove_from_job_chain );
                    }
                }
            }

            _new_files_index++;
        }

        if (!_remote_scheduler.empty()) {
            _expecting_request_order = true;
        }
    }
    return result;
}

//---------------------------------------------------Directory_file_order_source::read_known_orders

void Directory_file_order_source::read_known_orders( String_set* known_orders )
{
    S select_sql;
    select_sql << "select `id`  from " << db()->_orders_tablename
               << "  where " << _job_chain->db_where_condition();

    for( Retry_transaction ta ( _spooler->db() ); ta.enter_loop(); ta++ ) try
    {
        known_orders->clear();

        Any_file result_set = ta.open_result_set( select_sql, Z_FUNCTION );
        while( !result_set.eof() )
        {
            Record record = result_set.get_record();
            known_orders->insert( record.as_string( 0 ) );
        }
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", db()->_orders_tablename, x ), Z_FUNCTION ); }
}

//-----------------------------------------------------Directory_file_order_source::clear_new_files

void Directory_file_order_source::clear_new_files()
{
    _new_files.clear();
    _new_files.reserve( 1000 );
    _new_files_index = 0;
    _new_files_count = 0;
}

//------------------------------------------------------Directory_file_order_source::read_new_files

bool Directory_file_order_source::read_new_files()
{
    clear_new_files();
    _new_files_time  = Time::now();

    Z_LOG2( "scheduler.file_order", Z_FUNCTION << "  " << _path << "\n" );
    bool is_first_file = true;

    for( Directory_watcher::Directory_reader dir ( _path, _regex_string == ""? NULL : &_regex );; )
    {
        if( _spooler->_cluster )  _spooler->_cluster->do_a_heart_beat_when_needed( Z_FUNCTION );    // PROVISORISCH FÜR LANGE VERZEICHNISSE AUF ENTFERNTEM RECHNER, macht bei Bedarf einen Herzschlag

        //Z_LOG2( "scheduler.file_order", Z_FUNCTION << "  " << _path << "  " << _new_files.size() << " Dateinamen gelesen\n" );

        ptr<zschimmer::file::File_info> file_info = dir.get();
        if( !file_info )  break;

        bool file_still_exists = file_info->try_call_stat();       // last_write_time füllen für sort, quick_last_write_less()
        if( file_still_exists )
        {
            _new_files.push_back( file_info );
            _new_files_count++;
        }

        if( is_first_file ) 
        {
            is_first_file = false;
            Z_LOG2( "scheduler.file_order", Z_FUNCTION << "  " << file_info->path() << ", erste Datei\n" );
        }
    }

    Z_LOG2( "scheduler.file_order", Z_FUNCTION << "  " << _path << "  " << _new_files.size() << " filenames has been read\n" );

    sort( _new_files.begin(), _new_files.end(), zschimmer::file::File_info::quick_last_write_less );

    return !_new_files.empty();
}

//------------------------------------------Directory_file_order_source::clean_up_blacklisted_files

void Directory_file_order_source::clean_up_blacklisted_files()
{
    Z_LOGI2("scheduler.file_order", Z_FUNCTION << "\n");
    String_set files;
    get_blacklisted_files(&files);
    // Dem Agenten gemeldete bekannte Dateien ignorieren, denn sie sind vom Agenten ignoriert worden und deshalb nicht zurückgemeldet.
    Z_FOR_EACH_CONST(String_set, _agent_request_known_files, i) {
        Z_LOG2("scheduler", Z_FUNCTION << " _blacklisted_files_at_agent_request " << *i << "\n");
        files.erase(*i);
    }
    if( !files.empty() )
    {
        for( int i = 0; i < _new_files.size(); i++ )
        {
            if( zschimmer::file::File_info* new_file = _new_files[ i ] )
                files.erase( new_file->path() );
        }

        Z_FOR_EACH(String_set, files, it)   // Removed blacklisted file
        {
            string path = *it;

            try
            {
                if( ptr<Order> order = _job_chain->order_or_null( path ) )  // Kein Datenbankzugriff 
                {
                    order->on_blacklisted_file_removed();   // Removes the order
                }
                else
                if( _job_chain->is_distributed() )
                {
                    Transaction ta ( _spooler->db() ); 

                    ptr<Order> order = order_subsystem()->try_load_distributed_order_from_database( &ta, _job_chain->path(), path, Order_subsystem::lo_blacklisted_lock );
                    if( order )
                    {
                        order->log()->info( message_string( "SCHEDULER-981" ) );   // "File has been removed"
                        order->db_delete( Order::update_not_occupied, &ta );
                    }

                    ta.commit( Z_FUNCTION );
                }
            }
            catch( exception& x )
            {
                _log->error( S() << x.what() << ", in " << Z_FUNCTION << ", " << path << "\n" );
            }
        }
    }
}


void Directory_file_order_source::get_blacklisted_files(String_set* result) {
    if (_job_chain->is_distributed()) {
        try {
            *result = _job_chain->db_get_blacklisted_order_id_set( _path, _regex );
        }
        catch( exception& x )  { _log->error( S() << x.what() << ", in " << Z_FUNCTION << ", db_get_blacklisted_order_id_set()\n" ); }
    } else {
        result->clear();
        // Not g++ 4.8.3: result->reserve(_job_chain->_blacklist_map.size());
        Z_FOR_EACH_CONST(Job_chain::Blacklist_map, _job_chain->_blacklist_map, i) result->insert(i->first);
    }
}

//-----------------------------------------------------------Directory_file_order_source::send_mail

void Directory_file_order_source::send_mail( Scheduler_event_type event_code, const exception* x )
{
    try
    {                                   
        switch( event_code )
        {
            case evt_file_order_source_error:
            {
                assert( x );

                Scheduler_event scheduler_event ( event_code, log_error, this );

                scheduler_event.set_message( x->what() );
                scheduler_event.set_error( *x );
                scheduler_event.mail()->set_from_name( _spooler->name() + ", " + _job_chain->obj_name() );    // "Scheduler host:port -id=xxx Job chain ..."
                scheduler_event.mail()->set_subject( string("ERROR ") + x->what() );

                S body;
                body << Sos_optional_date_time::now().as_string() << "\n";
                body << "\n";
                body << _job_chain->obj_name() << "\n";
                body << "Scheduler -id=" << _spooler->id() << "  host=" << _spooler->_complete_hostname << "\n";
                body << "\n";
                body << "<file_order_source directory=\"" << _path << "\"/> doesn't work because of following error:\n";
                body << x->what() << "\n";
                body << "\n";

                if( !delay_after_error().is_eternal() )
                {
                    body << "Retrying every " << delay_after_error() << " seconds.\n";
                    body << "You will be notified when the directory is accessible again\n";
                }

                scheduler_event.mail()->set_body( body );
                scheduler_event.send_mail( _spooler->_mail_defaults );

                break;
            }

            case evt_file_order_source_recovered:
            {
                string msg = message_string( "SCHEDULER-984", _path );
                Scheduler_event scheduler_event ( event_code, log_info, this );

                scheduler_event.set_message( msg );
                scheduler_event.mail()->set_from_name( _spooler->name() + ", " + _job_chain->obj_name() );    // "Scheduler host:port -id=xxx Job chain ..."
                scheduler_event.mail()->set_subject( msg );

                S body;
                body << Sos_optional_date_time::now().as_string() << "\n\n" << _job_chain->obj_name() << "\n";
                body << "Scheduler -id=" << _spooler->id() << "  host=" << _spooler->_complete_hostname << "\n\n";
                body << msg << "\n";

                scheduler_event.mail()->set_body( body );
                scheduler_event.send_mail( _spooler->_mail_defaults );

                break;
            }

            case evt_file_order_error:
            {
                string msg = x->what();
                Scheduler_event scheduler_event ( event_code, log_info, this );

                scheduler_event.set_message( msg );
                scheduler_event.set_error( *x );
                scheduler_event.mail()->set_from_name( _spooler->name() + ", " + _job_chain->obj_name() );    // "Scheduler host:port -id=xxx Job chain ..."
                scheduler_event.mail()->set_subject( msg );

                S body;
                body << Sos_optional_date_time::now().as_string() << "\n\n" << _job_chain->obj_name() << "\n";
                body << "Scheduler -id=" << _spooler->id() << "  host=" << _spooler->_complete_hostname << "\n\n";
                body << msg << "\n";

                scheduler_event.mail()->set_body( body );
                scheduler_event.send_mail( _spooler->_mail_defaults );

                break;
            }

            default:
                assert(0), z::throw_xc( Z_FUNCTION );
        }
    }
    catch( const exception& x )  { log()->warn( x.what() ); }
}

//-----------------------------------------------------Directory_file_order_source::async_continue_

bool Directory_file_order_source::async_continue_( Async_operation::Continue_flags flags )
{
    _last_continue = Time::now();
    Z_LOG2("scheduler.file_order", Z_FUNCTION << "\n");
    if (!_is_watching) {
        return false;
    } else {
        check_and_handle_process_class_replacement();
        if (_remote_scheduler != "") {
            start_read_new_files_from_agent();
        } else {
            bool was_notified = _notification_event.signaled_flag();
            _notification_event.reset();
            read_directory(was_notified);
            on_directory_read();
            repeat_after_delay(_repeat);
        }
        return true;
    }
}


void Directory_file_order_source::on_directory_read() {
    if (_job_chain->untouched_is_allowed() && has_new_file()) {
        _job_chain->tip_for_new_order(_next_state);
    }
}


void Directory_file_order_source::repeat_after_delay(const Duration& duration) {
    assert(_is_watching);
    double delay = (double)(_directory_error        ? delay_after_error().seconds() :
                            _expecting_request_order? INT_MAX                 // Nächstes request_order() abwarten
                                                    : duration.seconds());    // Unter Unix (C++) funktioniert's _nur_ durch wiederkehrendes Nachsehen
    Duration lower_bound = (_last_continue + minimum_delay) - Time::now();
    delay = max(lower_bound.as_double(), delay);
    set_async_delay(delay);
    //Z_LOG2( "scheduler.file_order", Z_FUNCTION  << " set_async_delay(" << delay << ")  _expecting_request_order=" << _expecting_request_order << 
    //          "   async_next_gmtime" << Time( async_next_gmtime() ).as_string() << "GMT \n" );
}


bool Directory_file_order_source::has_new_file() {
    // read_known_orders, also Lesen der Datenbank, ist nicht aktiv, weil request_order und dmait has_new_file sehr oft aufgerufen wird. Jedenfalls mit dem alten (<2013) Mikroscheduling
    // Bei verteilter Jobkette wird das Problem JS-1354 (hochzählen der nächsten Task-ID in der Datenbank) weiterhin bestehen.
    //String_set known_orders;
    //bool known_orders_has_been_read = false;
    while (_new_files_index < _new_files.size()) {
        if (file::File_info* f = _new_files[_new_files_index]) {
            File_path path = f->path();
            if (_remote_scheduler == "" && !path.exists()) {
                _new_files[_new_files_index] = NULL;  // Clean up the entry for the by now deleted file
                while( _new_files_index < _new_files.size()  &&  _new_files[ _new_files_index ] == NULL )  _new_files_index++;
                continue;
            } else
            if (!_job_chain->order_id_space_contains_order_id(path)) {
                //if (!_job_chain->is_distributed()) {
                    return true;
                //} else {
                //    if (!known_orders_has_been_read) {
                //        read_known_orders(&known_orders); 
                //        known_orders_has_been_read = true;
                //    }
                //    return known_orders.find(path) == known_orders.end();
                //}
            }
        }
        _new_files_index++;
    }
    return false;
}

//---------------------------------------------------Directory_file_order_source::delay_after_error

Duration Directory_file_order_source::delay_after_error()
{
    return !_delay_after_error.is_eternal()? _delay_after_error : _repeat;
}

//------------------------------------------------------------Directory_file_order_source::obj_name

string Directory_file_order_source::obj_name() const
{
    S result;

    result << "Directory_file_order_source(\"" << _path << "\",\"" << _regex_string << "\")";

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace order
} //namespace spoooler
} //namespace sos
