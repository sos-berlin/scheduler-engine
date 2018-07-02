// $Id: spooler_module_remote_server.h 14010 2010-09-13 08:05:23Z jz $

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
    void                        try_delete_files            ();

    Fill_zero                  _zero_;
    ptr<Module_instance>       _module_instance;
    ptr<File_logger>           _file_logger;
    Fill_end                   _end_;

    #if defined Z_UNIX
        static void signal_handler(int, siginfo_t*, void*);
        void catch_sigterm_for_shell_process();
        void on_sigterm();
        bool _own_sigaction_installed;
        struct sigaction _previous_sigaction;
        static Remote_module_instance_server* static_this;
    #endif
};

//----------------------------------------------------------------Com_remote_module_instance_server

struct Com_remote_module_instance_server : spooler_com::Iremote_module_instance_server,
                                           Sos_ole_object
{
    struct Class_data : Object
    {
                                Class_data                  ();
        void                    read_xml_bytes              ( const string& );

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

public:
    Fill_zero                          _zero_;
    ptr<Remote_module_instance_server> _server;
    com::object_server::Session*  _session;
    ptr<Class_data>               _class_data;
    ptr<Com_log_proxy>            _log;
    private: z::Log_level _stderr_log_level;

  //bool                          _log_stdout_stderr;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
