// $Id: spooler_module.h,v 1.3 2002/11/06 06:30:48 jz Exp $

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


struct Module_instance;

//--------------------------------------------------------------------------------------Source_part

struct Source_part
{
                                Source_part                 ( int linenr, const string& text )      : _linenr(linenr), _text(text) {}

                                operator string             () const                                { return _text; }

    int                        _linenr;
    string                     _text;
};

//-----------------------------------------------------------------------------Source_with_includes

struct Source_with_parts
{
    void                        add                         ( int linenr, const string& text )      { _parts.push_back( Source_part( linenr, text ) ); }
    bool                        empty                       ()                                      { return _parts.empty(); }
    void                        clear                       ()                                      { _parts.clear(); }

    string                      text                        () const                                { return zschimmer::join( SYSTEM_NL, _parts ); }
                                operator string             () const                                { return text(); }

    typedef list<Source_part>   Parts;

    Parts                      _parts;
};

//-------------------------------------------------------------------------------------------Module

struct Module
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
        kind_com,
        kind_scripting_engine,
        kind_java,
        kind_perl
    };

                                Module                      ( Spooler* sp, Prefix_log* log )         : _spooler(sp), _log(log) {}
    explicit                    Module                      ( Spooler* sp, const xml::Element_ptr& e, const string& include_path )  : _spooler(sp) { set_xml(e,include_path); }
                               ~Module                      ()                                      {}

    void                        set_xml                     ( const xml::Element_ptr& e, const string& include_path )  { set_xml_without_source(e); set_xml_source_only(e,include_path); }
    void                        set_xml_without_source      ( const xml::Element_ptr& );
    void                        set_xml_source_only         ( const xml::Element_ptr&, const string& include_path );

    ptr<Module_instance>        create_instance             ();

    bool                        set                         ()                                      { return _set; }
  //void                        clear                       ()                                      { _language="", _source.clear(); }

    Kind                        kind                        () const                                { return _kind; }

    jmethodID                   java_method_id              ( const string& name );                 // in spooler_module_java.cxx

    Spooler*                   _spooler;
    Prefix_log*                _log;
    bool                       _set;

    Source_with_parts          _source;
    Reuse                      _reuse;
    Kind                       _kind;


    // Scripting Engine
    string                     _language;                   // <script language="...">
    
    // COM
    string                     _com_class_name;             // <script com_class="...">
    string                     _filename;                   // <script filename="...">

    // Java
    string                     _java_class_name;            // <script java_class="...">
  //string                     _java_class_path;            // <script java_class_path="..">

    jclass                     _java_class;
    typedef map<string,jmethodID>  Method_map;
    Method_map                 _method_map;
};

//----------------------------------------------------------------------------------Module_instance
// Oberklasse

struct Module_instance : Object 
{
                                Module_instance             ( Module* script )                      : _zero_(this+1), _module(script), _log(script->_log) {}

    virtual void                init                        ();
    virtual void                load                        ()                                      {}
    virtual void                start                       ()                                      {}
    virtual IDispatch*          dispatch                    () const                                { throw_xc( "SPOOLER-172", "dispatch()" ); }
    virtual void                add_obj                     ( const ptr<IDispatch>&, const string& name );
    virtual void                close                       ()                                      = 0;
    Variant                     call_if_exists              ( const string& name );
    virtual Variant             call                        ( const string& name )                  = 0;
    virtual Variant             call                        ( const string& name, int param )       = 0;
    virtual bool                name_exists                 ( const string& name )                  = 0;
    bool                        loaded                      ()                                      { return _loaded; }
    virtual bool                callable                    ()                                      = 0;


    Fill_zero                  _zero_;
    Prefix_log*                _log;
    Module*                    _module;

    ptr<Com_context>           _com_context;
    ptr<IDispatch>             _idispatch;
    bool                       _loaded;
    map<string,bool>           _names;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
