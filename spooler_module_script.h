#ifndef __SCRIPT_MODULE_SCRIPT_H
#define __SCRIPT_MODULE_SCRIPT_H

#include "spooler_module_script_interface.h"
namespace sos {
namespace scheduler {

//----------------------------------------------------------------------------------Script_module

struct Script_module : Module
{
                                Script_module             ( Spooler*, Prefix_log* = NULL );
    virtual ptr<Module_instance> create_instance_impl       () = 0;

};

//-------------------------------------------------------------------------Script_module_instance

struct Script_module_instance : Module_instance
{
                                Script_module_instance    ( Module*, const string& servicename );

  //void                        init                        ();
    bool                        load                        ()                                      { _loaded = true; return Module_instance::load(); }
  //void                        start                       ();
    virtual void                add_obj                     ( IDispatch*, const string& name );
  //void                        close__end                  ();
    Variant                     call                        ( const string& name );
    Variant                     call                        ( const string& name, const Variant& param, const Variant& );      
    bool                        name_exists                 ( const string& name );
    bool                        loaded                      ()                                      { return _loaded; }
    bool                        callable                    ()                                      { return true; }

    virtual bool                spooler_process             ()                                      { return false; }

    Fill_zero                  _zero_;
    Task*                      _task;
    Has_log*                   _log;
    bool                       _loaded;

//    string                     _servicename;
//    scheduler_java::ScriptConnector  _scriptconnector;                
    scheduler_java::ScriptInterface  _scriptinterface;                

    typedef list< ptr<z::javabridge::Java_idispatch> >  Added_objects;
    Added_objects              _added_jobjects;

};
//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
