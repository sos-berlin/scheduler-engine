// $Id: spooler_module_remote_server.h,v 1.12 2004/04/05 08:49:46 jz Exp $

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


    void                        close__end                  ();
    void                        load_implicitly             ();


    Fill_zero                  _zero_;
    ptr<Module_instance>       _module_instance;
    bool                       _loaded_and_started;
    bool                       _load_error;
    Fill_end                   _end_;
};

//----------------------------------------------------------------Com_remote_module_instance_server

struct Com_remote_module_instance_server : spooler_com::Iremote_module_instance_server,
                                           Sos_ole_object

{
                                Com_remote_module_instance_server();
                               ~Com_remote_module_instance_server();

    static HRESULT              Create_instance             ( const IID&, ptr<IUnknown>* );

    STDMETHODIMP                QueryInterface              ( const IID&, void** );
    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP                Construct                   ( SAFEARRAY* );
    STDMETHODIMP                Add_obj                     ( IDispatch*, BSTR name );
    STDMETHODIMP                Name_exists                 ( BSTR name, VARIANT_BOOL* result );
    STDMETHODIMP                Call                        ( BSTR name, VARIANT* result );
    STDMETHODIMP                Begin                       ( SAFEARRAY* objects, SAFEARRAY* names, VARIANT* result );
    STDMETHODIMP                End                         ( VARIANT_BOOL, VARIANT* result );
    STDMETHODIMP                Step                        ( VARIANT* result );



    Remote_module_instance_server _server;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
