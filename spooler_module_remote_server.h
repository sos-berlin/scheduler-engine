// $Id$

#ifndef __SPOOLER_MODULE_REMOTE_SERVER_H
#define __SPOOLER_MODULE_REMOTE_SERVER_H

#include "../zschimmer/com_remote.h"

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------Remote_module_instance_server

struct Remote_module_instance_server : Com_module_instance_base
{
                                Remote_module_instance_server( const string& include_path );
                               ~Remote_module_instance_server();


    void                        close__end                  ();
    void                        load_implicitly             ();
    void                        try_delete_files            ();


    Fill_zero                  _zero_;
    ptr<Module_instance>       _module_instance;
    ptr<File_logger>           _file_logger;
    Fill_end                   _end_;
};

//----------------------------------------------------------------Com_remote_module_instance_server

struct Com_remote_module_instance_server : spooler_com::Iremote_module_instance_server,
                                           Sos_ole_object
{
    struct Class_data : Object
    {
                                Class_data                  ();
        void                    read_xml                    ( const string& );

        Fill_zero              _zero_;
        xml::Document_ptr      _stdin_dom_document;
        xml::Element_ptr       _task_process_element;
        pid_t                  _remote_instance_pid;
    };


                                Com_remote_module_instance_server( com::object_server::Session*, ptr<Object>* );
                               ~Com_remote_module_instance_server();

    static HRESULT              Create_instance             ( zschimmer::com::object_server::Session*, ptr<Object>*, const IID&, ptr<IUnknown>* );

    STDMETHODIMP                QueryInterface              ( const IID&, void** );
    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP                Construct                   ( SAFEARRAY*, VARIANT_BOOL* );
    STDMETHODIMP                Add_obj                     ( IDispatch*, BSTR name );
    STDMETHODIMP                Name_exists                 ( BSTR name, VARIANT_BOOL* result );
    STDMETHODIMP                Call                        ( BSTR name, VARIANT* result );
    STDMETHODIMP                Begin                       ( SAFEARRAY* objects, SAFEARRAY* names, VARIANT* result );
    STDMETHODIMP                End                         ( VARIANT_BOOL, VARIANT* result );
    STDMETHODIMP                Step                        ( VARIANT* result );
    STDMETHODIMP                Wait_for_subprocesses       ();

private:
    void                    set_vm_class_path               (javabridge::Vm* java_vm, const string& task_class_path) const;

public:
    Fill_zero                          _zero_;
    ptr<Remote_module_instance_server> _server;
    com::object_server::Session*  _session;
    ptr<Class_data>               _class_data;
    ptr<Com_log_proxy>            _log;

  //bool                          _log_stdout_stderr;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
