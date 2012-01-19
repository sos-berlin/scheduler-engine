// $Id: spooler_module_process.cxx 14205 2011-03-21 15:45:37Z jz $

#include "spooler.h"

using namespace std;

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------------------------const

const string                    order_params_environment_name   = "SCHEDULER_RETURN_VALUES";
const int                       max_stdout_state_text_length    = 100;                              // Für Job.state_text und Order.state_text

//--------------------------------------------------------------------------------------Kill_thread
#ifdef Z_UNIX

struct Kill_thread : Thread
{
    Kill_thread()
    :
        _zero_(this+1)
    {
    }


    int thread_main()
    {
        try
        {
            posix::try_kill_process_with_descendants_immediately( _pid, _log, &_message, _process_name );
            posix::try_kill_process_group_immediately           ( _pid,                  _process_name );
        }
        catch( exception& x )
        {
            if( _log )  _log->warn( x.what() );
                  else  Z_LOG( Z_FUNCTION << " " << x.what() << "\n" );
        }

        return 0;
    }


    Fill_zero              _zero_;
    pid_t                  _pid;
    string                 _process_name;
    ptr<Prefix_log>        _log;
    Message_string         _message;
};

#endif
//-------------------------------------------------Process_module_instance::Process_module_instance

Process_module_instance::Process_module_instance( Module* module )
: 
    Module_instance(module),
    _zero_(this+1),
    _process_handle( "process_handle" )
{
    assert( _module );
}

//------------------------------------------------Process_module_instance::~Process_module_instance
    
Process_module_instance::~Process_module_instance()
{
    close_handle();

    // Nicht ins _log, also Task-Log protokollieren, damit mail_on_warning nicht greift. 
    // Das soll kein Task-Problem sein! Siehe auch Task::do_something, s_deleting_files
    // Kein base_log? Dann nicht nach stderr protokollieren (bei Betrieb Remote_module_instance_server).
    Delegated_log* unlink_log = _module &&  _module->_log.base_log()? &_module->_log : NULL;      

    if( _stdout_file      .is_to_be_unlinked() )  _stdout_file      .try_unlink( unlink_log );
    if( _stderr_file      .is_to_be_unlinked() )  _stderr_file      .try_unlink( unlink_log );
    if( _order_params_file.is_to_be_unlinked() )  _order_params_file.try_unlink( unlink_log );

    if( _kill_thread )  _kill_thread->thread_wait_for_termination();
}

//------------------------------------------------------------Process_module_instance::close_handle

void Process_module_instance::close_handle()
{
#   ifndef Z_WINDOWS
        if( _pid_to_unregister )
        {
            _spooler->unregister_process_handle( _pid_to_unregister );
            if( _spooler )  _pid_to_unregister = 0;
        }
#   endif


    if( _process_handle )
    {
#       ifdef Z_WINDOWS
            if( _spooler )  _spooler->unregister_process_handle( _process_handle );
#       endif

        _process_handle.close();
    }
}

//--------------------------------------------------------------------Process_module_instance::init
    
void Process_module_instance::init()
{
    Module_instance::init();

    if( _has_order )
    {
        _order_params_file.open_temporary( File::open_unlink_later );
        _order_params_file.close();
        _process_environment->set_var( order_params_environment_name, _order_params_file.path() );
    }
}

//--------------------------------------------------------------------Process_module_instance::load

bool Process_module_instance::load()
{
    //if( _module->_process_class_path != ""  &&  _module->process_class()->remote_scheduler() )  z::throw_xc( "SCHEDULER-400" );

    bool ok = Module_instance::load();
    if( !ok )  return ok;

    ok = check_result( _monitor_instances.spooler_process_before() );
    if( !ok )  return ok;
    _spooler_process_before_called = true;

    if( _module->_process_filename == "" )
    {
        string script = _module->read_source_script();

        _shell_file.unlink_later();

#       ifdef Z_WINDOWS
            // Dateiname muss auf .cmd enden!

            string prefix = get_temp_path() + "\\" + Z_TEMP_FILE_ID;
            for( int i = 1; i < 10000; i++ )
            {
                int random = ( rand() ^ (int)::time(NULL) ) & 0xfffff;
                bool ok = _shell_file.try_open( prefix + as_hex_string( random ) + ".cmd", _O_CREAT | _O_EXCL | _O_WRONLY | _O_SHORT_LIVED, 0600 );
                if( ok )  break;
                if( _shell_file.last_errno() == EEXIST )  continue;
                _shell_file.check_error( "open" );
            }

            if( _module->_encoding_code_page >= 0  &&  script.length() > 0 )
            {
                Bstr         script_bstr                = script;
                BOOL         default_character_was_used = false;
                vector<char> buffer                     ( 2 * script.length() );

                int cp = _module->_encoding_code_page;
                int length = WideCharToMultiByte( cp, WC_NO_BEST_FIT_CHARS, script_bstr, script_bstr.length(), &buffer[0], buffer.size(), 
                                                  NULL, cp == CP_UTF8 || cp == CP_UTF7? NULL : &default_character_was_used );
                if( length <= 0 )  z::throw_mswin( GetLastError(), "WideCharToMultiByte" );
                assert( length <= buffer.size() );
                if( default_character_was_used )  z::throw_xc( "SCHEDULER-471", "in <script>" );     // Leider wissen wir nicht, welches Zeichen nicht umgesetzt werden kann.
                
                script.assign( &buffer[0], length );
            }
#       else
            _shell_file.open_temporary( File::open_unlink_later );
            int ret = fchmod( _shell_file, 0700 );
            if( ret )  throw_errno( errno, "fchmod", _shell_file.path().c_str() );

            script = trim( script );    // Damit Unix-"#!" am Anfang steht. Das ändert die Zeilennummerierung.
#       endif

        _shell_file.print( script );
        _shell_file.close();
    }

    return ok;
}

//-------------------------------------------------------------------Process_module_instance::start

void Process_module_instance::start()
{
    Module_instance::start();

    //2008-06-15: _task_paramters an Remote Scheduler übergeben
    //ptr<Variable_set> vs = _process_environment->clone();
    //vs->merge( _task_parameters );
    //_process_param = subst_env( _module->_process_param_raw, vs );
    _process_param = subst_env( _module->_process_param_raw, _process_environment );
}

//---------------------------------------------------------------Process_module_instance::name_exists

bool Process_module_instance::name_exists( const string& )
{ 
    return false;
}

//----------------------------------------------------------------------Process_module_instance::call

Variant Process_module_instance::call( const string& name )
{
    assert(0);
    z::throw_xc( Z_FUNCTION, name );
}

//----------------------------------------------------------------------Process_module_instance::call

Variant Process_module_instance::call( const string& name, const Variant&, const Variant& )
{
    assert(0);
    z::throw_xc( Z_FUNCTION, name );
}

//------------------------------------------------------------Process_module_instance::program_path

string Process_module_instance::program_path()
{
    return _shell_file.path() != ""? _shell_file.path() :  _module->_process_filename;
}

//--------------------------------------------------------------Process_module_instance::close__end

void Process_module_instance::close__end()
{
    close_process();
    Module_instance::close__end();
}

//-----------------------------------------------------------Process_module_instance::close_process

void Process_module_instance::close_process()
{
#ifdef Z_WINDOWS
    if( _process_handle )
#endif
        kill();

    close_handle();

    _stdout_file.close();
    _stderr_file.close();

    /*
    if( !_stdout_logged )
    {
        _log.log_file( _stdout_file.filename(), "stdout:" );
        _log.log_file( _stderr_file.filename(), "stderr:" );
        _stdout_logged = true;
    }
    */
}

//------------------------------------------------------------Process_module_instance::begin__start

Async_operation* Process_module_instance::begin__start()
{
    // Für Com_remote_module_instance_server::Begin (File_logger) stdout- und stderr-Dateien schon hier anlegen
    // Wegen open_inheritable sollte vor begin__end() kein anderer Prozess gestartet werden.
    // (Sowieso wird begin__end() gleich danach gerufen)

    _stdout_file.open_temporary( File::open_unlink_later | File::open_inheritable );
    _stderr_file.open_temporary( File::open_unlink_later | File::open_inheritable );

    return Module_instance::begin__start();
}

//--------------------------------------------------------------Process_module_instance::begin__end
#ifdef Z_WINDOWS

bool Process_module_instance::begin__end()
{
    if( _spooler  &&  _spooler->_process_count >= max_processes )  z::throw_xc( "SCHEDULER-210", max_processes );

    if( !_load_called )
    {
        bool ok = implicit_load_and_start();
        if( !ok )  return false;
    }

    fill_process_environment_with_params();


    File stdin_file ("nul", "rI");
    PROCESS_INFORMATION process_info;
    STARTUPINFO         startup_info;
    BOOL                ok;

    memset( &process_info, 0, sizeof process_info );

    memset( &startup_info, 0, sizeof startup_info );
    startup_info.cb          = sizeof startup_info;
    startup_info.dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup_info.hStdInput   = stdin_file.handle();
    startup_info.hStdOutput  = _stdout_file.handle();
    startup_info.hStdError   = _stderr_file.handle();
    startup_info.wShowWindow = SW_MINIMIZE;            // Als Dienst mit Zugriff auf Desktop wird ein leeres Konsol-Fenster gezeigt. Das minimieren wir.


    string executable_path = program_path();
    string command_line = quoted_windows_process_parameter( executable_path );
    if( !_process_param.empty() )  command_line += " " + _process_param;

    if( _shell_file.path() != "" )
    {
        command_line = quoted_windows_process_parameter( _shell_file.path() );
    }

    if( _process_environment )
    {
        for( int i = 1;; i++ )
        {
            string nr = as_string(i);
            Variant vt;
            HRESULT hr;

            hr = _process_environment->get_Var( Bstr(nr), &vt );
            if( FAILED(hr) )  throw_ole( hr, "Variable_set.var", nr.c_str() );

            if( vt.vt == VT_EMPTY )  break;

            hr = vt.ChangeType( VT_BSTR );
            if( FAILED(hr) )  throw_ole( hr, "VariantChangeType", nr.c_str() );

            command_line += " " + quoted_windows_process_parameter( bstr_as_string( vt.bstrVal ) );
        }
    }

    // Environment
    S env;
    {
        ptr<Com_variable_set> environment = variable_set_from_environment();
        environment->merge( _process_environment );
        Z_FOR_EACH( Com_variable_set::Map, environment->_map, m )
            env << string_from_bstr ( m->second->_name ) << "=" << string_from_variant( m->second->_value ) << '\0';
        env << '\0';
    }

    DWORD creation_flags = 0;
    if( _module->_priority != "" )  creation_flags |= windows::priority_class_from_string( _module->_priority );        // Liefert 0 bei Fehler

    Message_string m ( "SCHEDULER-987" );
    m.set_max_insertion_length( INT_MAX );
    m.insert( 1, command_line );
    _log.info( m );

    Z_LOG2( "scheduler", "CreateProcess(" << command_line << ")\n" );
    
    ok = CreateProcess( NULL, //executable_path.c_str(),    // application name
                        (char*)command_line.c_str(),    // command line
                        NULL,                           // process security attributes
                        NULL,                           // primary thread security attributes
                        TRUE,                           // handles are inherited?
                        creation_flags,
                        (char*)((string)env).c_str(),
                        NULL,                           // use parent's current directory
                        &startup_info,                  // STARTUPINFO pointer
                        &process_info );                // receives PROCESS_INFORMATION
    if( !ok )  throw_mswin_error( "CreateProcess", executable_path );

    CloseHandle( process_info.hThread );

    _pid = process_info.dwProcessId;
    _process_handle.set_handle_noninheritable( process_info.hProcess );
    _process_handle.set_name( "Process " + program_path() );
    
    if( _spooler )  
    {
        _process_handle.add_to( &_spooler->_wait_handles );
        _spooler->register_process_handle( _process_handle );
    }

    _stdout_file.close();       // Schließen, damit nicht ein anderer Prozess die Handles erbt und damit das Löschen verhindert (ERRNO-13 Permission denied)
    _stderr_file.close();

    return true;
}

//--------------------------------------------------------------------Process_module_instance::kill

bool Process_module_instance::kill()
{
    if( !_process_handle )  return false;
    if( _is_killed )  return false;

    _log.warn( message_string( "SCHEDULER-281" ) );   

    Message_string m ( "SCHEDULER-709" );
    m.set_log_level( log_warn );
    windows::try_kill_process_with_descendants_immediately( _pid, _log.base_log(), &m, Z_FUNCTION );

    _is_killed = true;

    return true;
}

#endif
//----------------------------------------------------Process_module_instance::process_has_signaled

bool Process_module_instance::process_has_signaled()
{
    bool signaled = _process_handle.signaled();
    //_log->debug3( "signaled=" + as_string( signaled ) );
    return signaled;
}

//---------------------------------------------------------------Process_module_instance::step__end

Variant Process_module_instance::step__end()
{
    Z_LOG2( "scheduler", Z_FUNCTION << "\n" );
    assert( !_spooler );    // Soll nur über Com_remote_module_instance_server gerufen werden

    _process_handle.wait();
    assert( process_has_signaled() );
    end__end();
    close_process();

    if( _order_params_file.path() != "" )
        transfer_back_order_params();       // Vor spooler_process_after(), damit da die geänderten Auftragsparameter verfügbar sind.

    if( _spooler_process_before_called )
        _spooler_process_result = check_result( _monitor_instances.spooler_process_after( _spooler_process_result ) );

    return step_result();
}

//-------------------------------------------------------------Process_module_instance::step_result

string Process_module_instance::step_result()
{
    io::String_writer string_writer;
    xml::Xml_writer   xml_writer   ( &string_writer );

    xml_writer.set_encoding( scheduler_character_encoding );
    xml_writer.write_prolog();

    xml_writer.begin_element( "process.result" );
    {
        xml_writer.set_attribute( "spooler_process_result", _spooler_process_result? "true" : "false" );
        xml_writer.set_attribute( "exit_code", exit_code() );
        if( int s = termination_signal() )  xml_writer.set_attribute( "signal", s );

        xml_writer.set_attribute_optional( "state_text", xml::non_xml_latin1_characters_substituted( get_first_line_as_state_text() ) );
    }

    xml_writer.end_element( "process.result" );
    xml_writer.flush();

    return string_writer.to_string();
}

//--------------------------------------------Process_module_instance::get_first_line_as_state_text

string Process_module_instance::get_first_line_as_state_text()
{
    string result;

    try
    {
        file::File stdout_file ( stdout_path(), "rb" );
        string first_line = stdout_file.read_string( max_stdout_state_text_length );
        stdout_file.close();

        size_t n = first_line.find( '\n' );
        if( n == string::npos )  n = first_line.length();
        if( n > 0  &&  first_line[ n - 1 ] == '\r' )  n--;
        first_line.erase( n );

        result = first_line;
    }
    catch( exception& x )
    {
        _log.warn( message_string( "SCHEDULER-881", x ) );
    }

    return result;
}

//-----------------------------------------------------------------------------Process_event::close
#ifndef Z_WINDOWS

void Process_module_instance::Process_event::close()
{
    // waitpid() rufen, falls noch nicht geschehen (um Zombie zu schließen)

    if( _pid )
    {
        int status = 0;

        if( z::Log_ptr log = "scheduler" )  log << "waitpid(" << _pid << ")  ";

        int ret = waitpid( _pid, &status, WNOHANG | WUNTRACED );    // WUNTRACED: "which means to also return for children which are stopped, and whose status has not been reported."

        if( z::Log_ptr log = "scheduler" )  if( ret == -1 )  log << "ERRNO-" << errno << "  " << strerror(errno) << "\n";
                                                       else  log << "\n";

        _pid = 0;
    }
}

//--------------------------------------------------------------------------Process_event::signaled

bool Process_module_instance::Process_event::signaled()
{
    //LOG2( "zschimmer", "Process_event::signaled()   _signaled=" << _signaled << "\n" );
    if( _signaled )  return true;
    return wait( 0 );
}

//------------------------------------------------------------------------------Process_event::wait

bool Process_module_instance::Process_event::wait( double seconds )
{
    if( !_pid )  return true;

    while(1)
    {
        int status = 0;

      //if( log_ptr )  *log_ptr << "waitpid(" << _pid << ")  ";

        int waitpid_flags = WUNTRACED;                          // WUNTRACED: "which means to also return for children which are stopped, and whose status has not been reported."
        if( seconds != INT_MAX )  waitpid_flags |= WNOHANG;
                            else  Z_LOG( "pid=" << _pid << " waitpid() ...\n" );

        int ret = waitpid( _pid, &status, waitpid_flags );    
        if( ret == -1 )
        {
            Z_LOG2( "scheduler", "waitpid(" << _pid << ") ==> ERRNO-" << errno << "  " << z_strerror(errno) << "\n" );
            //int pid = _pid;
            _pid = 0;
            //throw_errno( errno, "waitpid", as_string(pid).c_str() );
            set_signaled();
            return true;
        }

        if( ret == _pid )
        {
            if( WIFSIGNALED( status ) )
            {
                _process_signaled = WTERMSIG( status );
                //cerr << "signal=" << _process_signaled << "\n";  //test
                _pid = 0;
                set_signaled();
                return true;
            }

            if( WIFEXITED( status ) )
            {
                _process_exit_code = WEXITSTATUS( status );
                //cerr << "exit_code=" << _process_exit_code << "\n";  //test
                _pid = 0;
                set_signaled();
                return true;
            }
        }

      //Z_LOG2( "scheduler", "Prozess läuft noch\n" );

        if( seconds < 0.0001 )  break;

        double w = min( 0.1, seconds );
        seconds -= w;

        struct timespec t;
        t.tv_sec = (time_t)floor( w );
        t.tv_nsec = (time_t)max( 0.0, ( w - floor( w ) ) * 1e9 );

        int err = nanosleep( &t, NULL );
        if( err )
        {
            if( errno == EINTR )  return true;
            throw_errno( errno, "nanosleep" );
        }
    }

    return false;
}

#endif
//--------------------------------------------------------------Process_module_instance::begin__end
#ifndef Z_WINDOWS

bool Process_module_instance::begin__end()
{
    if( _spooler  &&  _spooler->_process_count >= max_processes )  z::throw_xc( "SCHEDULER-210", max_processes );

    if( !_load_called )
    {
        bool ok = implicit_load_and_start();
        if( !ok )  return false;
    }

    fill_process_environment_with_params();

    vector<string> string_args = get_string_args();


    Message_string m ( "SCHEDULER-987" );
    m.set_max_insertion_length( INT_MAX );
    m.insert( 1, posix::shell_command_line_from_argv( string_args ) );
    _log.info( m );

    Z_LOG2( "scheduler", "signal(SIGCHLD,SIG_DFL)\n" );
    ::signal( SIGCHLD, SIG_DFL );                 // Java verändert das Signal-Verhalten, so dass waitpid() ohne diesen Aufruf versagte.

    Z_LOG2( "scheduler", "fork(), execvp(\"" << program_path() << "\")\n" );
    int pid = fork();

    switch( pid )
    {
        case -1:
            throw_errno( errno, "fork" );

        case 0:
        {
            zschimmer::Log_ptr::disable_logging(); // fork() kann gesperrte Mutex übernehmen, was zum Deadlock führt (stimmt das?)
            // Z_LOG() ist jetzt wirkunglos. Kann cerr auch gesperrt sein? Wenigstens ist es unwahrscheinlich, weil cerr kaum benutzt wird.

            setpgid(0,0);   // Neue Prozessgruppe

            if( _module->_priority != "" ) 
            {
                try
                {
                    int error = setpriority( PRIO_PROCESS, getpid(), posix::priority_from_string( _module->_priority ) );
                    if( error )  throw_errno( errno, "setpriority" );
                }
                catch( exception& x ) { cerr << "setpriority(" << _module->_priority << ") ==> ERROR " << x.what() << "\n"; }
            }

            ::signal( SIGINT, SIG_IGN );    // Ctrl-C ignorieren (Darum kümmert sich der Haupt-Prozess)

            dup2( _stdout_file.file_no(), STDOUT_FILENO );
            dup2( _stderr_file.file_no(), STDERR_FILENO );

            int n = sysconf( _SC_OPEN_MAX );
            for( int i = 3; i < n; i++ )  ::close(i);
            //::close( STDIN_FILENO );
            int new_stdin = ::open( "/dev/null", O_RDONLY );
            if( new_stdin != -1  &&  new_stdin != STDIN_FILENO )  dup2( new_stdin, STDIN_FILENO ),  ::close( new_stdin );

            // Arguments

            char** args = new char* [ string_args.size() + 1 ];
            int    i;
            for( i = 0; i < string_args.size(); i++ )  args[i] = (char*)string_args[i].c_str();
            args[i] = NULL;

            // Environment

            Z_FOR_EACH( Com_variable_set::Map, _process_environment->_map, m )
            {
#               if defined Z_HPUX || defined Z_SOLARIS
                    string e = string_from_bstr( m->second->_name ) + "=" + m->second->_value.as_string();
                    Z_LOG2( "env", "putenv(\"" << e << "\")\n" );
                    putenv( strdup( e.c_str() ) );
#                else
                    Z_LOG2( "env", "setenv(\"" << m->second->_name << "\",\"" << m->second->_value.as_string() << "\")\n" );
                    setenv( string_from_bstr( m->second->_name ).c_str(), m->second->_value.as_string().c_str(), true );
#               endif
            }


            Z_LOG2( "scheduler", "execvp(\"" << program_path() << "\")\n" );
            execvp( args[0], args );

            int e = errno;
            
            Z_LOG2( "scheduler", "execvp()  errno-" << e << "  " << z_strerror(e) << "\n" );
            fprintf( stderr, "ERRNO-%d  %s, at execvp(%s", e, strerror(e), args[0] );
            for( int i = 0; args[i]; i++ )  fprintf( stderr, ",%s", quoted_string( args[i] ).c_str() );
            fprintf( stderr, ")\n" );

            int size = 1;
            for( char** e = environ; *e; e++ )  size += strlen( *e ) + 1;
            fprintf( stderr, "environment (%d bytes):\n", size );
            for( char** e = environ; *e; e++ )  fprintf( stderr, "%s\n", *e );

            fflush( stderr );
            _exit( e > 0  &&  e < 250? e : 250 );  // Wie melden wir den Fehler an den rufenden Prozess?
        }

        default:
            Z_LOG2( "scheduler", "pid=" << pid << "\n" );
            _pid = pid;
            _process_handle._pid = pid;
            _process_handle.set_name( S() << "process_handle(pid=" << pid << ")" );
            break;
    }


    //set_state( s_running_process );

    _process_handle.set_name( "Process " + program_path() );

    if( _spooler )
    {
        _process_handle.add_to( &_spooler->_wait_handles );
        _spooler->register_process_handle( _process_handle._pid );
        _pid_to_unregister = _process_handle._pid;
    }

    _stdout_file.close();
    _stderr_file.close();

    //_operation = &dummy_sync_operation;

    return true;
}

//---------------------------------------------------------Process_module_instance::get_string_args

vector<string> Process_module_instance::get_string_args()
{
    vector<string> string_args;

    if( _shell_file.path() != "" )  // <script language="shell">?
    {
        string shell_argument = quoted_unix_command_parameter( program_path() );

        //if( _process_param != "" )    // <shell> kennt nicht param=
        //{
        //    shell_argument += " " + _process_param;
        //}
        //else
        //{
        //    if( _process_environment )
        //    {
        //        for( int i = 1;; i++ )
        //        {
        //            string nr = as_string(i);
        //            Variant vt;
        //            HRESULT hr;

        //            hr = _process_environment->get_Var( Bstr(nr), &vt );
        //            if( FAILED(hr) )  throw_ole( hr, "Variable_set.var", nr.c_str() );

        //            if( vt.vt == VT_EMPTY )  break;

        //            shell_argument += " " + quoted_unix_command_parameter( string_from_variant( vt ) );
        //        }
        //    }
        //}

        string_args.push_back( "/bin/sh" );
        string_args.push_back( "-c" );
        string_args.push_back( shell_argument );
    }
    else 
    {
        if( _process_param != "" )
        {
            string_args = posix::argv_from_command_line( _process_param );
            string_args.insert( string_args.begin(), program_path() );   // argv[0]
        }
        else
        {
            string_args.push_back( program_path() );   // argv[0]

            if( _process_environment )
            {
                for( int i = 1;; i++ )
                {
                    string nr = as_string(i);
                    Variant vt;
                    HRESULT hr;

                    hr = _process_environment->get_Var( Bstr(nr), &vt );
                    if( FAILED(hr) )  throw_ole( hr, "Variable_set.var", nr.c_str() );

                    if( vt.vt == VT_EMPTY )  break;

                    string_args.push_back( string_from_variant( vt ) );
                }
            }
        }
    }

    return string_args;
}

//--------------------------------------------------------------------Process_module_instance::kill

bool Process_module_instance::kill()
{
    bool result = false;

    if( _process_handle._pid  &&  !_is_killed  &&  !_kill_thread )
    {
        _log.warn( message_string( "SCHEDULER-281" ) );   

        Message_string m ( "SCHEDULER-709" );
        m.set_log_level( log_warn );

        ptr<Kill_thread> kill_thread = Z_NEW( Kill_thread );

        kill_thread->_pid          = _process_handle._pid;
        kill_thread->_log          = dynamic_cast<Prefix_log*>( _log.base_log() );
        kill_thread->_message      = m;
        kill_thread->_process_name = _task? _task->obj_name() : "";
        kill_thread->set_thread_name( S() << "Kill_thread pid=" << _process_handle._pid );

        _kill_thread = +kill_thread;
        _kill_thread->thread_start();

        _is_killed = true;
        result = true;
    }

    return result;
}

#endif
//----------------------------------------------------------------Process_module_instance::end__end

void Process_module_instance::end__end()
{
#   ifdef Z_WINDOWS

        DWORD exit_code = 0;

        if( _process_handle )
        {
            BOOL ok = GetExitCodeProcess( _process_handle, &exit_code );
            if( !ok )  throw_mswin_error( "GetExitCodeProcess" );

            if( exit_code == STILL_ACTIVE )  throw_xc( "STILL_ACTIVE", obj_name() );

            close_handle();
        }

#    else

        if( _process_handle._pid )
        {
            step__end();      // waitpid() sollte schon gerufen sein.
            if( _process_handle._pid )   z::throw_xc( "SCHEDULER-179", _process_handle._pid );       // Sollte nicht passieren (ein Zombie wird stehen bleiben)
        }

        close_handle();

        if( _process_handle._process_signaled )
        {
            _termination_signal = _process_handle._process_signaled; 

            /* Siehe Module_task::do_close__end(), Meldung SCHEDULER-279
            try
            {
                z::throw_xc( "SCHEDULER-181", _process_handle._process_signaled );
            }
            catch( const exception& x )
            {
                if( !_module->_process_ignore_signal )  throw;
                _log->warn( x.what() );
            }
            */
        }

        int exit_code = _process_handle._process_exit_code;

#   endif

    _exit_code = (int)exit_code;
    //transfer_exit_code_to_task();
    _spooler_process_result = _exit_code == 0  &&  _termination_signal == 0;;

    _log.log_file( _module->_process_log_filename );

}

//------------------------------------------------------Process_module_instance::termination_signal

//int Process_module_instance::termination_signal()
//{ 
//#   ifdef Z_WINDOWS
//        return 0;
//#    else
//        return _process_handle._process_signaled; 
//#   endif
//}

//------------------------------------Process_module_instance::fill_process_environment_with_params

void Process_module_instance::fill_process_environment_with_params()
{
    // the default prefix for scheduler environment variables is SCHEDULER_PARAM_, but it can overwrite with the scheduler variable
    // SCHEDULER_VARIABLE_NAME_PREFIX. Use SCHEDULER_VARIABLE_NAME_PREFIX=*NONE to use your scheduler environment variables without prefix.
    string prefix;
    if( _spooler && _spooler->variables() )
       prefix = _spooler->variables()->get_string( "SCHEDULER_VARIABLE_NAME_PREFIX" );

    string environment_variable_name_prefix = "SCHEDULER_PARAM_";
    if (!prefix.empty())
       environment_variable_name_prefix = (prefix == "*NONE") ? "" : prefix;

    ptr<Com_variable_set> task_params;
    ptr<Com_variable_set> order_params;

    if( _task ) {
        Z_LOG2( "scheduler", "fill_process_environment_with_params(), _task" << "\n" );
        task_params = _task->params();
        if( _task->order() )  order_params = _task->order()->params();
    } else {
        Variant xml;

        IDispatch* task = object( "spooler_task" );

        vector<Variant> parameters;
        xml = com_invoke( DISPATCH_PROPERTYGET, task, "Params_xml", &parameters );
        task_params = new Com_variable_set();
        task_params->set_xml( xml.as_string() );

        xml = com_invoke( DISPATCH_PROPERTYGET, task, "Order_params_xml", &parameters );
        if( !xml.is_null_or_empty_string() ) {
            order_params = new Com_variable_set();
            order_params->set_xml( xml.as_string() );
        }
    }

    Z_FOR_EACH_CONST( Com_variable_set::Map, task_params->_map, v )  
        _process_environment->set_var( ucase( environment_variable_name_prefix + v->second->name() ), v->second->string_value() );

    if( order_params )
    {
        Z_FOR_EACH_CONST( Com_variable_set::Map, order_params->_map, v ) {
           string environment_variable_name_suffix = get_parameter_name(v->second->name());
           if (environment_variable_name_suffix != "")
              _process_environment->set_var( ucase( environment_variable_name_prefix + environment_variable_name_suffix ), v->second->string_value() );
        }
    }
}

//------------------------------------Process_module_instance::fill_process_environment_with_params

string Process_module_instance::get_parameter_name(const string suffix)
{
   const string state_parameter_delimiter = "/";
   string result = suffix;
   if ( _task ) {
      if (_task->order()) {
         string searchFor = _task->order()->string_state() + state_parameter_delimiter;
         if (result.find(searchFor,0) != string::npos) result.replace(0, searchFor.length(), "");
         if (result.find(state_parameter_delimiter,0) != string::npos) result = "";
      }
   }
   return result;
}

//----------------------------------------------Process_module_instance::transfer_exit_code_to_task

//void Process_module_instance::transfer_exit_code_to_task()
//{
//    if( _task ) 
//        _task->set_exit_code(_exit_code);
//    else {
//        IDispatch* task = object( "spooler_task" );
//        vector<Variant> parameters;
//        parameters.push_back( _exit_code );
//        com_invoke( DISPATCH_PROPERTYPUT, task, "Exit_code", &parameters );
//    }
//}

//----------------------------------------------Process_module_instance::transfer_back_order_params

void Process_module_instance::transfer_back_order_params()
{
    ptr<Com_variable_set> order_parameters = new Com_variable_set();
    fetch_parameters_from_process( order_parameters );

    if( !order_parameters->is_empty() )
    {
        IDispatch* task = object( "spooler_task" );
        vector<Variant> parameters;
        Bstr xml_bstr;
        order_parameters->get_Xml( &xml_bstr );
        parameters.push_back( xml_bstr );
        com_invoke( DISPATCH_PROPERTYPUT, task, "Order_params_xml", &parameters );

        //xml_writer.write_element( order_parameters->dom( "order.params", "param" ).documentElement() );
    }
}

//-------------------------------------------Process_module_instance::fetch_parameters_from_process

void Process_module_instance::fetch_parameters_from_process( Com_variable_set* params )
{
    _order_params_file.open( _order_params_file.path(), "rS" );

    if( _order_params_file.length() > max_memory_file_size )  z::throw_xc( "SCHEDULER-448", _order_params_file.path(), max_memory_file_size / 1024 / 1024, order_params_environment_name );
    
    const char* p     = (const char*)_order_params_file.map();
    const char* p_end = p + _order_params_file.map_length();

    while( p < p_end )
    {
        const char* b = p;
        while( p < p_end  &&  *p != '='  &&  *p != '\n' )  p++;
        if( p >= p_end  ||  *p != '=' )  z::throw_xc( "SCHEDULER-449", order_params_environment_name );
        string name ( b, p++ - b );

        b = p;
        while( p < p_end  &&  *p != '\n' )  p++;
        string value ( b, ( p[-1] == '\r'? p - 1 : p ) - b );
        p++;

        params->set_var( trim( name ), trim( value ) );

        // Wenn letztes '\n' fehlt, ist p == p_end + 1
    }

    _order_params_file.close();
}

//--------------------------------------------------------Process_module_instance::try_delete_files

bool Process_module_instance::try_delete_files( Has_log* log )
{
    bool result = true;

    _stdout_file.close();
    _stderr_file.close();
    _order_params_file.close();

    if( _stdout_file      .is_to_be_unlinked() )  result &= _stdout_file      .try_unlink( log );
    if( _stderr_file      .is_to_be_unlinked() )  result &= _stderr_file      .try_unlink( log );
    if( _order_params_file.is_to_be_unlinked() )  result &= _order_params_file.try_unlink( log );

    return result;
}

//---------------------------------------------------------Process_module_instance::undeleted_files

list<File_path> Process_module_instance::undeleted_files()
{
    list<File_path> result;

    if( _stdout_file      .is_to_be_unlinked() )  result.push_back( _stdout_file      .path() );
    if( _stderr_file      .is_to_be_unlinked() )  result.push_back( _stderr_file      .path() );
    if( _order_params_file.is_to_be_unlinked() )  result.push_back( _order_params_file.path() );

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
