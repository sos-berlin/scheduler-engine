// $Id: spooler_module.h,v 1.30 2003/08/11 19:33:11 jz Exp $

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

const string spooler_init_name;
const string spooler_open_name;
const string spooler_close_name;
const string spooler_process_name;
const string spooler_on_error_name;
const string spooler_on_success_name;

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
        reuse_global
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

                                Module                      ( Spooler* sp, Prefix_log* log )        : _zero_(this+1), _spooler(sp), _log(log) {}
    explicit                    Module                      ( Spooler* sp, const xml::Element_ptr& e, const Time& xml_mod_time, const string& include_path )  : _zero_(this+1), _spooler(sp) { set_dom(e,xml_mod_time,include_path); }
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
};

//----------------------------------------------------------------------------------Module_instance
// Oberklasse

struct Module_instance : Object 
{
    struct In_call
    {
                                In_call                     ( Task* task, const string& name, const string& extra = "" );
                              //In_call                     ( Job* job  , const string& name );
                               ~In_call                     ();

        void                    set_result                  ( bool result )                         { _result = result; _result_set = true; }

      //Job*                   _job;
        Task*                  _task;
        Log_indent             _log_indent;
        string                 _name;                       // Fürs Log
        bool                   _result_set;
        bool                   _result;
    };


    struct Object_list_entry
    {
        ptr<IDispatch>         _object;
        string                 _name;
    };

    typedef list<Object_list_entry>  Object_list;




    Z_GNU_ONLY(                 Module_instance             ();  )                                  // Für gcc 3.2. Nicht implementiert.
                                Module_instance             ( Module* module )                      : _zero_(this+1), _module(module), _log(module?module->_log:NULL) {}
    virtual                    ~Module_instance             ()                                      {}      // Für gcc 3.2

    void                    set_title                       ( const string& title )                 { _title = title; }
    void                    set_in_call                     ( const string& name, const string& extra = "" );

    virtual void                close                       ()                                      = 0;
    virtual void                init                        ();

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

/*
    virtual void                begin__start                ( const Object_list& );
    virtual bool                begin__end                  ();

    virtual void                end__start                  ();
    virtual void                end__end                    ();

    virtual void                step__start                 ();
    virtual bool                step__end                   ();

    virtual bool                operation_finished          ()                                      { return true; }
    virtual void                process                     ( bool wait = false )                   {}
*/

    Fill_zero                  _zero_;
    Delegated_log              _log;
    ptr<Module>                _module;
    string                     _title;                      // Wird lokalem Objectserver als -title=... übergeben, für die Prozessliste (ps)
    int                        _pid;                        // Wird von Remote_module_instance_proxy gesetzt

    Task*                      _task;
    ptr<Com_context>           _com_context;
    ptr<IDispatch>             _idispatch;
    map<string,bool>           _names;
    bool                       _spooler_exit_called;

    ptr<Com_task>              _com_task;                   // spooler_task
    ptr<Com_log>               _com_log;                    // spooler_log
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
