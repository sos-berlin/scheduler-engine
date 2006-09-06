// $Id$

#include "spooler.h"

using namespace std;

namespace sos {
namespace spooler {

//-------------------------------------------------Process_module_instance::Process_module_instance

Process_module_instance::Process_module_instance( Module* module )
: 
    Module_instance(module),
    _zero_(this+1),
    _process_handle( "process_handle" ),

#   ifdef Z_WINDOWS
        _process_environment( variable_set_from_environment() )   
#    else
        _process_environment( new Com_variable_set() )            // Unix vererbt automatisch die Umgebungsvariablen
#   endif

{
    assert( _module );
    _process_environment->merge( _module->_process_environment );
}

//------------------------------------------------Process_module_instance::~Process_module_instance
    
Process_module_instance::~Process_module_instance()
{
    close_handle();

    if( _stdout_file.is_to_be_unlinked() )
        _stdout_file.try_unlink( _module? &_module->_log : NULL );      // Nicht ins _log, also Task-Log protokollieren, damit mail_on_warning nicht greift. 
                                                                        // Das soll kein Task-Problem sein!
    if( _stderr_file.is_to_be_unlinked() )
        _stderr_file.try_unlink( _module? &_module->_log : NULL );      
}

//------------------------------------------------------------Process_module_instance::close_handle

void Process_module_instance::close_handle()
{
#   ifndef Z_WINDOWS
        if( _pid_to_unregister )
        {
            _spooler->unregister_process_handle( _pid_to_unregister );
            _pid_to_unregister = 0;
        }
#   endif


    if( _process_handle )
    {
#       ifdef Z_WINDOWS
            _spooler->unregister_process_handle( _process_handle );
#       endif

        _process_handle.close();
    }
}

//--------------------------------------------------------------------Process_module_instance::init
    
void Process_module_instance::init()
{
    Module_instance::init();
}

//-------------------------------------------------------------Process_module_instance::attach_task

void Process_module_instance::attach_task( Task* task, Prefix_log* log )
{
    Module_instance::attach_task( task, log );
    
    _process_environment->merge( task->params() );
    _process_environment->set_var( "SCHEDULER_TASK_TRIGGER_FILES", task->trigger_files() );
}

//--------------------------------------------------------------------Process_module_instance::load

bool Process_module_instance::load()
{
    bool ok = Module_instance::load();
    if( !ok )  return ok;

    if( _module->_process_filename == "" )
    {
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
#       else
            _shell_file.open_temporary( File::open_unlink_later );
            int ret = fchmod( _shell_file, 0700 );
            if( ret )  throw_errno( errno, "fchmod", _shell_file.path().c_str() );
#       endif

        //_shell_file.create_temporary();
        //_shell_file.open( _shell_file.filename(), "w" );
        _shell_file.print( _module->_source );
        _shell_file.close();
    }

    return ok;
}

//-------------------------------------------------------------------Process_module_instance::start

void Process_module_instance::start()
{
    Module_instance::start();
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
    z::throw_xc( __FUNCTION__, name );
}

//----------------------------------------------------------------------Process_module_instance::call

Variant Process_module_instance::call( const string& name, bool )
{
    z::throw_xc( __FUNCTION__, name );
}

//------------------------------------------------------------Process_module_instance::program_path

string Process_module_instance::program_path()
{
    return _shell_file.path() != ""? _shell_file.path() :  _module->_process_filename;
}

//-------------------------------------------Process_module_instance::variable_set_from_environment
#ifdef Z_WINDOWS

ptr<Com_variable_set> Process_module_instance::variable_set_from_environment()
{
    ptr<Com_variable_set> result = new Com_variable_set();
    for( char** e = environ; *e; e++ )
    {
        const char* equal = strchr( *e, '=' );
        if( equal )  result->set_var( string( *e, equal - *e ), equal + 1 );
    }

    return result;
}

#endif
//--------------------------------------------------------------Process_module_instance::close__end

void Process_module_instance::close__end()
{
    close_monitor();

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

    Module_instance::close__end();
}

//--------------------------------------------------------------Process_module_instance::begin__end
#ifdef Z_WINDOWS

bool Process_module_instance::begin__end()
{
    if( _spooler->_process_count == max_processes )  z::throw_xc( "SCHEDULER-210", max_processes );

    PROCESS_INFORMATION process_info;
    STARTUPINFO         startup_info;
    BOOL                ok;

    memset( &process_info, 0, sizeof process_info );


    _stdout_file.open_temporary( File::open_unlink_later | File::open_inheritable );
    _stderr_file.open_temporary( File::open_unlink_later | File::open_inheritable );

    memset( &startup_info, 0, sizeof startup_info );
    startup_info.cb          = sizeof startup_info;
    startup_info.dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup_info.hStdInput   = INVALID_HANDLE_VALUE;
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
    Z_FOR_EACH( Com_variable_set::Map, _process_environment->_map, m )
        env << string_from_bstr ( m->second->_name ) << "=" << string_from_variant( m->second->_value ) << '\0';
    env << '\0';

    DWORD creation_flags = 0;
    if( _module->_priority != "" )  creation_flags |= windows::priority_class_from_string( _module->_priority );        // Liefert 0 bei Fehler

    ok = CreateProcess( NULL, //executable_path.c_str(),    // application name
                        (char*)command_line.c_str(),      // command line
                        NULL,                       // process security attributes
                        NULL,                       // primary thread security attributes
                        TRUE,                       // handles are inherited?
                        creation_flags,
                        (char*)((string)env).c_str(),      // use parent's environment
                        NULL,                       // use parent's current directory
                        &startup_info,              // STARTUPINFO pointer
                        &process_info );            // receives PROCESS_INFORMATION
    if( !ok )  throw_mswin_error( "CreateProcess", executable_path );

    CloseHandle( process_info.hThread );

    _pid = process_info.dwProcessId;
    _process_handle.set_handle( process_info.hProcess );
    _process_handle.set_name( "Process " + program_path() );
  //_process_handle.add_to( &_thread->_wait_handles );
    _process_handle.add_to( &_spooler->_wait_handles );

    _spooler->register_process_handle( _process_handle );

#   ifdef Z_WINDOWS
        _stdout_file.close();       // Schließen, damit nicht ein anderer Prozess die Handles erbt und damit das Löschen verhindet (ERRNO-13 Permission denied)
        _stderr_file.close();
#   endif

    return true;
}

//--------------------------------------------------------------------Process_module_instance::kill

bool Process_module_instance::kill()
{
    if( !_process_handle )  return false;
    if( _is_killed )  return false;

    _log.warn( message_string( "SCHEDULER-281" ) );   

    LOG( "TerminateProcess(" <<  _process_handle << ",255)\n" );
    BOOL ok = TerminateProcess( _process_handle, 255 );
    if( !ok )  throw_mswin_error( "TerminateProcess" );

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
    return !process_has_signaled();
}

//-----------------------------------------------------------------------------Process_event::close
#ifndef Z_WINDOWS

void Process_module_instance::Process_event::close()
{
    // waitpid() rufen, falls noch nicht geschehen (um Zombie zu schließen)

    if( _pid )
    {
        int status = 0;

        if( log_ptr )  *log_ptr << "waitpid(" << _pid << ")  ";

        int ret = waitpid( _pid, &status, WNOHANG | WUNTRACED );    // WUNTRACED: "which means to also return for children which are stopped, and whose status has not been reported."

        if( log_ptr )  if( ret == -1 )  *log_ptr << "ERRNO-" << errno << "  " << strerror(errno) << endl;
                                  else  *log_ptr << endl;

        _pid = 0;
    }
}

//--------------------------------------------------------------------------Process_event::signaled

bool Process_module_instance::Process_event::signaled()
{
    //LOG2( "joacim", "Process_event::signaled()   _signaled=" << _signaled << "\n" );
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

        int ret = waitpid( _pid, &status, WNOHANG | WUNTRACED );    // WUNTRACED: "which means to also return for children which are stopped, and whose status has not been reported."
        if( ret == -1 )
        {
            LOG( "waitpid(" << _pid << ") ==> ERRNO-" << errno << "  " << z_strerror(errno) << "\n" );
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
              //LOG( "signal=" << _process_exit_code << "\n" );
                _pid = 0;
                set_signaled();
                return true;
            }

            if( WIFEXITED( status ) )
            {
                _process_exit_code = WEXITSTATUS( status );
              //LOG( "exit_code=" << _process_exit_code << "\n" );
                _pid = 0;
                set_signaled();
                return true;
            }
        }

      //LOG( "Prozess läuft noch\n" );

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
    if( _spooler->_process_count == max_processes )  z::throw_xc( "SCHEDULER-210", max_processes );

    vector<string> string_args;

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

    _stdout_file.open_temporary( File::open_unlink_later | File::open_inheritable );
    _stderr_file.open_temporary( File::open_unlink_later | File::open_inheritable );

    LOG( "signal(SIGCHLD,SIG_DFL)\n" );
    ::signal( SIGCHLD, SIG_DFL );                 // Java verändert das Signal-Verhalten, so dass waitpid() ohne diesen Aufruf versagte.

    if( log_ptr )  *log_ptr << "fork()  ";
    int pid = fork();

    switch( pid )
    {
        case -1:
            throw_errno( errno, "fork" );

        case 0:
        {
            if( _module->_priority != "" ) 
            {
                try
                {
                    int error = setpriority( PRIO_PROCESS, getpid(), posix::priority_from_string( _module->_priority ) );
                    if( error )  throw_errno( errno, "setpriority" );
                }
                catch( exception& x ) { Z_LOG( "setpriority(" << _module->_priority << ") ==> ERROR " << x.what() << "\n" ); }
            }

            ::signal( SIGINT, SIG_IGN );    // Ctrl-C ignorieren (Darum kümmert sich der Haupt-Prozess)

            dup2( _stdout_file._file, STDOUT_FILENO );
            dup2( _stderr_file._file, STDERR_FILENO );

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
                    putenv( strdup( e.c_str() ) );
#                else
                    setenv( string_from_bstr( m->second->_name ).c_str(), m->second->_value.as_string().c_str(), true );
#               endif
            }


            Z_LOG( "execvp(\"" << program_path() << "\")\n" );
            execvp( program_path().c_str(), args );

            int e = errno;
            Z_LOG( "execvp()  errno-" << e << "  " << z_strerror(e) << "\n" );
            fprintf( stderr, "ERRNO-%d  %s, bei execlp(\"%s\")\n", e, strerror(e), program_path().c_str() );
            _exit( e? e : 250 );  // Wie melden wir den Fehler an den rufenden Prozess?
        }

        default:
            LOG( "pid=" << pid << "\n" );
            _pid = pid;
            _process_handle._pid = pid;
            _process_handle.set_name( S() << "process_handle(pid=" << pid << ")" );
            break;
    }


    //set_state( s_running_process );

    _process_handle.set_name( "Process " + program_path() );
    _process_handle.add_to( &_spooler->_wait_handles );

    _spooler->register_process_handle( _process_handle._pid );
    _pid_to_unregister = _process_handle._pid;

    //_operation = &dummy_sync_operation;

    return true;
}

//--------------------------------------------------------------------Process_module_instance::kill

bool Process_module_instance::kill()
{
    if( _is_killed )  return false;

    if( _process_handle._pid )
    {
        _log.warn( message_string( "SCHEDULER-281" ) );   

        LOG( "kill(" << _process_handle._pid << ",SIGTERM)\n" );
        int err = ::kill( _process_handle._pid, SIGTERM );
        if( err )  throw_errno( errno, "killpid" );

        //? _process_handle._pid = 0;
        _is_killed = true;
        return true;
    }

    return false;
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

        /* Siehe Module_task::do_close__end(), Meldung SCHEDULER-279
        if( _process_handle._process_signaled )
        {
            try
            {
                z::throw_xc( "SCHEDULER-181", _process_handle._process_signaled );
            }
            catch( const exception& x )
            {
                if( !_module->_process_ignore_signal )  throw;
                _log->warn( x.what() );
            }
        }
        */

        int exit_code = _process_handle._process_exit_code;

#   endif

    _exit_code = (int)exit_code;

    _log.log_file( _module->_process_log_filename );

    /* siehe Module_task::do_close__end(), MeldungSCHEDULER-280
    if( exit_code )
    {
        try
        {
            z::throw_xc( "SCHEDULER-126", exit_code );
        }
        catch( const exception& x )
        {
            if( !_module->_process_ignore_error )  throw;
            _log.warn( x.what() );
        }
    }
    */
}

//------------------------------------------------------Process_module_instance::termination_signal

int Process_module_instance::termination_signal()
{ 
#   ifdef Z_WINDOWS
        return 0;
#    else
        return _process_handle._process_signaled; 
#   endif
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

