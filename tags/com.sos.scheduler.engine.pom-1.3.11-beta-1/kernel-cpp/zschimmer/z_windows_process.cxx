// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "zschimmer.h"
#ifdef Z_WINDOWS

#include "log.h"
#include "z_process.h"
#include "file.h"

#include <tlhelp32.h>


using namespace std;
using namespace zschimmer::file;


namespace zschimmer {
namespace windows {

//-------------------------------------------------------------------------------------------------

struct Process_entry
{
    Process_entry( int pid, const string& path ) : _pid(pid), _path(path) {}
    Process_entry( const PROCESSENTRY32& e ) : _pid(e.th32ProcessID), _path(e.szExeFile) {}

    int                        _pid;
    string                     _path;
};

//-------------------------------------------------------------------------kill_process_immediately

std::vector<Process_entry>      get_child_pids              ( pid_t parent_pid, bool with_descendants = false );

//-------------------------------------------------------------------------kill_process_immediately

void kill_process_immediately( Process_handle handle, const string& debug_string )
{
    string pid_string;

/*
    // GetProcessId() gibt es erst ab XP SP1
    typedef DWORD (WINAPI GetProcessId_function)( HANDLE ); 
    static GetProcessId_function* getProcessId = NULL;

    if( getProcessId == NULL )
    {
        // Wenn mehrere Threads hier gleichzeit reingehen, macht das nichts.

        HINSTANCE module = LoadLibrary( "kernel32.dll" );
        if( module )
        {
            GetProcessId_function* f = (GetProcessId_function*)GetProcAddress( module, "GetProcessId" );
            getProcessId = f? f : (GetProcessId_function*)-1;
        }
    }

    if( getProcessId != NULL  &&  getProcessId != (GetProcessId_function*)-1 )  pid_string = S() << " pid=" << getProcessId( handle );
*/

    Z_LOG( "TerminateProcess("<< (void*)handle << pid_string << ",99)  "  << debug_string << "\n" );
    BOOL ok = TerminateProcess( handle, 99 );
    if( !ok )  throw_mswin( "TerminateProcess" );
}

//-------------------------------------------------------------------------kill_process_immediately

bool try_kill_process_immediately( Process_handle handle, const string& debug_string )
{
    try
    {
        kill_process_immediately( handle, debug_string );
    }
    catch( const exception& x ) 
    { 
        Z_LOG( "FEHLER " << x << "\n" );  
        return false; 
    }

    return true;
}

//-------------------------------------------------------------------------kill_process_immediately
// S.a. z_posix_process.cxx

void kill_process_immediately( pid_t pid, const string& debug_string )
{
    windows::Handle handle = OpenProcess( PROCESS_TERMINATE, false, pid );
    if( !handle )  throw_mswin( "OpenProcess", Z_FUNCTION );

    Z_LOG( "TerminateProcess(pid="<< pid << ")  " << debug_string << "\n" );
    BOOL ok = TerminateProcess( handle, 99 );
    if( !ok )  throw_mswin( "TerminateProcess" );
}

//----------------------------------------------------try_kill_process_with_descendants_immediately

void try_kill_process_with_descendants_immediately( pid_t pid, Has_log* log, const Message_string* msg, const string& debug_string )
{
    windows::Handle handle = OpenProcess( PROCESS_TERMINATE, false, pid );
    if( !handle )  throw_mswin( "OpenProcess", Z_FUNCTION );

    vector<Process_entry> descendants = windows::get_child_pids( pid, true );

    Z_LOGI( "TerminateProcess(pid="<< pid << ")  " << debug_string << "\n" );
    BOOL ok = TerminateProcess( handle, 99 );
    if( ok )  {
        Z_FOR_EACH( vector<Process_entry>, descendants, d ) {
            Message_string m = *msg;
            m.insert_string( 1, as_string( d->_pid ) );
            m.insert_string( 2, d->_path );
            if( log && msg ) log->log( msg->log_level(), m );

            zschimmer::try_kill_process_immediately( d->_pid, debug_string );
        }
    }
}

//-----------------------------------------------------------------------priority_class_from_string
    
int priority_class_from_string( const string& priority )
{
    if( priority.length() >= 1  &&  ( isdigit( (unsigned char)priority[ 0 ] )  ||  priority[ 0 ] == '-' )  )
    {
        int i = as_int( priority );

      //if( i >= 24 )  return REALTIME_PRIORITY_CLASS;
        if( i >= 13 )  return HIGH_PRIORITY_CLASS;
        if( i >= 10 )  return ABOVE_NORMAL_PRIORITY_CLASS;
        if( i >=  8 )  return NORMAL_PRIORITY_CLASS;
        if( i >=  6 )  return BELOW_NORMAL_PRIORITY_CLASS;
                 else  return IDLE_PRIORITY_CLASS;
    }
    else
    {
        string p = lcase( priority );
        
        if( p == "4"   ||  p == "idle"         )  return IDLE_PRIORITY_CLASS;           // 0x00000040   2..6   idle: 1  time critical: 15
        else
        if( p == "6"   ||  p == "below_normal" )  return BELOW_NORMAL_PRIORITY_CLASS;   // 0x00004000   4..8   idle: 1  time critical: 15
        else                                                                                                  
        if( p == "8"   ||  p == "normal"       )  return NORMAL_PRIORITY_CLASS;         // 0x00000020   6..10  idle: 1  time critical: 15
        else
        if( p == "10"  ||  p == "above_normal" )  return ABOVE_NORMAL_PRIORITY_CLASS;   // 0x00008000   8..12  idle: 1  time critical: 15
        else
        if( p == "13"  ||  p == "high"         )  return HIGH_PRIORITY_CLASS;           // 0x00000080   11..15 idle: 1  time critical: 15
        else
      // Realtime verdrängt Maus- und Tastaturroutinen.
      //if( p == "24"  ||  p == "realtime"     )  return REALTIME_PRIORITY_CLASS;       // 0x00000100   2..31  idle: 16 time critical: 31
      //else
        {
            throw_xc( "Z-4011", priority );
        }
    }
}

//-----------------------------------------------------------------------------------get_child_pids

std::vector<Process_entry> get_child_pids( int parent_pid, bool with_descendants )
{
    typedef stdext::hash_map<int,PROCESSENTRY32> Pid_processentry_map;

    windows::Handle      snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    Pid_processentry_map process_entries;

    {
        PROCESSENTRY32 e;
        memset( &e, 0, sizeof e );
        e.dwSize = sizeof e;

        for( BOOL ok = Process32First( snapshot, &e ); ok; ok = Process32Next( snapshot, &e ) )
            process_entries[ e.th32ProcessID ] = e;
    }


    vector<Process_entry>           result;
    stdext::hash_multimap<int,int>  childs;

    Z_FOR_EACH( Pid_processentry_map, process_entries, p ) {
        if( with_descendants )  childs.insert( pair<int,int>( p->second.th32ParentProcessID, p->second.th32ProcessID ) );
        if( p->second.th32ParentProcessID == parent_pid )  result.push_back( Process_entry( p->second ) );
    }


    if( with_descendants ) {
        for( uint i = 0; i < result.size(); i++  ) {
            stdext::hash_multimap<int,int>::iterator c = childs.find( result[ i ]._pid );
            while( c != childs.end()  &&  c->first == result[ i ]._pid ) {
                Pid_processentry_map::iterator p = process_entries.find( c->second );
                if( p != process_entries.end() )  result.push_back( Process_entry( p->second ) );
                c++;
            }
        }
    }

    return result;
}

//---------------------------------------------------------------------------------Process::Process
    
Process::Process()
:
    _zero_(this+1)
{
    init();

    _startup_info.dwFlags    |= STARTF_USESHOWWINDOW;
    _startup_info.wShowWindow = SW_SHOWMINNOACTIVE;         // Als Dienst mit Zugriff auf Desktop wird ein leeres Konsol-Fenster gezeigt. Das minimieren wir.
}

//-----------------------------------------------------------------------------------Process::close

void Process::close()
{
    //_pid = 0;

    try
    {
        Handle::close();
    }
    catch( exception& x ) { Z_LOG( "Process::close(): " << x << "\n" ); }
}

//------------------------------------------------------------------------------------Process::init

void Process::init()
{
    //_with_console_window = true;
}

//-----------------------------------------------------------------------------------Process::start

void Process::start( const std::vector<string>& program_and_parameters )
{
    string cmd_line;

    for( uint i = 0; i < program_and_parameters.size(); i++ )
    {
        if( !cmd_line.empty() )  cmd_line += ' ';
        cmd_line += quoted_windows_process_parameter( program_and_parameters[i] );
    }

    start( cmd_line );
}

//-----------------------------------------------------------------------------------Process::start

void Process::start( const string& cmd_line )
{
    if( _pid )  throw_xc( "Z-PROCESS-002" );


    BOOL                ok;
    DWORD               creation_flags = 0;
    PROCESS_INFORMATION process_info;           memset( &process_info, 0, sizeof process_info );
    File                stdout_file;
    File                stderr_file;

    _startup_info.cb = sizeof _startup_info; 


    _command_line = cmd_line;

    creation_flags |= _priority_class;
  //creation_flags |= CREATE_NO_WINDOW;     // Kein Konsolfenster. Aber stdout und stderr gehen verloren!
  //creation_flags |= DEBUG_PROCESS;


    if( _stdout_path != ""  ||  _stdout_path != "" )
    {
        _startup_info.dwFlags |= STARTF_USESTDHANDLES;
        _startup_info.hStdInput  = GetStdHandle( STD_INPUT_HANDLE );
        _startup_info.hStdOutput = GetStdHandle( STD_OUTPUT_HANDLE );
        _startup_info.hStdOutput = GetStdHandle( STD_ERROR_HANDLE );

        if( _stdout_path != ""  )
        {
            stdout_file.open( _stdout_path, "wI" ); // I: inherit
            _startup_info.hStdOutput = stdout_file.handle();
        }

        if( _stderr_path != "" )
        {
            if( _stderr_path == _stdout_path )  stderr_file.assign_fileno( stdout_file );
                                          else  stderr_file.open( _stderr_path, "wI" );     // I: inherit

            _startup_info.hStdError = stderr_file.handle();
        }
    }


    string      environment;
    const char* e = _environment.size() == 0? NULL : ( environment = make_environment_string(), environment.data() );

    Z_LOG( "CreateProcess(" << cmd_line << ")\n" );
    ok = CreateProcess( NULL,                           // application name
                        const_cast<char*>( cmd_line.c_str() ),    // command line 
                        NULL,                           // process security attributes 
                        NULL,                           // primary thread security attributes 
                        TRUE,                           // handles are inherited?
                        creation_flags,
                        (void*)e,                       // NULL: use parent's environment 
                        NULL,                           // use parent's current directory 
                        &_startup_info,                 // STARTUPINFO pointer 
                        &process_info );                // receives PROCESS_INFORMATION 

    if( !ok )  throw_mswin( "CreateProcess", cmd_line );

    Z_LOG( "pid=" << process_info.dwProcessId << "\n" );
    _pid    = process_info.dwProcessId;
    _handle = process_info.hProcess;

    CloseHandle( process_info.hThread );

    // Ein Versuch, einem abstürzenden Programm die Messagebox auszutreiben:
    //if( creation_flags & DEBUG_PROCESS )  debug();
}

//-----------------------------------------------------------------------------------Process::debug
/*
void Process::debug()
{
    HANDLE      process_file = (HANDLE)0;
    DEBUG_EVENT debug_event;

    while(1)
    {
        ok = WaitForDebugEvent( &debug_event, INFINITE ); 
        if( !ok )  throw_mswin( "WaitForDebugEvent" );

        bool  exit          = false;
        DWORD continue_flag = DBG_CONTINUE;

        switch( debug_event.dwDebugEventCode )
        {
            case CREATE_PROCESS_DEBUG_EVENT:
            {
                process_file = debug_event.u.CreateProcessInfo.hFile;
                break;
            }

            case EXIT_PROCESS_DEBUG_EVENT:
            {
                exit = true;
                break;
            }

            case EXCEPTION_DEBUG_EVENT: 
            {
                continue_flag = DBG_EXCEPTION_NOT_HANDLED;

                if( !debug_event.u.Exception.dwFirstChance ) 
                {
                    Z_LOG( "TerminateProcess() nach Exception\n" );
                    TerminateProcess( _handle, 99 );
                    exit = true;
                }

                break;
            }
            
            default: break;
        }

        ok = ContinueDebugEvent( debug_event.dwProcessId, debug_event.dwThreadId, continue_flag ); 
        if( !ok  &&  !exit )  throw_mswin( "ContinueDebugEvent" );

        if( exit )  break;
    } 

    CloseHandle( process_file );
}
*/
//------------------------------------------------------------------------------Process::attach_pid

void Process::attach_pid( int pid )
{
    close();

    _handle = OpenProcess( PROCESS_TERMINATE | PROCESS_SET_INFORMATION | PROCESS_QUERY_INFORMATION | SYNCHRONIZE, false, pid );
    if( !_handle )  throw_mswin( "OpenProcess", as_string(pid) );

    _pid = pid;
}

//----------------------------------------------------------------------Process::set_priority_class

bool Process::set_priority_class( int priority_class ) //, Has_log* log )
{
    try
    {
        if( _handle )
        {
            Z_LOG( "SetPriorityClass(pid=" << _pid << ",0x" << hex << priority_class << dec << ")\n" );

            DWORD ok = SetPriorityClass( _handle, priority_class );
            if( !ok )  throw_mswin( "SetPriorityClass", as_string( priority_class ) );
        }
        else
        {
            _priority_class = priority_class;

          //_raise_priority_set = false;
        }
     
        return true;
    }
    catch( exception& x )
    {
        //if( log )  log->warn( x.what() );
        Z_LOG( x << '\n' );

        return false;
    }
}

//----------------------------------------------------------------------------Process::set_priority_class

bool Process::set_priority_class( const string& priority_class_string ) //, Has_log* log )
{
    int priority_class = priority_class_from_string( priority_class_string );

    return priority_class? set_priority_class( priority_class ) 
                         : false;
}

//--------------------------------------------------------------------------Process::priority_class

string Process::priority_class()
{
    DWORD priority_class = GetPriorityClass( _handle );
    if( !priority_class )  throw_mswin( "GetPriorityClass" );

    switch( priority_class )
    {
        case IDLE_PRIORITY_CLASS:           return "idle";
        case BELOW_NORMAL_PRIORITY_CLASS:   return "below_normal";
        case NORMAL_PRIORITY_CLASS:         return "normal";
        case ABOVE_NORMAL_PRIORITY_CLASS:   return "above_normal";
        case HIGH_PRIORITY_CLASS:           return "high";
        case REALTIME_PRIORITY_CLASS:       return "realtime";
        default:                            return as_string( priority_class );
    }
}

//----------------------------------------------------------------------------Process::set_priority

bool Process::set_priority( int base_priority ) //, Has_log* log )
{
    int priority_class = priority_class_from_string( as_string( base_priority ) );

    return priority_class? set_priority_class( priority_class ) 
                         : false;
}

//--------------------------------------------------------------------------------Process::priority

int Process::priority()
{
    DWORD priority_class = GetPriorityClass( _handle );
    if( !priority_class )  throw_mswin( "GetPriorityClass" );

    switch( priority_class )
    {
        case IDLE_PRIORITY_CLASS:           return 4;
        case BELOW_NORMAL_PRIORITY_CLASS:   return 6;
        case NORMAL_PRIORITY_CLASS:         return 8;
        case ABOVE_NORMAL_PRIORITY_CLASS:   return 10;
        case HIGH_PRIORITY_CLASS:           return 13;
        case REALTIME_PRIORITY_CLASS:       return 24;
        default:                            return -1;  //?
    }
}

//--------------------------------------------------------------------------Process::raise_priority
/*
bool Process::raise_priority( int difference )
{
    if( _handle )
    {
        return set_priority_class( priority_class() + difference );
    }
    else
    {
        _raise_priority_set = true;
        _raise_priority = difference;

        _priority_set = false;
        return true;
    }
}
*/
//------------------------------------------------------------------------------------Process::wait

void Process::wait()
{
    Z_LOG( "pid=" << _pid << " WaitForSingleObject(infinte)  ...\n" );
    DWORD ret = WaitForSingleObject( _handle, INFINITE );
    Z_LOG( "pid=" << _pid << " WaitForSingleObject()  ret=" <<  ret << "\n" );

    if( ret == WAIT_FAILED )  throw_mswin( "WaitForSingleObject", obj_name() );

    _terminated = true;
}

//------------------------------------------------------------------------------------Process::wait

bool Process::wait( double seconds )
{
    if( !_terminated )
    {
        if( seconds > 0 )  Z_LOG( "pid=" << _pid << " WaitForSingleObject(" << seconds << "s)  ...\n" );
        DWORD ret = WaitForSingleObject( _handle, (int)( seconds * 1000 ) );
        if( seconds > 0 )  Z_LOG( "pid=" << _pid << " WaitForSingleObject()  ret=" <<  ret << "\n" );

        if( ret == WAIT_FAILED )  throw_mswin( "WaitForSingleObject", obj_name() );

        _terminated = ret == WAIT_OBJECT_0;
    }

    return _terminated;
}

//------------------------------------------------------------------------------Process::terminated

bool Process::terminated()
{
    return wait( 0 );

    /*
    DWORD exit_code;
    
    BOOL ok = GetExitCodeProcess( _handle, &exit_code );
    if( !ok )  throw_mswin( "GetExitCodeProcess", obj_name() );

    return exit_code != STILL_ACTIVE;
    */
}

//-------------------------------------------------------------------------------Process::exit_code

int Process::exit_code()
{
    assert_terminated();

    DWORD exit_code;
    
    BOOL ok = GetExitCodeProcess( _handle, &exit_code );
    if( !ok )  throw_mswin( "GetExitCodeProcess", obj_name() );

    //if( exit_code == STILL_ACTIVE )  throw_xc( "Z-PROCESS-001", _pid );

    return exit_code;
}

//------------------------------------------------------------------------------------Process::kill

void Process::kill( int signal )
{
    if( _handle  &&  !_terminated )
    {
        BOOL ok = TerminateProcess( _handle, signal );                  //? signal nehmen wir unter Windows als Exit code.
        if( !ok )
        {
            int error = GetLastError();
            if( error == ERROR_ACCESS_DENIED  &&  terminated() )
            {
                // Prozess hat sich inzwischen selbst beendet.
            }
            else
                throw_mswin( error, "TerminateProcess", obj_name() );
        }
    }
}

//------------------------------------------------------------------------------------Process::kill

bool Process::try_kill( int signal )
{
    bool result = false;

    try
    {
        kill( signal );
        result = true;
    }
    catch( const exception& x ) { Z_LOG( "FEHLER  " << x << "\n" ); }

    return result;
}

//-----------------------------------------------------------------Process::make_environment_string

string Process::make_environment_string()
{
    string result;

    result.reserve( result.length() + 20000 );

    
    // Erst die eigene Umgebung übernehmen

    if( _inherit_environment )
    {
        for( char** e = environ; *e; e++ )
        {
            const char* equal = strchr( *e, '=' );
            if( equal )
            {
                string name ( *e, equal - *e );
                if( _environment.find( name ) == _environment.end() )  result.append( *e, strlen( *e ) + 1 );
            }
            else
            {
                // Fehler
            }
        }
    }


    // Jetzt die neue Umgebung übernehmen

    Z_FOR_EACH( Environment, _environment, e )
    {
        string entry = e->first + "=" + e->second;
        if( result.length() + entry.length() + 1 > result.capacity() )  result.reserve( 2 * result.capacity() + 10000 );
        result.append( entry.c_str(), entry.length() + 1 );
    }


    result += '\0';
    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace windows
} //namespace zschimmer


#endif
