// $Id: spooler_process.h 13558 2008-05-09 14:11:16Z jz $

#ifndef __SPOOLER_PROCESS_H
#define __SPOOLER_PROCESS_H

#include "../zschimmer/com_remote.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__processclass__ProcessClass.h"

typedef ::javaproxy::com::sos::scheduler::engine::kernel::processclass::ProcessClass ProcessClassJ;

namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------------------

struct Process_class;
struct Process_class_folder;
struct Process_class_subsystem;


struct Process_class_requestor {
    virtual void notify_a_process_is_available() = 0;
    virtual string obj_name() const = 0;
};


struct Api_process_configuration : javabridge::has_proxy<Api_process_configuration> {
    Api_process_configuration() : _zero_(this + 1) {}
    Fill_zero _zero_;
    Host_and_port _controller_address;
    string _remote_scheduler_address;
    string _job_path;
    int _task_id;
    bool _has_api;
    bool _is_thread;
    bool _log_stdout_and_stderr;   // Prozess oder Thread soll stdout und stderr selbst über COM/TCP protokollieren
    string _priority;
    ptr<Com_variable_set> _environment;
    ptr<Login> _login;
    string _java_options;
    string _java_classpath;
    bool _is_shell_dummy;
    string _credentials_key;
    bool _load_user_profile;
};


/** Ein Prozess, in dem ein Module oder eine Task ablaufen kann. Kann auch ein Thread sein. */
struct Process : zschimmer::Object, Scheduler_object {
    using Scheduler_object::obj_name;
    
    Process() :
        _task(NULL) 
    {}
    
    virtual Process_id process_id() const = 0;
    virtual string short_name() const = 0;
    virtual double async_next_gmtime() = 0;
    virtual object_server::Connection* connection() const = 0;
    virtual xml::Element_ptr dom_element(const xml::Document_ptr&, const Show_what&) = 0;
    virtual bool async_continue() = 0;

    public: void attach_task(Task* task) {
        _task = task; 
    }
    
    protected: Task* task() const {
        return _task;
    } 
    
    private: Task* _task;
};


struct Api_process : virtual Process {
    virtual bool is_started() = 0;
    virtual int exit_code() = 0;
    virtual int termination_signal() = 0;
    virtual object_server::Session* session() = 0;
    virtual int pid() const = 0;

    virtual void start() = 0;
    virtual bool kill(int unix_signal) = 0;
    virtual void close_async() = 0;
    virtual Async_operation* close__start(bool run_independently = false) = 0;
    virtual void close__end() = 0;
    virtual void close_session() = 0;
    virtual void check_exception() {}

    virtual string remote_scheduler_address() {
        return "";
    }

    static ptr<Api_process> new_process(Spooler* sp, Prefix_log* log, const Api_process_configuration&);
};


struct Local_api_process : virtual Api_process {
    virtual bool try_delete_files(Has_log*) = 0;
    virtual File_path stderr_path() = 0;
    virtual File_path stdout_path() = 0;
    virtual std::list<file::File_path> undeleted_files() = 0;
};

//----------------------------------------------------------------------Process_class_configuration

struct Process_class_configuration : idispatch_implementation< Process_class, spooler_com::Iprocess_class >,
                                     file_based< Process_class, Process_class_folder, Process_class_subsystem >
{
                                Process_class_configuration ( Scheduler*, const string& name = "" );

    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Idispatch_implementation::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Idispatch_implementation::Release(); }

    virtual void            set_max_processes               ( int );
    int                         max_processes               () const                                { return _max_processes; }
    virtual void          check_max_processes               ( int ) const                           {}

    virtual void            set_remote_scheduler_address    ( const string& );
    const string&               remote_scheduler_address    () const                                { return _remote_scheduler_address; }

    string                      obj_name                    () const;

    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    // spooler_com::Iprocess_class:
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Process_class"; }
    STDMETHODIMP                Remove                      ();
    STDMETHODIMP            put_Name                        ( BSTR );
    STDMETHODIMP            get_Name                        ( BSTR* result )                        { return String_to_bstr( name(), result ); }
    STDMETHODIMP            put_Remote_scheduler            ( BSTR );
    STDMETHODIMP            get_Remote_scheduler            ( BSTR* result )                        { return String_to_bstr( _remote_scheduler_address, result ); }
    STDMETHODIMP            put_Max_processes               ( int );
    STDMETHODIMP            get_Max_processes               ( int* result )                         { *result = _max_processes;  return S_OK; }

  protected:
    protected: virtual bool is_http_or_multiple(const string& remote_scheduler_address) const = 0;

    Fill_zero                  _zero_;

    int                        _max_processes;
    string                     _remote_scheduler_address;
    // Neue Einstellungen in Process_class::set_configuration() berücksichtigen!


    static Class_descriptor     class_descriptor;
    static const Com_method    _methods[];
};

//------------------------------------------------------------------------------------Process_class
// <process_class>

struct Process_class : Process_class_configuration,
                       javabridge::has_proxy<Process_class>
{
    Process_class               ( Scheduler*, const string& name = "" );
    Z_GNU_ONLY(                 Process_class               (); )
                               ~Process_class               ();

    jobject                     java_sister                 ()                                      { return javabridge::has_proxy<Process_class>::java_sister(); }


    // file_based<Process_class>
    void                        close                       ();
    bool                        on_initialize               ();
    bool                        on_load                     ();
    bool                        on_activate                 ();

    bool                        can_be_removed_now          ();

    void                        prepare_to_replace          ();
    bool                        can_be_replaced_now         ();
    Process_class*              on_replace_now              ();
    bool                        is_visible_requisite  () { return path() != ""; }   // default process_class will not shown

    void                    set_configuration               ( const Process_class_configuration& );
    void                  check_max_processes               ( int ) const;
    void                    set_max_processes               ( int );

    void                        add_process                 (Process*);
    void                        remove_process              (Process*);

    void add_file_order_source() {
        _file_order_source_count++;
    }

    void remove_file_order_source();

    Process*                    new_process                 (const Api_process_configuration&, Prefix_log*);
    Process*                    select_process_if_available (const Api_process_configuration&, Prefix_log*);
    bool                        process_available           ( Job* for_job );
    void enqueue_requestor(Process_class_requestor*);
    void remove_requestor(Process_class_requestor*);
    Process_class_requestor* waiting_requestor_or_null();
    void                        check_then_notify_a_process_is_available    ();

    typedef stdext::hash_set< ptr<Process> > Process_set;
    Process_set&                process_set                 () { return _process_set; }
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    void set_dom(const xml::Element_ptr&);

    xml::ElementJ java_dom_element() {
        xml::Document_ptr doc;
        doc.create();
        doc.appendChild(doc.createElement("XXX"));
        return xml::Element_ptr(dom_element(doc, Show_what()));
    }

    public: bool max_processes_reached() const {
        return used_process_count() >= _max_processes;
    }

    public: int used_process_count() const {
        return _process_set.size();
    }

    bool is_remote_host() const {
        return !_remote_scheduler_address.empty() || typed_java_sister().hasMoreAgents();
    }

    const ProcessClassJ& typed_java_sister() const {
        return _typed_java_sister;
    }

    protected: bool is_http_or_multiple(const string& remote_scheduler_address) const;

  private:
    friend struct               Process_class_subsystem;

    Fill_zero                  _zero_;

    typedef list<Process_class_requestor*> Requestor_list;
    Requestor_list _requestor_list;

    Process_set                _process_set;
    int                        _process_set_version;
    ProcessClassJ _typed_java_sister;

    int _file_order_source_count;
};

//-----------------------------------------------------------------------------Process_class_folder

struct Process_class_folder : typed_folder<Process_class>
{
                                Process_class_folder        ( Folder* );
                               ~Process_class_folder        ();


    // Typed_folder:
    bool                        is_empty_name_allowed       () const                                { return true; }

    void                        add_process_class           ( Process_class* process_class )        { add_file_based( process_class ); }
    void                        remove_process_class        ( Process_class* process_class )        { remove_file_based( process_class ); }
    Process_class*              process_class               ( const string& name )                  { return file_based( name ); }
    Process_class*              process_class_or_null       ( const string& name )                  { return file_based_or_null( name ); }
    xml::Element_ptr            new_dom_element             ( const xml::Document_ptr& doc, const Show_what& ) { return doc.createElement( "process_classes" ); }
};

//--------------------------------------------------------------------------Process_class_subsystem

struct Process_class_subsystem : idispatch_implementation< Process_class_subsystem, spooler_com::Iprocess_classes>,
                                 file_based_subsystem< Process_class >,
                                 javabridge::has_proxy<Process_class_subsystem>
{
    static bool is_empty_default_path(const Absolute_path& o) {
        return o.empty();
    }

                                Process_class_subsystem     ( Scheduler* );

    // Subsystem
    void                        close                       ();
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();

    jobject                     java_sister                 ()                                      { return javabridge::has_proxy<Process_class_subsystem>::java_sister(); }

    // file_based_subsystem< Process_class >
    string                      object_type_name            () const                                { return "Process_class"; }
    string                      filename_extension          () const                                { return ".process_class.xml"; }
    string                      xml_element_name            () const                                { return "process_class"; }
    string                      xml_elements_name           () const                                { return "process_classes"; }
    ptr<Process_class>          new_file_based              (const string& source)                  { return Z_NEW( Process_class( spooler() ) ); }
    xml::Element_ptr            new_file_baseds_dom_element ( const xml::Document_ptr& doc, const Show_what& ) { return doc.createElement( "process_classes" ); }

    ptr<Process_class_folder>   new_process_class_folder    ( Folder* folder )                      { return Z_NEW( Process_class_folder( folder ) ); }
    Process_class*              process_class               ( const Absolute_path& path )           { return file_based( path ); }
    Process_class*              process_class_or_null       ( const Absolute_path& path )           { return file_based_or_null( path ); }
    bool                        try_to_free_process         ( Job* for_job, Process_class*, const Time& now );
    bool                        async_continue              ();

  //xml::Element_ptr            execute_xml                 ( Command_processor*, const xml::Element_ptr&, const Show_what& );

    // spooler_com::Iprocess_classes
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Process_classes"; }
    STDMETHODIMP            get_Process_class               ( BSTR, spooler_com::Iprocess_class** );
    STDMETHODIMP            get_Process_class_or_null       ( BSTR, spooler_com::Iprocess_class** );
    STDMETHODIMP                Create_process_class        ( spooler_com::Iprocess_class** );
    STDMETHODIMP                Add_process_class           ( spooler_com::Iprocess_class* );

  private:
    Fill_zero                  _zero_;

  public:
    static Class_descriptor     class_descriptor;
    static const Com_method     _methods[];
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
