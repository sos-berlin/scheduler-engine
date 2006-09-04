// $Id$

#ifndef __SPOOLER_MODULE_INTERNAL_H
#define __SPOOLER_MODULE_INTERNAL_H

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------Internal_module_instance

struct Internal_module_instance : Module_instance
{
                                Internal_module_instance    ( Module* );

  //void                        init                        ();
  //bool                        load                        ();
  //void                        start                       ();
    virtual void                add_obj                     ( IDispatch*, const string& name );
  //void                        close__end                  ();
    Variant                     call                        ( const string& name );
    Variant                     call                        ( const string& name, bool param );      
    bool                        name_exists                 ( const string& name );
    bool                        loaded                      ()                                      { return true; }
    bool                        callable                    ()                                      { return true; }

    virtual bool                spooler_process             ()                                      { return false; }

    Fill_zero                  _zero_;
    Task*                      _task;
    Prefix_log*                _log;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
