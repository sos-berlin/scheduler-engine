// $Id: spooler_module_com.h,v 1.2 2002/11/06 06:30:48 jz Exp $

#ifndef __SPOOLER_MODULE_COM_H
#define __SPOOLER_MODULE_COM_H

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

bool                            check_result                ( const Variant& vt );

//-------------------------------------------------------------------------Com_module_instance_base
// Oberklasse

struct Com_module_instance_base : Module_instance
{
                                Com_module_instance_base    ( Module* script )                      : Module_instance(script), _zero_(this+1) {}

    void                        init                        ();
    IDispatch*                  dispatch                    () const                                { return _idispatch; }
               Module_instance::add_obj;
    void                        close                       ();
    Variant                     call                        ( const string& name );
    Variant                     call                        ( const string& name, int param );
    bool                        name_exists                 ( const string& name );
    bool                        callable                    ()                                      { return _idispatch != NULL; }


    Fill_zero                  _zero_;
    ptr<IDispatch>             _idispatch;
};

//------------------------------------------------------------------------------Com_module_instance
// Für COM-Objekte

struct Com_module_instance : Com_module_instance_base
{
                                Com_module_instance         ( Module* script )                      : Com_module_instance_base(script), _zero_(this+1) {}
                               ~Com_module_instance         ();

    void                        init                        ();
    void                        load                        ();
    void                        close                       ();


    typedef HRESULT (WINAPI *DllGetClassObject_func)(CLSID*,IID*,void**);

    Fill_zero                  _zero_;
    HMODULE                    _com_module;                 // Für _module->_filename != ""
    DllGetClassObject_func     _DllGetClassObject;          // Für _module->_filename != ""
};

//-----------------------------------------------------------------Scripting_engine_module_instance
// Für Scripting Engines

struct Scripting_engine_module_instance : Com_module_instance_base
{
                                Scripting_engine_module_instance( Module* script )                  : Com_module_instance_base(script), _zero_(this+1) {}
                               ~Scripting_engine_module_instance();

    void                        init                        ();
    void                        load                        ();
    void                        start                       ();
    virtual void                add_obj                     ( const ptr<IDispatch>&, const string& name );
    void                        close                       ();


    Fill_zero                  _zero_;
    ptr<Script_site>           _script_site;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
