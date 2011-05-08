// $Id$

#ifndef __SPOOLER_MODULE_H
#define __SPOOLER_MODULE_H

#include <jni.h>
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
extern const string spooler_api_version_name;

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

    string                      read_text                   ();
    string                      read_text_element           ( const xml::Element_ptr& );
    int                         text_element_linenr         ( const xml::Element_ptr& );
    string                      text_element_filepath       ( const xml::Element_ptr& );

    string                      xml                         ()                                      { return _dom_document.xml(); }
    void                    set_xml                         ( const string& x )                     { _dom_document.load_xml( x ); }
    xml::Document_ptr           includes_resolved           () const;

    xml::Element_ptr            dom_element                 ()                                      { return _dom_document.documentElement(); }
    void                        append_dom                  ( const xml::Element_ptr& dom );

  private:
    void                        initialize                  ();

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    File_based*                _file_based;
    File_path                  _include_path;
    xml::Document_ptr          _dom_document;
};

//-------------------------------------------------------------------------------------------Module

struct Module : Object
{
    enum Reuse
    {
        reuse_task,
        reuse_job
    };

    enum Kind
    {
        kind_none,
        kind_process,
        kind_java,
        kind_scripting_engine,
        kind_com,
        kind_remote,            // Nur für Module_instance
        kind_internal,
        kind_scripting_engine_java           // JS-498: Scriptverarbeitung über JAVA interface

    };

    Z_GNU_ONLY(                 Module                      (); )

                                Module                      ( Spooler*, File_based*, const string& include_path, Has_log* = NULL );
    explicit                    Module                      ( Spooler*, File_based*, const xml::Element_ptr&, const string& include_path );
                               ~Module                      ()                                      {}

    void                        set_log                     ( Has_log* log )                        { _log.set_log( log ); }
    void                        set_folder_path             ( const Absolute_path& p )              { _folder_path = p; }
    void                        set_dom                     ( const xml::Element_ptr& );
    void                        set_xml_text_with_includes  ( const string& xml );
    void                        set_process                 ();                                     // Für <process>
    void                        init0                       ();
    void                        init                        ();

    ptr<Module_instance>        create_instance             ();
    virtual ptr<Module_instance> create_instance_impl       ();
    bool                        set                         ()                                      { return _set; }
    Kind                        kind                        () const                                { return _kind; }
    bool                        make_java_class             ( bool force = false );                 // in spooler_module_java.cxx
    void                        set_checked_attribute       ( string*, const xml::Element_ptr&, const string&, bool modify_allowed = false );
    void                        set_priority                ( const string& );

    bool                        has_source_script           () const                                { return !_text_with_includes.is_empty(); }
    string                      read_source_script          ()                                      { return _text_with_includes.read_text(); }
    bool                        needs_java                  ();

    Process_class*              process_class               () const;
    Process_class*              process_class_or_null       () const;


    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    File_based*                _file_based;
    Delegated_log              _log;
    bool                       _set;

    Text_with_includes         _text_with_includes;
    string                     _include_path;
    Reuse                      _reuse;
    Absolute_path              _folder_path;
    string                     _process_class_string;
    Absolute_path              _process_class_path;
    bool                       _use_process_class;
    Kind                       _kind;
    bool                       _initialized;


    // Scripting Engine
    string                     _language;                   // <script language="...">
    
    // COM
    string                     _com_class_name;             // <script com_class="...">
    string                     _filename;                   // <script filename="...">

    // Java
    ptr<javabridge::Vm>        _java_vm;
    string                     _java_class_name;            // <script java_class="...">
    string                     _java_options;               // Gehört eigentlich nach Job
    bool                       _recompile;                  // <script recompile="..">    Immer kompilieren
    bool                       _compiled;
    string                     _java_class_path;            // JS-540

    // Process
    string                     _process_filename;           // Job ist ein externes Programm
    string                     _process_param_raw;          // Parameter für das Programm, vor der Variablenersetzung
    string                     _process_log_filename;
    bool                       _process_ignore_error;
    bool                       _process_ignore_signal;
    ptr<Com_variable_set>      _process_environment;
    string                     _priority;                   // "", "-20" bis "+20" oder "idle", "below_normal" etc.

#ifdef Z_WINDOWS
    int                        _encoding_code_page;
#endif

    bool                       _dont_remote;
    ptr<Module_monitors>       _monitors;

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
    virtual bool                kill                        ()                                      { return false; }
    virtual bool                is_remote_host              () const                                { return false; }
    Module::Kind                kind                        () const                                { return _kind; }
    void                    set_log                         ( Prefix_log* );
    void                    set_log                         ( Has_log* );
    void                    set_in_call                     ( In_call* in_call, const string& extra = "" );
    void                    set_close_instance_at_end       ( bool )                                {} // veraltet: _close_instance_at_end = b; }   // Nach spooler_close() Instanz schließen

    virtual void                attach_task                 ( Task*, Prefix_log* );
    virtual void                detach_task                 ();
    virtual void                add_obj                     ( IDispatch*, const string& name );
    IDispatch*                  object                      ( const string& name );
    IDispatch*                  object                      ( const string& name, IDispatch* deflt );
    void                        fill_process_environment    ();

    void                        end_task                    ();

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

    virtual bool                try_to_get_process          ();
    void                        detach_process              ();

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
    int                         spooler_process_result      () const                                { return _spooler_process_result; }
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


    Fill_zero                  _zero_;

    string                     _job_name;                   // Wird lokalem Objectserver als -job=... übergeben, für die Prozessliste (ps)
    Task*                      _task;
    int                        _task_id;                    // Wird lokalem Objectserver als -task-id=... übergeben, für die Prozessliste (ps)
    Spooler*                   _spooler;
    Module::Kind               _kind;
    Delegated_log              _log;
    ptr<Module>                _module;
    int                        _pid;                        // Wird von Remote_module_instance_proxy gesetzt
    int                        _exit_code;
    int                        _termination_signal;
    bool                       _initialized;
    bool                       _load_called;

    ptr<Process>               _process;                    // Wird nur von Remote_instance_module_proxy benutzt, sonst Dummy
    ptr<Com_variable_set>      _process_environment;
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
    bool                       _spooler_process_result;     // Bisher nur für Process_module / Process_module_instance

    ptr<Com_task>              _com_task;                   // spooler_task
    ptr<Com_log>               _com_log;                    // spooler_log

    Module_monitor_instances   _monitor_instances;

    Fill_end                   _end_;
};

//-----------------------------------------------------------------------------------Module_monitor

struct Module_monitor : Object
{
    static bool                 less_ordering               ( const Module_monitor* a, const Module_monitor* b )  { return a->_ordering < b->_ordering; }


                                Module_monitor              ()                                      : _zero_(this+1), _ordering(1) {}

    string                      name                        () const                                { return _name; }
    string                      obj_name                    () const                                { return S() << "Script_monitor " << name(); }

    Fill_zero                  _zero_;
    string                     _name;
    int                        _ordering;
    ptr<Module>                _module;
};

//----------------------------------------------------------------------------------Module_monitors

struct Module_monitors : Object
{
                                Module_monitors             ( Module* module )                      : _zero_(this+1), _main_module(module) {}

    void                        close                       ();
    void                    set_dom                         ( const xml::Element_ptr& );
    void                        initialize                  ();
    void                        add_monitor                 ( Module_monitor* monitor )             { _monitor_map[ monitor->name() ] = monitor; }
    Module_monitor*             monitor_or_null             ( const string& );
    bool                        is_empty                    () const                                { return _monitor_map.empty(); }
    bool                        needs_java                  ();
    vector<Module_monitor*>     ordered_monitors            ();


    Fill_zero                  _zero_;
    typedef stdext::hash_map< string, ptr<Module_monitor> > Monitor_map;
    Monitor_map                _monitor_map;

  private:
    Module*                    _main_module;
};

//--------------------------------------------------------------------------Module_monitor_instance

struct Module_monitor_instance : Object
{
                                Module_monitor_instance     ( Module_monitor*, Module_instance* );

    string                      obj_name                    () const                                { return _obj_name; }

    ptr<Module_instance>       _module_instance;
    string                     _obj_name;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
