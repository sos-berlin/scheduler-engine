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
      //Com_remote_module_instance_server* _com_remote_module_instance_server;      // Das einzige Objekt der Klasse
    };


    //struct Stdout_stderr_handler : Object, Stdout_stderr_collector::Handler
    //{
    //                            Stdout_stderr_handler       ( Com_remote_module_instance_server* s, const string& prefix ) : _com_server(s), _prefix(prefix) {}
    //                           ~Stdout_stderr_handler       ()                                      {}

    //    // Stdout_stderr_collector::Handler
    //    void                    on_thread_has_received_data ( const string& );

    //  private:
    //    string                             _prefix;
    //    Com_remote_module_instance_server* _com_server;
    //};


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



    ptr<Remote_module_instance_server> _server;
    com::object_server::Session*  _session;
    ptr<Class_data>               _class_data;
    ptr<Com_log_proxy>            _log;
    ptr<File_logger>              _file_logger;
    //ptr<Stdout_stderr_collector>  _stdout_stderr_collector;
    //ptr<Stdout_stderr_handler>    _stdout_handler;
    //ptr<Stdout_stderr_handler>    _stderr_handler;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
