// $Id: spooler_module_remote_server.h,v 1.3 2003/05/31 10:01:13 jz Exp $

#ifndef __SPOOLER_MODULE_REMOTE_SERVER_H
#define __SPOOLER_MODULE_REMOTE_SERVER_H

#include "../zschimmer/com_remote.h"

namespace sos {
namespace spooler {

//--------------------------------------------------------------------Remote_module_instance_server

struct Remote_module_instance_server : spooler_com::Iremote_module_instance_server,
                                       Sos_ole_object,
                                       Com_module_instance_base

{
                Sos_ole_object::operator new;
                Sos_ole_object::operator delete;


                                Remote_module_instance_server();
                               ~Remote_module_instance_server();

    static HRESULT              create_instance             ( const IID&, ptr<IUnknown>* );

    STDMETHODIMP                QueryInterface              ( const IID&, void** );
    USE_SOS_OLE_OBJECT_WITHOUT_QI
/*
    STDMETHODIMP_(ulong)        AddRef                      ()                                      { return Object::AddRef(); }
    STDMETHODIMP_(ulong)        Release                     ()                                      { return Object::Release(); }
    STDMETHODIMP                GetTypeInfoCount            ( UINT* result )                        { *result = 0; return E_FAIL; }
    STDMETHODIMP                GetTypeInfo                 ( UINT, LCID, ITypeInfo** )             { return E_FAIL; }
    STDMETHODIMP                GetIDsOfNames               ( REFIID, OLECHAR**, UINT, LCID, DISPID* );
    STDMETHODIMP                Invoke                      ( DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT* );
*/

    STDMETHODIMP                construct                   ( SAFEARRAY* );
    STDMETHODIMP                add_obj                     ( IDispatch*, BSTR name );
    STDMETHODIMP                name_exists                 ( BSTR name, VARIANT_BOOL* result );
    STDMETHODIMP                call                        ( BSTR name, VARIANT* result );

    void                        load_implicitly             ();


    Fill_zero                  _zero_;
    ptr<Module_instance>       _module_instance;
    bool                       _loaded_and_started;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
