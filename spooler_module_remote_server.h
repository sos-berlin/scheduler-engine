// $Id: spooler_module_remote_server.h,v 1.4 2003/05/31 16:20:51 jz Exp $

#ifndef __SPOOLER_MODULE_REMOTE_SERVER_H
#define __SPOOLER_MODULE_REMOTE_SERVER_H

#include "../zschimmer/com_remote.h"

namespace sos {
namespace spooler {

//--------------------------------------------------------------------Remote_module_instance_server

struct Remote_module_instance_server : Com_module_instance_base

{
              //Sos_ole_object::operator new;
              //Sos_ole_object::operator delete;


                                Remote_module_instance_server();
                               ~Remote_module_instance_server();

    static HRESULT              create_instance             ( const IID&, ptr<IUnknown>* );

    STDMETHODIMP                QueryInterface              ( const IID&, void** );
    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP                construct                   ( SAFEARRAY* );
    STDMETHODIMP                add_obj                     ( IDispatch*, BSTR name );
    STDMETHODIMP                name_exists                 ( BSTR name, VARIANT_BOOL* result );
    STDMETHODIMP                call                        ( BSTR name, VARIANT* result );
    
    static IDispatch*           z_get_idispatch_base_class  ( Remote_module_instance_server* object ) { return (IDispatch*)(spooler_com::Iremote_module_instance_server*)object; }   // Für USE_SOS_OLE_OBJECT_WITHOUT_QI


    void                        load_implicitly             ();


    Fill_zero                  _zero_;
    ptr<Module_instance>       _module_instance;
    bool                       _loaded_and_started;
};

//----------------------------------------------------------------Com_remote_module_instance_server

struct Com_remote_module_instance_server : spooler_com::Iremote_module_instance_server,
                                           Sos_ole_object

{
                                Com_remote_module_instance_server();
                               ~Com_remote_module_instance_server();

    static HRESULT              create_instance             ( const IID&, ptr<IUnknown>* );

    STDMETHODIMP                QueryInterface              ( const IID&, void** );
    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP                construct                   ( SAFEARRAY* );
    STDMETHODIMP                add_obj                     ( IDispatch*, BSTR name );
    STDMETHODIMP                name_exists                 ( BSTR name, VARIANT_BOOL* result );
    STDMETHODIMP                call                        ( BSTR name, VARIANT* result );
    
    void                        load_implicitly             ();


    Remote_module_instance_server _remote_module_instance_server;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
