// $Id: spooler_module.h,v 1.38 2003/08/30 22:40:27 jz Exp $

#ifndef __SPOOLER_MODULE_H
#define __SPOOLER_MODULE_H

#include <jni.h>

namespace sos {
namespace spooler {

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

struct Module_instance;

//--------------------------------------------------------------------------------------Source_part

struct Source_part
{
                                Source_part                 ( int linenr, const string& text, const Time& mod_time )      : _linenr(linenr), _text(text), _modification_time(mod_time) {}

                                operator string             () const                                { return _text; }
    xml::Element_ptr            dom                         ( const xml::Document_ptr& ) const;


    int                        _linenr;
    string                     _text;
    Time                       _modification_time;
};

//-----------------------------------------------------------------------------Source_with_includes

struct Source_with_parts
{
                                Source_with_parts           ()                                      {}
                                Source_with_parts           ( const string& text )                  { assign( text); }
                                Source_with_parts           ( const xml::Element_ptr& dom )         { assign_dom(dom); }

    void                        add                         ( int linenr, const string& text, const Time& mod_time );
    bool                        empty                       ()                                      { return _parts.empty(); }
    void                        clear                       ()                                      { _parts.clear(); }

    string                      text                        () const                                { return zschimmer::join( SYSTEM_NL, _parts ); }
                                operator string             () const                                { return text(); }

    xml::Document_ptr           dom_doc                     () const;
    xml::Element_ptr            dom                         ( const xml::Document_ptr& ) const;

    Source_with_parts&          operator =                  ( const string& text )                  { assign( text );  return *this; }
    void                        assign                      ( const string& text )                  { clear(); add( 1, text, Time(0) ); }

    Source_with_parts&          operator =                  ( const xml::Element_ptr& dom )         { assign_dom( dom );  return *this; }

    void                        assign_dom                  ( const xml::Element_ptr& dom );


    typedef list<Source_part>   Parts;

    Parts                      _parts;
    Time                       _max_modification_time;
};

//-------------------------------------------------------------------------------------------Module

struct Module : Object
{
    enum Reuse
    {
        reuse_task,
        reuse_job,
      //reuse_global
    };

    enum Kind
    {
        kind_none,
        kind_java,
      //kind_perl,
        kind_scripting_engine,
        kind_com,
        kind_remote
    };

    Z_GNU_ONLY(                 Module                      (); )

                                Module                      ( Spooler* sp, Prefix_log* log )        : _zero_(_end_), _spooler(sp), _log(log) {}
    explicit                    Module                      ( Spooler* sp, const xml::Element_ptr& e, const Time& xml_mod_time, const string& include_path )  : _zero_(_end_), _spooler(sp) { set_dom(e,xml_mod_time,include_path); }
                               ~Module                      ()                                      {}

    void                        set_dom                     ( const xml::Element_ptr& e, const Time& xml_mod_time, const string& include_path )  { set_dom_without_source(e); set_dom_source_only(e,xml_mod_time,include_path); }
    void                        set_dom_without_source      ( const xml::Element_ptr& );
    void                        set_dom_source_only         ( const xml::Element_ptr&, const Time& xml_mod_time, const string& include_path );
    void                        set_source_only             ( const Source_with_parts& );
    void                        init                        ();

    ptr<Module_instance>        create_instance             ();
    bool                        set                         ()                                      { return _set; }
    Kind                        kind                        () const                                { return _kind; }
    void                        clear_java                  ();
    bool                        make_java_class             ( bool force = false );                 // in spooler_module_java.cxx
    jmethodID                   java_method_id              ( const string& name );                 // in spooler_module_java.cxx


    Fill_zero                  _zero_;
    Spooler*                   _spooler;
  //Prefix_log*                _log;
    Delegated_log              _log;
    bool                       _set;

    Source_with_parts          _source;
    Reuse                      _reuse;
    bool                       _separate_process;           // Das Skript soll einem getrennten, eigenen Prozess laufen
    string                     _process_class_name;
    bool                       _use_process_class;
    Kind                       _kind;


    // Scripting Engine
    string                     _language;                   // <script language="...">
    
    // COM
    string                     _com_class_name;             // <script com_class="...">
    string                     _filename;                   // <script filename="...">

    // Java
    ptr<java::Vm>              _java_vm;
    string                     _java_class_name;            // <script java_class="...">
    bool                       _recompile;                  // <script recompile="..">    Immer kompilieren

    jclass                     _java_class;
    typedef map<string,jmethodID>  Method_map;
    Method_map                 _method_map;

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
                                Object_list_entry           ( const ptr<IDispatch>& object, const string& name ) : _object(object), _name(name) {}

        ptr<IDispatch>         _object;
        string                 _name;
    };

    typedef list<Object_list_entry>  Object_list;




    Z_GNU_ONLY(                 Module_instance             ();  )                                  // Für gcc 3.2. Nicht implementiert.
                                Module_instance             ( Module* );
    virtual                    ~Module_instance             ()                                      {}      // Für gcc 3.2

  //void                    set_title                       ( const string& title )                 { _title = title; }

    void                        clear                       ()                                      { _object_list.clear(); }
    virtual void                close                       ()                                      = 0;
    virtual void                init                        ();
    void                    set_log                         ( Prefix_log* log )                     { _log = log; }
    void                    set_in_call                     ( In_call* in_call, const string& extra = "" );
    void                    set_close_instance_at_end       ( bool b )                              { _close_instance_at_end = b; }   // Nach spooler_close() Instanz schließen

    void                        attach_task                 ( Task*, Prefix_log* );
    void                        detach_task                 ();
    virtual void                add_obj                     ( const ptr<IDispatch>&, const string& name );
    virtual void                load                        ()                                      {}
    virtual void                start                       ()                                      {}
    virtual IDispatch*          dispatch                    () const                                { throw_xc( "SPOOLER-172", "dispatch()" ); }
    Variant                     call_if_exists              ( const string& name );
    virtual Variant             call                        ( const string& name )                  = 0;
    virtual Variant             call                        ( const string& name, int param )       = 0;
    virtual bool                name_exists                 ( const string& name )                  = 0;
    virtual bool                loaded                      ()                                      = 0;
    virtual bool                callable                    ()                                      = 0;
    int                         pid                         ()                                      { return _pid; }        // 0, wenn kein Prozess

    virtual Async_operation*    begin__start                (); // const Object_list& );
    virtual bool                begin__end                  ();

    virtual Async_operation*    end__start                  ( bool success = true );
    virtual void                end__end                    ();

    virtual Async_operation*    step__start                 ();
    virtual bool                step__end                   ();

    virtual Async_operation*    call__start                 ( const string& method );
    virtual bool                call__end                   ();

  //virtual bool                operation_finished          ()                                      { return true; }
  //virtual void                process                     ( bool wait = false )                   {}


    Fill_zero                  _zero_;

    string                     _title;                      // Wird lokalem Objectserver als -title=... übergeben, für die Prozessliste (ps)
    Spooler*                   _spooler;
    Delegated_log              _log;
    ptr<Module>                _module;
    int                        _pid;                        // Wird von Remote_module_instance_proxy gesetzt

  //Task*                      _task;
    Object_list                _object_list;
    ptr<IDispatch>             _idispatch;
    map<string,bool>           _names;
    bool                       _spooler_init_called;
    bool                       _spooler_exit_called;
    bool                       _spooler_open_called;
    bool                       _close_instance_at_end;
    In_call*                   _in_call;
    string                     _call_method;                // Für Module_instance::call__start()

    ptr<Com_task>              _com_task;                   // spooler_task
    ptr<Com_log>               _com_log;                    // spooler_log

    Fill_end                   _end_;
};

//------------------------------------------------------------------------------Com_module_instance
/*
struct Com_job_instance_factory : IDispatch
{

};
*/
//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
