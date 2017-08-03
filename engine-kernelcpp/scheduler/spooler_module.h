// $Id: spooler_module.h 14213 2011-04-17 10:23:45Z jz $

#ifndef __SPOOLER_MODULE_H
#define __SPOOLER_MODULE_H

#include <jni.h>

#include "Module_monitor.h"
#include "Module_monitors.h"
#include "Module_monitor_instance.h"
#include "Module_monitor_instances.h"

namespace sos {
namespace scheduler {

#ifdef Z_WINDOWS
#   define SPOOLER_DEFAULT_LANGUAGE     "VBScript"
#else
#   define SPOOLER_DEFAULT_LANGUAGE     "Perl"
#endif

//--------------------------------------------------------------------------------------------const

extern const string spooler_init_name;
extern const string spooler_exit_name;
extern const string spooler_open_name;
extern const string spooler_close_name;
extern const string spooler_process_name;
extern const string spooler_on_error_name;
extern const string spooler_on_success_name;

//-------------------------------------------------------------------------------------------------

struct                                  Module_instance;
typedef list< ptr<Module> >             Module_list;
typedef list< ptr<Module_instance> >    Module_instance_list;
struct                                  Module_monitors;
struct                                  Module_monitor_instance;

//-------------------------------------------------------------------------------Text_with_includes

struct Text_with_includes : Non_cloneable
{
                                Text_with_includes          ( Scheduler*, File_based*, const File_path& include_path, const xml::Element_ptr& = xml::Element_ptr() );

    bool                        is_empty                    () const;

    string                      read_xml_string             ()                                      { return read_text(true); }
    string                      read_plain_or_xml_string    ()                                      { return read_text(false); }
    string                      read_plain_text_element     (const xml::Element_ptr& e)             { return read_text_element(e, false); }
    int                         text_element_linenr         ( const xml::Element_ptr& );
    string                      text_element_filepath       ( const xml::Element_ptr& );

    string                      xml_string                  () const                                { return _xml_string; }
    void                    set_xml_string                  ( const string& x )                     { _xml_string = x; }
    xml::Document_ptr           includes_resolved           () const;

    xml::Element_ptr            dom_element                 () const                                { return xml::Document_ptr::from_xml_string(_xml_string).documentElement(); }
    void                        append_dom                  ( const xml::Element_ptr& dom );

  private:
    string                      read_text                   (bool xml_only);
    string                      read_text_element           (const xml::Element_ptr&, bool xml_only);

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    File_based*                _file_based;
    File_path                  _include_path;
    string                     _xml_string;
};

//-------------------------------------------------------------------------------------------Module

struct Module : Object
{
    enum Kind
    {
        kind_none,
        kind_process,
        kind_java,
        kind_java_in_process,
        kind_scripting_engine,
        kind_com,
        kind_remote,            // Nur für Module_instance
        kind_internal,
        kind_scripting_engine_java           // JS-498: Scriptverarbeitung über JAVA interface

    };

    Z_GNU_ONLY(                 Module                      (); )

                                Module                      ( Spooler*, File_based*, const string& include_path, Has_log*, bool is_monitor);
    explicit                    Module                      ( Spooler*, File_based*, const xml::Element_ptr&, const string& include_path, bool is_monitor);
                               ~Module                      ()                                      {}

    void                        set_log                     ( Has_log* log )                        { _log.set_log( log ); }
    void                        set_folder_path             ( const Absolute_path& p )              { _folder_path = p; }
    void                        set_dom                     ( const xml::Element_ptr& );
    void                        set_xml_string_text_with_includes(const string& xml);
    void                        init0                       ();
    void                        init                        ();
    
    void set_injectorJ(jobject injectorJ) {
        _injectorJ = injectorJ;
    }

    ptr<Module_instance>        create_instance             (Process_class*, const string& remote_scheduler, Task* task_or_null);
    virtual ptr<Module_instance> create_instance_impl       (Process_class*, const string& remote_scheduler, Task* task_or_null);
    bool                        set                         ()                                      { return _set; }
    bool                        has_api                     () const;
    Kind                        kind                        () const                                { return _kind; }
    void                        set_checked_attribute       ( string*, const xml::Element_ptr&, const string&, bool modify_allowed = false );
    void                        set_priority                ( const string& );

    bool                        has_source_script           () const                                { return !_text_with_includes.is_empty(); }
    string                      read_source_script          ()                                      { return _text_with_includes.read_plain_or_xml_string(); }

    Fill_zero                  _zero_;
    Spooler* const             _spooler;
    File_based*                _file_based;
    Delegated_log              _log;
    bool                       _set;

    Text_with_includes         _text_with_includes;
    string                     _include_path;
    Absolute_path              _folder_path;
    Kind                       _kind;
    bool                       _initialized;
    bool _is_monitor;


    // Scripting Engine
    string                     _language;                   // <script language="...">
    
    // COM
    string                     _com_class_name;             // <script com_class="...">
    string                     _filename;                   // <script filename="...">

    // Java
    string                     _java_class_name;            // <script java_class="...">
    string                     _java_options;               // Gehört eigentlich nach Job
    string                     _java_class_path;            // JS-540

    // .Net
    string _dotnet_class_name;
    string _dll;

    // Shell script
    ptr<Com_variable_set>      _process_environment;
    string                     _process_shell_variable_prefix;
    bool                       _process_shell_variable_prefix_is_configured;
    string                     _priority;                   // "", "-20" bis "+20" oder "idle", "below_normal" etc.
    string                     _credentials_key;
    bool                       _load_user_profile;

#ifdef Z_WINDOWS
    int                        _encoding_code_page;
#endif

    ptr<Module_monitors>       _monitors;
    ptr<Login>                 _login;
    InjectorJ                  _injectorJ;

    Fill_end                   _end_;
};

//----------------------------------------------------------------------------------Module_instance
// Oberklasse

struct Module_instance : Object 
{
    struct In_call
    {
                                In_call                     ( Module_instance* module_instance, const string& name, const string& extra = "" );
                               ~In_call                     ();

        void                    set_result                  ( bool result )                         { _result = result; _result_set = true; }
        const string&           name                        ()                                      { return _name; }

        Module_instance*       _module_instance;
        Log_indent             _log_indent;
        string                 _name;                       // Fürs Log
        bool                   _result_set;
        bool                   _result;
    };


    struct Object_list_entry
    {
                                Object_list_entry           ()                                      {}
                                Object_list_entry           ( IDispatch* object, const string& name ) : _object(object), _name(name) {}

        ptr<IDispatch>         _object;
        string                 _name;
    };

    typedef list<Object_list_entry>  Object_list;


    Z_GNU_ONLY(                 Module_instance             ();  )                                  // Für gcc 3.2. Nicht implementiert.
                                Module_instance             ( Module* );
    virtual                    ~Module_instance             ();

    void                    set_job_name                    ( const string& );
    void                    set_task_id                     ( int );

    void                        clear                       ();
    virtual void                close                       ();
    void                        close_monitor               ();
    virtual void                init                        ();
    virtual bool                kill                        (int unix_signal)                       { return false; }
    virtual bool                is_remote_host              () const                                { return false; }
    Module::Kind                kind                        () const                                { return _kind; }
    void                    set_log                         ( Prefix_log* );
    void                    set_log                         ( Has_log* );
    void                    set_in_call                     ( In_call* in_call, const string& extra = "" );

    virtual void                attach_task                 ( Task*, Prefix_log* );
    virtual void                detach_task                 ();
    virtual void                add_objs                    (Task* task_or_null);
    virtual void                add_obj                     ( IDispatch*, const string& name );
    IDispatch*                  object                      ( const string& name );
    IDispatch*                  object                      ( const string& name, IDispatch* deflt );
    void                        fill_process_environment    ();

    bool                        implicit_load_and_start     ();
    virtual bool                load                        ();
    virtual void                start                       ();
    virtual IDispatch*          dispatch                    () const                                { z::throw_xc( "SCHEDULER-172", "dispatch()" ); }
    Variant                     call_if_exists              ( const string& name );
    Variant                     call_if_exists              ( const string& name, const Variant& );
    virtual Variant             call                        ( const string& name )                  = 0;
    virtual Variant             call                        ( const string& name, const Variant&, const Variant& = missing_variant )  = 0;
    virtual bool                name_exists                 ( const string& name )                  = 0;
    virtual bool                loaded                      ()                                      = 0;
    virtual bool                callable                    ()                                      = 0;
    int                         pid                         ()                                      { return _pid; }        // 0, wenn kein Prozess
    string                      process_name                () const                                { return ""; }

    virtual bool                try_to_get_process          (const Api_process_configuration* = NULL);
    virtual void                detach_process              ();
    
    bool has_process() const {
        return _process != NULL;
    }

    virtual Async_operation*    close__start                ();
    virtual void                close__end                  ();

    virtual Async_operation*    begin__start                (); // const Object_list& );
    virtual bool                begin__end                  ();

    virtual Async_operation*    end__start                  ( bool success = true );
    virtual void                end__end                    ();

    virtual Async_operation*    step__start                 ();
    virtual Variant             step__end                   ();

    virtual Async_operation*    call__start                 ( const string& method );
    virtual Variant             call__end                   ();

    virtual Async_operation*    release__start              ();
    virtual void                release__end                ();

    virtual void                check_connection_error      ()                                      {}
    void                    set_spooler_process_result      ( bool b )                              { _spooler_process_result = b; }
    bool                        spooler_process_result      () const                                { return _spooler_process_result; }
    int                         exit_code                   ()                                      { return _exit_code; }
    void                    set_exit_code                   ( int exit_code )                       { _exit_code = exit_code; }
    int                         termination_signal          ()                                      { return _termination_signal; }
    void                    set_termination_signal          ( int s )                               { _termination_signal = s; }
    virtual File_path           stdout_path                 ()                                      { return File_path(); }
    virtual File_path           stderr_path                 ()                                      { return File_path(); }
    virtual bool                try_delete_files            ( Has_log* )                            { return true; }
    virtual std::list<File_path> undeleted_files            ()                                      { return std::list<File_path>(); }
    virtual bool                process_has_signaled        ()                                      { return false; }       // Für Process_module_instance
    virtual bool                is_kill_thread_running      ()                                      { return false; }
    virtual string              obj_name                    () const                                { return "Module_instance(" + _job_name + ":" + as_string(_task_id) + ")"; }

    public: Order_state_transition order_state_transition() const;
    
    public: Process_class* process_class() const {
        if (!_process_class_or_null) z::throw_xc(Z_FUNCTION);
        return _process_class_or_null;
    }
    
    public: Process_class* process_class_or_null() const {
        return _process_class_or_null;
    }

    public: void set_process_class(Process_class*);

    public: virtual string remote_scheduler_address() const {
        return "";
    }

    Fill_zero                  _zero_;

    string                     _job_name;                   // Wird lokalem Objectserver als -job=... übergeben, für die Prozessliste (ps)
    Task*                      _task;
    int                        _task_id;                    // Wird lokalem Objectserver als -task-id=... übergeben, für die Prozessliste (ps)
    Spooler*                   _spooler;
    Module::Kind               _kind;
    Delegated_log              _log;
    ptr<Module>                _module;
    ptr<Async_operation>       _sync_operation;
    int                        _pid;                        // Wird von Remote_module_instance_proxy gesetzt
    int                        _exit_code;
    int                        _termination_signal;
    bool                       _initialized;
    bool                       _load_called;

    ptr<Com_variable_set> const _process_environment;
    bool                       _has_order;
    Object_list                _object_list;
    ptr<IDispatch>             _idispatch;
    map<string,bool>           _names;
    bool                       _spooler_init_called;
    bool                       _spooler_exit_called;
    bool                       _spooler_open_called;
    bool                       _spooler_close_called;
    In_call*                   _in_call;
    string                     _call_method;                // Für Module_instance::call__start()
    bool                       _spooler_process_result;

    ptr<Com_task>              _com_task;                   // spooler_task
    ptr<Com_log>               _com_log;                    // spooler_log

    Module_monitor_instances   _monitor_instances;
    protected: string _remote_scheduler_address;   // Wird nur von Remote_module_instance_proxy benutzt, sonst Dummy
    protected: ptr<Process> _process;              // Wird nur von Remote_module_instance_proxy benutzt, sonst Dummy
    private: Process_class* _process_class_or_null;
    private: Fill_end _end_;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
