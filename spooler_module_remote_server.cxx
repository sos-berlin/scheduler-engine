// $Id$
/*
    Hier sind implementiert

    Remote_module_instance_server
*/



#include "spooler.h"
#include "spooler_module_remote_server.h"
#include "file_logger.h"
#include "../kram/sos_java.h"

#ifdef Z_WINDOWS
#   include <io.h>
#endif


namespace sos {
namespace scheduler {

using namespace zschimmer::com::object_server;
using namespace spooler_com;

DESCRIBE_CLASS( &spooler_typelib, Remote_module_instance_server, remote_module_instance_server, CLSID_Remote_module_instance_server, "Spooler.Remote_module_instance_server", "1.0" )


//-------------------------------------Remote_module_instance_server::Remote_module_instance_server

Remote_module_instance_server::Remote_module_instance_server( const string& include_path )
:
    Com_module_instance_base( Z_NEW( Module( NULL, include_path, NULL ) ) ),
    _zero_(_end_)
{
}

//------------------------------------Remote_module_instance_server::~Remote_module_instance_server

Remote_module_instance_server::~Remote_module_instance_server()
{
    try
    {
        close__end();  // Synchron
    }
    catch( exception& ) {}
}

//--------------------------------------------------------Remote_module_instance_server::close__end

void Remote_module_instance_server::close__end()   // synchron
{
    close_monitor();

    if( _module_instance )  
    {
        try_delete_files();     // Kann verzögern um delete_temporary_files_delay Sekunden

        _module_instance->close(), 
        _module_instance = NULL;
    }

    Com_module_instance_base::close__end();  // synchron
}

//--------------------------------------------------Remote_module_instance_server::try_delete_files

void Remote_module_instance_server::try_delete_files()
{
    bool deleted = _module_instance->try_delete_files( NULL );

    if( !deleted )
    {
        string paths = join( ", ", _module_instance->undeleted_files() );
        _log.debug( message_string( "SCHEDULER-876", paths ) );  // Nur beim ersten Mal

        double until = double_from_gmtime() + delete_temporary_files_delay;
        while(1)
        {
            sleep( delete_temporary_files_retry );
            deleted = _module_instance->try_delete_files( NULL );
            if( deleted )  break;
            if( double_from_gmtime() >= until )  break;
        }

        if( deleted )  
        {
            _log.debug( message_string( "SCHEDULER-877" ) );  // Nur, wenn eine Datei nicht löschbar gewesen ist
        }
        else 
        {
            string paths = join( ", ", _module_instance->undeleted_files() );
            _log.info( message_string( "SCHEDULER-878", paths ) );
            //_job.log()->warn( message_string( "SCHEDULER-878", paths ) );
        }
    }
}

//------------------------------------------------------Com_remote_module_instance_server::_methods
#ifdef Z_COM

const Com_method Com_remote_module_instance_server::_methods[] =
{ 
   // _flags              , _name                     , _method                                                                  , _result_type , _types        , _default_arg_count
    { DISPATCH_METHOD     , 1, "Construct"            , (Com_method_ptr)&Com_remote_module_instance_server::Construct            , VT_BOOL      , { VT_ARRAY|VT_VARIANT } },
    { DISPATCH_METHOD     , 2, "Add_obj"              , (Com_method_ptr)&Com_remote_module_instance_server::Add_obj              , VT_EMPTY     , { VT_DISPATCH, VT_BSTR } },
    { DISPATCH_METHOD     , 3, "Name_exists"          , (Com_method_ptr)&Com_remote_module_instance_server::Name_exists          , VT_BOOL      , { VT_BSTR } },
    { DISPATCH_METHOD     , 4, "Call"                 , (Com_method_ptr)&Com_remote_module_instance_server::Call                 , VT_VARIANT   , { VT_BSTR } },
    { DISPATCH_METHOD     , 5, "Begin"                , (Com_method_ptr)&Com_remote_module_instance_server::Begin                , VT_VARIANT   , { VT_ARRAY|VT_VARIANT, VT_ARRAY|VT_VARIANT } },
    { DISPATCH_METHOD     , 6, "End"                  , (Com_method_ptr)&Com_remote_module_instance_server::End                  , VT_VARIANT   , { VT_BOOL } },
    { DISPATCH_METHOD     , 7, "Step"                 , (Com_method_ptr)&Com_remote_module_instance_server::Step                 , VT_VARIANT   },
    { DISPATCH_METHOD     , 8, "Wait_for_subprocesses", (Com_method_ptr)&Com_remote_module_instance_server::Wait_for_subprocesses, VT_EMPTY     },
    {}
};

#endif
//-----------------------------------------------Com_remote_module_instance_server::create_instance

HRESULT Com_remote_module_instance_server::Create_instance( zschimmer::com::object_server::Session* session, ptr<Object>* class_object_ptr, const IID& iid, ptr<IUnknown>* result )
{
    if( iid == IID_Iremote_module_instance_server )
    {
        ptr<Iremote_module_instance_server> instance = new Com_remote_module_instance_server( session, class_object_ptr );
        *result = +instance;
        return S_OK;
    }

    return E_NOINTERFACE;
}

//----------------------------------------Com_remote_module_instance_server::Class_data::Class_data

Com_remote_module_instance_server::Class_data::Class_data()
:
    _zero_(this+1)
{
}

//----------------------------------------Com_remote_module_instance_server::Class_data::initialize

void Com_remote_module_instance_server::Class_data::read_xml( const string& xml_text )
{
    if( xml_text != "" )
    {
        //Z_LOG2( "joacim", Z_FUNCTION << xml_text << "\n" );
        
        _stdin_dom_document.load_xml( xml_text );

        _task_process_element = _stdin_dom_document.documentElement();
        if( !_task_process_element  ||  !_task_process_element.nodeName_is( "task_process" ) )  z::throw_xc( Z_FUNCTION, "<task_process> expected" );
    }

    //::close( STDIN_FILENO );  // 2007-07-09 Brauchen wir nicht mehr, also schließen, bevor ein Enkelprozess den Handle erbt und dann die Datei nicht löschbar ist.
    //STDIN_FILENO neu belegen (mit dup()?), damit nicht irgend ein open() STDIN_FILENO zurückliefert. Windows: "nul:", Unix: "/dev/null".
}

//------------Com_remote_module_instance_server::Stdout_stderr_handler::on_thread_has_received_data

//void Com_remote_module_instance_server::Stdout_stderr_handler::on_thread_has_received_data( const string& text )
//{
//    int MUTEX_SPERREN;
//    int WAS_MACHEN_WIR_MIT_DER_EXCEPTION;
//
//    if( _com_server  &&  _com_server->_server  &&  _com_server->_server->_module_instance )
//    {
//        if( IDispatch* spooler_log = _com_server->_server->_module_instance->object( "spooler_log", (IDispatch*)NULL ) )
//        {
//            vector<Variant> parameters;
//            parameters.push_back( ( S() << _prefix << ": " << text ).to_string() );
//
//            com_invoke( DISPATCH_METHOD, spooler_log, "info", &parameters );
//        }
//    }
//}

//-----------------------------Com_remote_module_instance_server::Com_remote_module_instance_server

Com_remote_module_instance_server::Com_remote_module_instance_server( com::object_server::Session* session, ptr<Object>* class_object_ptr )
:
    Sos_ole_object( remote_module_instance_server_class_ptr, (Iremote_module_instance_server*)this ),
    _zero_(this+1),
    _session(session)
{
    if( *class_object_ptr )
    {
        _class_data = dynamic_cast<Class_data*>( +*class_object_ptr );
        if( !_class_data )  assert(0), z::throw_xc( Z_FUNCTION );
    }
    else
    {
        _class_data = Z_NEW( Class_data );
        _class_data->read_xml( _session->connection()->server()->stdin_data() );

        *class_object_ptr = _class_data;
    }
}

//----------------------------Com_remote_module_instance_server::~Com_remote_module_instance_server

Com_remote_module_instance_server::~Com_remote_module_instance_server()
{
    if( _file_logger )  _file_logger->close();
}

//------------------------------------------------Com_remote_module_instance_server::QueryInterface

STDMETHODIMP Com_remote_module_instance_server::QueryInterface( const IID& iid, void** result )
{
    if( iid == IID_IUnknown )
    {
        *result = (IUnknown*)(Iremote_module_instance_server*)this;
        ((IUnknown*)(Iremote_module_instance_server*)this)->AddRef();
        return S_OK;
    }
    if( iid == IID_IDispatch )
    {
        *result = (IDispatch*)(Iremote_module_instance_server*)this;
        ((IDispatch*)(Iremote_module_instance_server*)this)->AddRef();
        return S_OK;
    }
    else
    if( iid == IID_Iremote_module_instance_server )
    {
        *result = (Iremote_module_instance_server*)this;
        ((Iremote_module_instance_server*)this)->AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

//-----------------------------------------------------Com_remote_module_instance_server::construct

STDMETHODIMP Com_remote_module_instance_server::Construct( SAFEARRAY* safearray, VARIANT_BOOL* result )
{
    HRESULT hr = NOERROR;

    *result = VARIANT_FALSE;

    try
    {
        string job_name;
        string include_path    = _class_data->_task_process_element.getAttribute( "include_path"    );
        string java_options    = _class_data->_task_process_element.getAttribute( "java_options"    );
        string java_class_path = _class_data->_task_process_element.getAttribute( "java_class_path" );
        string javac           = _class_data->_task_process_element.getAttribute( "javac"           );
        string java_work_dir   = _class_data->_task_process_element.getAttribute( "java_work_dir"   );
      //_log_stdout_stderr     = _class_data->_task_process_element.getAttribute( "stdout_path"     ) == "";

        DOM_FOR_EACH_ELEMENT( _class_data->_task_process_element, e )
        {
            if( e.nodeName_is( "environment" ) )
            {
                DOM_FOR_EACH_ELEMENT( e, ee )
                {
                    if( ee.nodeName_is( "variable" ) ) 
                    {
                        string name;
                        string value;

                        get_variable_name_and_value( ee, &name, &value );

                        set_environment_variable( name, value );
                    }
                }
            }
        }


        int                       task_id = 0;
        Locked_safearray<Variant> params  ( safearray );
        ptr<Module_monitor>       monitor;

        _server = Z_NEW( Remote_module_instance_server( include_path ) );

        for( int i = 0; i < params.count(); i++ )
        {
            if( params[i].vt != VT_EMPTY )
            {
                if( params[i].vt != VT_BSTR )  assert(0), throw_xc( "_spooler_construct" );

                const OLECHAR* ole_value = wcschr( V_BSTR( &params[i] ), '=' );
                if( !ole_value )  assert(0), throw_xc( "_spooler_construct" );
                string key_word = string_from_ole( V_BSTR( &params[i] ), ole_value - V_BSTR( &params[i] ) );
                ole_value++;
                string value = string_from_ole( ole_value );

                if( key_word == "language"         )  _server->_module->_language        = value;
                else                                                                         
                if( key_word == "com_class"        )  _server->_module->_com_class_name  = value;
                else                                                                         
                if( key_word == "filename"         )  _server->_module->_filename        = value;
                else
                if( key_word == "java_class"       )  _server->_module->_java_class_name = value;
                else
                if( key_word == "recompile"        )  _server->_module->_recompile       = value[0] == '1';
                else
                if( key_word == "script"           )  _server->_module->set_xml_text_with_includes( value );
                else
                if( key_word == "java_options"     )  java_options                     += " " + value;
                else
                if( key_word == "job"              )  job_name                          = value;
                else
                if( key_word == "task_id"          )  task_id                           = as_int( value );
                //else
                //if( key_word == "log_stdout_stderr"     )  _log_stdout_stderr                       = as_bool( value );
                else
                if( key_word == "process.filename"      )  _server->_module->_process_filename      = value;
                else
                if( key_word == "process.param_raw"     )  _server->_module->_process_param_raw     = value;
                else
                if( key_word == "process.log_filename"  )  _server->_module->_process_log_filename  = value;
                else
                if( key_word == "process.ignore_error"  )  _server->_module->_process_ignore_error  = as_bool( value );
                else
                if( key_word == "process.ignore_signal" )  _server->_module->_process_ignore_signal = as_bool( value );
                else
                if( key_word == "monitor.language" ) // Muss der erste Parameter für den Module_monitor sein!
                {
                    monitor = Z_NEW( Module_monitor );
                    monitor->_module = Z_NEW( Module( NULL, include_path, NULL ) );  
                    monitor->_module->_language = value;
                }
                else                                                                         
                if( monitor )
                {
                    if( key_word == "monitor.name"       )  monitor->_name                     = value;
                    else                                                                         
                    if( key_word == "monitor.ordering"   )  monitor->_ordering                 = as_int( value );
                    else                                                                         
                    if( key_word == "monitor.com_class"  )  monitor->_module->_com_class_name  = value;
                    else                                                                         
                    if( key_word == "monitor.filename"   )  monitor->_module->_filename        = value;
                    else
                    if( key_word == "monitor.java_class" )  monitor->_module->_java_class_name = value;
                    else
                    if( key_word == "monitor.recompile"  )  monitor->_module->_recompile       = value[0] == '1';
                    else
                    if( key_word == "monitor.script"     )  // Muss der letzte Paraemter sein!
                    {
                        monitor->_module->set_xml_text_with_includes( value );
                        _server->_module->_monitors->add_monitor( monitor );
                    }
                    else
                        assert(0), throw_xc( "server::construct", as_string(i), key_word );
                }
            }
        }

        //Z_LOG2( "joacim", Z_FUNCTION << " java_class_path=" << java_class_path << "\n" );;
        //Z_LOG2( "joacim", Z_FUNCTION << " java_work_dir  =" << java_work_dir << "\n" );;
        //Z_LOG2( "joacim", Z_FUNCTION << " java_options   =" << java_options << "\n" );;
        //Z_LOG2( "joacim", Z_FUNCTION << " javac          =" << javac << "\n" );;

        _server->_module->init();


        ptr<java::Vm> java_vm = get_java_vm( false );
        _server->_module->_java_vm = java_vm;

        if( !_server->_module->_java_vm->running() )
        {
            // Java einstellen, falls der Job in Java geschrieben ist oder indirekt (über Javascript) Java benutzt.
            //java_vm->set_log( &_log );
            if( !java_class_path.empty() )
                java_vm->set_class_path( java_class_path );

            java_vm->set_javac_filename( javac );
            java_vm->set_options( java_options );

            if( !java_work_dir.empty() )
            {
                if( _server->_module->needs_java() )
                {
                    java_vm->set_work_dir( java_work_dir );
                    java_vm->prepend_class_path( java_work_dir );
                }
            }
        }
        else
        {
            Z_LOG2( "java", "Com_remote_module_instance_server::Construct: Die Java Virtual Machine läuft bereits.\n" );
            // Parameter für Java können nicht übernommen werden.
        }

        _server->_module_instance = _server->_module->create_instance();
       
        if( _server->_module_instance )
        {
            _server->_module_instance->set_job_name( job_name );             // Nur zur Diagnose
            _server->_module_instance->set_task_id( task_id );               // Nur zur Diagnose
          //_server->_module_instance->init();
          //_server->_module_instance->_spooler_exit_called = true;            // Der Client wird spooler_exit() explizit aufrufen, um den Fehler zu bekommen.
            *result = VARIANT_TRUE;
        }
    }
    catch( const exception& x ) { hr = Com_set_error( x, "Remote_module_instance_server::construct" ); }

    return hr;
}

//-------------------------------------------------------Com_remote_module_instance_server::Add_obj

STDMETHODIMP Com_remote_module_instance_server::Add_obj( IDispatch* object, BSTR name_bstr )
{
    HRESULT hr = NOERROR;

    try
    {
        string name = string_from_bstr( name_bstr );
        
        if( !_server->_module_instance )  z::throw_xc( "SCHEDULER-203", "add_obj", name );
        _server->_module_instance->add_obj( object, name );
    }
    catch( const exception& x ) { hr = Com_set_error( x, "Remote_module_instance_server::add_obj" ); }

    return hr;
}

//---------------------------------------------------Com_remote_module_instance_server::name_exists

STDMETHODIMP Com_remote_module_instance_server::Name_exists( BSTR name, VARIANT_BOOL* result )
{
    HRESULT hr = NOERROR;

    try
    {
        if( !_server->_module_instance )  z::throw_xc( "SCHEDULER-203", "name_exists", string_from_bstr(name) );
      //_server->load_implicitly();
        *result = _server->_module_instance->name_exists( string_from_bstr(name) );
    }
    catch( const exception& x ) { hr = Com_set_error( x, "Remote_module_instance_server::name_exists" ); }

    return hr;
}

//----------------------------------------------------------Com_remote_module_instance_server::call

STDMETHODIMP Com_remote_module_instance_server::Call( BSTR name_bstr, VARIANT* result )
{
    HRESULT hr = NOERROR;

  //In_call in_call ( this, name );

    try
    {
      //_server->load_implicitly();
      //_server->_module_instance->call( string_from_bstr(name) ).CopyTo( result );

        string name = string_from_bstr( name_bstr );

        if( !_server  ||  !_server->_module_instance )  
        {
            if( name == spooler_close_name
             || name == spooler_on_error_name
             || name == spooler_exit_name     ) 
            {
                result->vt = VT_EMPTY;
                return hr;
            }

            z::throw_xc( "SCHEDULER-203", "call", name );
        }

        _server->_module_instance->call__start( name ) -> async_finish();
        _server->_module_instance->call__end().move_to( result );
    }
    catch( const exception& x ) { hr = Com_set_error( x, "Remote_module_instance_server::call" ); }

    return hr;
}

//---------------------------------------------------------Com_remote_module_instance_server::begin

STDMETHODIMP Com_remote_module_instance_server::Begin( SAFEARRAY* objects_safearray, SAFEARRAY* names_safearray, VARIANT* result )
{
    HRESULT hr = NOERROR;

    try
    {
        if( !_server->_module_instance )  z::throw_xc( "SCHEDULER-203", "begin" );

        Locked_safearray<Variant> objects ( objects_safearray );
        Locked_safearray<Variant> names   ( names_safearray );

        for( int i = 0; i < objects.count(); i++ )  
        {
            VARIANT* o = &objects[i];
            if( o->vt != VT_DISPATCH )  return DISP_E_BADVARTYPE;
            IDispatch* object = V_DISPATCH( o );
            
            string name = string_from_variant( names[i] );
            _server->_module_instance->_object_list.push_back( Module_instance::Object_list_entry( object, name ) );

            if( name == "spooler_log" )  
            {
                _log = dynamic_cast<Com_log_proxy*>( object ),  assert( _log );
                _server->set_log( _log );
                _server->_module_instance->set_log( _log );
            }
        }


        Async_operation* operation = _server->_module_instance->begin__start();

        if( _log ) 
        {
            assert( !_file_logger );

            _file_logger = Z_NEW( File_logger( _log ) );
            _file_logger->set_object_name( "Com_remote_module_instance_server" );   // Nur zur Info
            
            _file_logger->add_file( _class_data->_task_process_element.getAttribute( "stdout_path" ), "stdout" );
            _file_logger->add_file( _class_data->_task_process_element.getAttribute( "stderr_path" ), "stderr" );
            // Entweder die oberen beiden oder die unteren beiden sind gültig, also nicht "". Die unteren bei Process_module_instance
            _file_logger->add_file( _server->_module_instance->stdout_path(), "stdout" );  // Process_module_instance::begin__start() hat die Dateien angelegt
            _file_logger->add_file( _server->_module_instance->stderr_path(), "stderr" );
            
            if( _file_logger->has_files() )  _file_logger->start_thread();
        }

        operation->async_finish();

        result->vt = VT_BOOL, V_BOOL( result ) = _server->_module_instance->begin__end();


        assert( !_class_data->_remote_instance_pid );
        _class_data->_remote_instance_pid = _server->_module_instance->pid();
    }
    catch( const exception& x ) { hr = Com_set_error( x, "Remote_module_instance_server::begin" ); }

    return hr;
}

//-----------------------------------------------------------Com_remote_module_instance_server::end

STDMETHODIMP Com_remote_module_instance_server::End( VARIANT_BOOL succeeded, VARIANT* result )
{
    HRESULT hr = NOERROR;
    
    if( result )  result->vt = VT_EMPTY;

    try
    {
        if( _server &&  _server->_module_instance )
        {
            _server->_module_instance->end__start( succeeded != 0 ) -> async_finish();
            _server->_module_instance->end__end();
        }

        if( _file_logger )  _file_logger->finish();
    }
    catch( const exception& x ) { hr = Com_set_error( x, "Remote_module_instance_server::end" ); }

    return hr;
}

//----------------------------------------------------------Com_remote_module_instance_server::step

STDMETHODIMP Com_remote_module_instance_server::Step( VARIANT* result )
{
    HRESULT hr = NOERROR;

    try
    {
        if( !_server->_module_instance )  z::throw_xc( "SCHEDULER-203", "step" );

        _server->_module_instance->step__start() -> async_finish();
        _server->_module_instance->step__end().move_to( result );
    }
    catch( const exception& x ) { hr = Com_set_error( x, "Remote_module_instance_server::step" ); }

    return hr;
}

//-----------------------------------------Com_remote_module_instance_server::Wait_for_subprocesses

STDMETHODIMP Com_remote_module_instance_server::Wait_for_subprocesses()
{
    Z_LOGI2( "scheduler", "Com_remote_module_instance_server::Wait_for_subprocesses()\n" );

    HRESULT hr = NOERROR;

    try
    {
        //Com_task_proxy* task_proxy = NULL;
        //com_query_interface( _server->_module_instance->object( "spooler_task" ), &task_proxy );
        
        Com_task_proxy* task_proxy = dynamic_cast< Com_task_proxy* >( _server->_module_instance->object( "spooler_task" ) );
        if( !task_proxy )  assert(0), throw_xc( "Wait_for_subprocesses" );

        task_proxy->wait_for_subprocesses();
    }
    catch( const exception& x ) { hr = Com_set_error( x, "Remote_module_instance_server::step" ); }

    return hr;
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
