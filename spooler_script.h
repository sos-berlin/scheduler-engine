// $Id: spooler_script.h,v 1.10 2002/06/29 09:49:38 jz Exp $

#ifndef __SPOOLER_SCRIPT_H
#define __SPOOLER_SCRIPT_H

namespace sos {
namespace spooler {

bool                            check_result                ( const CComVariant& vt );

//--------------------------------------------------------------------------------------Source_part

struct Source_part
{
                                Source_part                 ( int linenr, const string& text )      : _linenr(linenr), _text(text) {}

                                operator string             () const                                { return _text; }

    int                        _linenr;
    string                     _text;
};

//inline string&                  operator +=                 ( string& a, const Source_part& b )     { return a += b._text; }

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

//-------------------------------------------------------------------------------------------Script

struct Script
{
    enum Reuse
    {
        reuse_task,
        reuse_job,
        reuse_global
    };

                                Script                      ( Spooler* sp )                 : _spooler(sp) {}
    explicit                    Script                      ( Spooler* sp, const xml::Element_ptr& e, const string& include_path )  : _spooler(sp) { set_xml(e,include_path); }

    void                        set_xml                     ( const xml::Element_ptr&, const string& include_path );

    bool                        empty                       ()                              { return _source.empty(); }
    void                        clear                       ()                              { _language="", _source.clear(); }

    Spooler*                   _spooler;
    string                     _com_class_name;             // com_class=
    string                     _filename;                   // filename=
    string                     _language;                   // language=
    Source_with_parts          _source;
    Reuse                      _reuse;
};

//----------------------------------------------------------------------------------Script_instance

struct Script_instance
{
                                Script_instance             ( Prefix_log* log )            : _zero_(this+1), _log(log) {}
                               ~Script_instance             ();

    void                        init                        ( Script* );
    void                        load                        ();
    void                        start                       ();
  //IDispatch*                  dispatch                    () const                        { return _script_site? _script_site->dispatch() : NULL; }
    IDispatch*                  dispatch                    () const                        { return _idispatch; }
    void                        add_obj                     ( const CComPtr<IDispatch>&, const string& name );
    void                        close                       ();
    CComVariant                 call_if_exists              ( const char* name );
    CComVariant                 call                        ( const char* name );
    CComVariant                 call                        ( const char* name, int param );
    CComVariant                 property_get                ( const char* name );
    void                        property_put                ( const char* name, const CComVariant& v ) { _script_site->property_put( name, v ); } 
    void                        optional_property_put       ( const char* name, const CComVariant& v );
    bool                        name_exists                 ( const string& name );
    bool                        loaded                      ()                              { return _loaded; }
  //void                        interrupt                   ();

                                operator bool               () const                        { return _script_site != NULL; }


    typedef HRESULT (WINAPI *DllGetClassObject_func)(CLSID*,IID*,void**);

    Fill_zero                  _zero_;
    Prefix_log*                _log;
    Script*                    _script;

    CComPtr<Com_context>       _com_context;

    CComPtr<Script_site>       _script_site;
    
    HMODULE                    _module;                     // Für _script->_filename != ""
    DllGetClassObject_func     _DllGetClassObject;          // Für _script->_filename != ""

    CComPtr<IDispatch>         _idispatch;
    bool                       _loaded;
    map<string,bool>           _names;
};

//-----------------------------------------------------------------------------------Script_context
/*
struct Script_context
{

    CComPtr<Spooler_com>       _spooler;
    CComPtr<Spooler_task>      _task;
};

//------------------------------------------------------------------------------------Script_object

struct Script_object
{

    CComPtr
    CComPtr<IDispatch>         _idispatch;
};
*/
//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
