// $Id: spooler_script.h,v 1.2 2001/01/27 19:26:16 jz Exp $

#ifndef __SPOOLER_SCRIPT_H
#define __SPOOLER_SCRIPT_H

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------Script

struct Script
{
    enum Reuse
    {
        reuse_task,
        reuse_job,
        reuse_global
    };
                                Script                      ()                              {}
    explicit                    Script                      ( const xml::Element_ptr& e )   { set_xml(e); }

    void                        set_xml                     ( const xml::Element_ptr& );

    bool                        empty                       () const                        { return _text.empty(); }
    void                        clear                       ()                              { _language="", _text=""; }

    string                     _language;
    string                     _text;
    Reuse                      _reuse;
};

//----------------------------------------------------------------------------------Script_instance

struct Script_instance
{
                                Script_instance             ( Spooler* )                    : _loaded(false) {}

    void                        init                        ( const string& language );
    void                        load                        ( const Script& );
    IDispatch*                  dispatch                    () const                        { return _script_site? _script_site->dispatch() : NULL; }
    void                        add_obj                     ( const CComPtr<IDispatch>&, const string& name );
    void                        close                       ();
    CComVariant                 call_if_exists              ( const char* name );
    CComVariant                 call                        ( const char* name );
    CComVariant                 call                        ( const char* name, int param );
    CComVariant                 property_get                ( const char* name );
    void                        property_put                ( const char* name, const CComVariant& v ) { _script_site->property_put( name, v ); } 
    void                        optional_property_put       ( const char* name, const CComVariant& v );
    bool                        name_exists                 ( const char* name )            { return _script_site->name_exists(name); }
    bool                        loaded                      ()                              { return _loaded; }

    Spooler*                   _spooler;
    CComPtr<Script_site>       _script_site;
    bool                       _loaded;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
