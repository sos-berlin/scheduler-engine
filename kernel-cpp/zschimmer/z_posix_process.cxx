// $Id$

#include "zschimmer.h"
#ifdef Z_UNIX

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>

#if !defined Z_AIX
#   include <sys/fcntl.h>
#endif

#include "log.h"
#include "regex_class.h"
#include "z_process.h"
#include "file.h"

using namespace std;
using namespace zschimmer::io;
using zschimmer::file::File;


namespace zschimmer {
namespace posix {

//-------------------------------------------------------------------------kill_process_immediately
// S.a. z_windows_process.cxx

void kill_process_immediately( int pid, const string& debug_string )
{
    //if( kill_descendants )   // No such process?
    //{
    //    Z_LOG( "kill(" << -pid << ",SIGKILL)\n" );
    //    int err = ::kill( -pid, SIGKILL );

    //    if( err )
    //    {
    //        int errn = errno;
    //        Z_LOG( "  ERROR kill() ==> ERRNO-" << errn << "  " << strerror(errn) << "\n" );
    //    }
    //}

    Z_LOG( "kill(" << pid << ",SIGKILL)  " << debug_string << "\n" );
    int err = ::kill( pid, SIGKILL );

    if( err )  { int errn = errno; throw_errno( errn, "kill", as_string(pid).c_str() ); }
}

//-------------------------------------------------------------------kill_process_group_immediately
// S.a. z_windows_process.cxx

void kill_process_group_immediately( int pgid, const string& debug_string )
{
    Z_LOG( "kill(" << -pgid << ",SIGKILL)  " << debug_string << "\n" );
    int err = ::kill( -pgid, SIGKILL );

    if( err )  { int errn = errno; throw_errno( errn, "kill", as_string(-pgid).c_str() ); }
}

//---------------------------------------------------------------try_kill_process_group_immediately

bool try_kill_process_group_immediately( int pid, const string& debug_string )
{
    bool result = true;

    try
    {
        kill_process_group_immediately( pid, debug_string );
    }
    catch( exception& ) 
    {
        result = false;
    }

    return result;
}

//---------------------------------------------------------------------------------------Ps_ef_line

struct Ps_ef_line
{
    static bool pid_ordering( const Ps_ef_line& a, const Ps_ef_line& b ) { return a.pid < b.pid; }

    Ps_ef_line() : pid(0), parent_pid(0) {}

    string      user;
    pid_t       pid;
    pid_t       parent_pid;
    string      command_line;
    list<pid_t> children;
};

typedef stdext::hash_map<pid_t,Ps_ef_line> Pid_map;

//------------------------------------------------------------------try_kill_all_descendants_of_pid

static bool try_kill_all_descendants_of_pid( pid_t pid, Pid_map& pid_map, const string& indent, Has_log* log, const Message_string* msg )
{
    bool result;

    Pid_map::const_iterator it = pid_map.find( pid );
    if( it == pid_map.end() )  
    {
        Z_LOG( Z_FUNCTION << " Unknown pid " << pid << "\n" );
        result = false;
    }
    else
    {
        result = true;

        Z_FOR_EACH_CONST( list<pid_t>, it->second.children, child_pid )
        {
            string command_line = pid_map[ *child_pid ].command_line;

            Message_string m = *msg;
            m.insert_string( 1, as_string( *child_pid ) );
            m.insert_string( 2, indent + command_line );
            if( log && msg ) log->log( msg->log_level(), m );

            result &= try_kill_process_immediately( *child_pid, command_line );
            result &= try_kill_all_descendants_of_pid( *child_pid, pid_map, "  " + indent, log, msg );
        }
    }

    return result;
}

//----------------------------------------------------try_kill_process_with_descendants_immediately

bool try_kill_process_with_descendants_immediately( pid_t pid, Has_log* log, const Message_string* msg, const string& debug_string )
{
    // Bricht alle mit "ps -ef" ermittelten Nachfahren ab.

    bool            result;
    int             pipe_files [2];
    File            pipe_out;
    File            pipe_in;
    vector<string>  args;
    Process         ps_process;
    Pid_map         pid_map;
    
    
    int error = pipe( pipe_files ); 
    if( error )  throw_errno( errno, "pipe" );
    Z_LOG2( "zschimmer", Z_FUNCTION << " pipe() ==> " << pipe_files[0] << " " << pipe_files[1] << "\n" );

    pipe_out.take_fileno( pipe_files[ 1 ] );
    pipe_in .take_fileno( pipe_files[ 0 ] );

    args.push_back( "/bin/ps" );
    args.push_back( "-ef" );
    //args.push_back( "/bin/sh" );
    //args.push_back( "-c" );
    //args.push_back( "sleep 2 && /bin/ps -ef && sleep 10" );

    ps_process.kill_at_end_with( SIGKILL );
    ps_process.set_environment_entry( "LANG", "C" );
    ps_process.set_stdout_handle( pipe_out );
    ps_process.start( args );

    pipe_out.close();

    file::File_base_input_stream input_stream ( &pipe_in );
  //Input_stream_reader          reader       ( &input_stream, &transparent_char_set );
    Transparent_input_stream_reader reader    ( &input_stream );
    Line_reader                  line_reader  ( &reader );

    string line = trim( line_reader.read_line() );    // Überschrift
    //Z_LOG2( "zschimmer", Z_FUNCTION << " " << line << "\n" );


    while(1)
    {
        line = trim( line_reader.read_line() );
        if( line_reader.was_eof() )  break;

        //Z_LOG2( "zschimmer", Z_FUNCTION << " " << line << "\n" );

        vector<string> fields = vector_split( " +", line, 8 );

        Ps_ef_line ps_ef_line;

        ps_ef_line.user         = fields[ 0 ];
        ps_ef_line.pid          = static_cast<pid_t>( zschimmer::as_int64( fields[ 1 ] ) );
        ps_ef_line.parent_pid   = static_cast<pid_t>( zschimmer::as_int64( fields[ 2 ] ) );
        ps_ef_line.command_line = fields[ 7 ];

        pid_map[ ps_ef_line.pid ] = ps_ef_line;
    }

    Z_FOR_EACH( Pid_map, pid_map, p )   // Über die parent_pid die Kinder ermitteln
    {
        Pid_map::iterator it = pid_map.find( p->second.parent_pid );
        if( it != pid_map.end() )  it->second.children.push_back( p->second.pid );
    }

    result = try_kill_process_immediately( pid, debug_string );

    result &= try_kill_all_descendants_of_pid( pid, pid_map, "", log, msg );

    ps_process.wait();

    return result;
}

//-------------------------------------------------------------------------------------------------

const int idle_priority         = +16;
const int below_normal_priority = +6;
const int normal_priority       = 0;
const int above_normal_priority = -6;
const int high_priority         = -16;

//-----------------------------------------------------------------------------priority_from_string
// Siehe auch Process::priority_class()

int priority_from_string( const string& priority )
{
    int result;

    if( priority.length() >= 1  &&  isdigit( (unsigned char)priority[ 0 ] ) )
    {
        return as_int( priority );
    }
    else
    {
        string p = lcase( priority );
        
        if( p == "idle"         )  result = idle_priority;
        else
        if( p == "below_normal" )  result = below_normal_priority;
        else                                                                                                  
        if( p == "normal"       )  result = normal_priority;
        else
        if( p == "above_normal" )  result = above_normal_priority;
        else
        if( p == "high"         )  result = high_priority;
        else
        {
            throw_xc( "Z-4011", priority );
        }
    }

    return result;
}

//---------------------------------------------------------------------------argv_from_command_line
    
vector<string> argv_from_command_line( const string& command_line )
{
    vector<string>       argv;
    const unsigned char* p    = (const unsigned char*)command_line.c_str();

    while(1)
    {
        //fprintf( stderr, "p=%s\n", p );
        while( isspace( p[0] ) )  p++;
        if( p[0] == '\0' )  break;

        string arg;
        arg.reserve( 1000 );

        if( p[0] != '"'  &&  p[0] != '\'' )
        {
            while( p[0] != '\0'  &&  !isspace( p[0] )  &&  p[0] != '"'  &&  p[0] != '\'' )  arg += *p++;
        }

        if( p[0] == '"' )
        {
            p++;

            while( p[0] != '\0'  &&  p[0] != '"' )
            {
                if( p[0] == '\\' ) 
                {
                    p++;
                    if( p[0] == '\0' )  break;
                }

                arg += *p++;
            }

            if( p[0] == '"' )  p++;
        }
        else
        if( p[0] == '\'' )
        {
            p++;

            while( p[0] != '\0'  &&  p[0] != '\'' )
            {
                arg += *p++;
            }

            if( p[0] == '\'' )  p++;
        }

        //fprintf(stderr,"%d=%s.\n", argv.size(), arg.c_str() );
        argv.push_back( arg );
    }

    return argv;
}

//---------------------------------------------------------------------shell_command_line_from_argv
// Erstmal nur zur Debug-Ausgabe

string shell_command_line_from_argv( const vector<string>& args )
{
    // Siehe "man bash" unter DEFINITIONS und QUOTING

    S     result;
    Regex meta_characters_regex ( "[\"'\\ |&;()<>{}*? \t\n]" );   // Sind das alle?

    for( int i = 0; i < args.size(); i++ )
    {
        if( i > 0 )  result << ' ';

        const string& arg = args[ i ];
        
        //Regex_match m = meta_characters_regex.match( arg );
        //fprintf(stderr, "Z_FUNCTION  arg=%s  rm_so=%d rm_eo=%d\n", arg.c_str(), m._match.rm_so, _match.rm_eo );

        //if( !meta_characters_regex.match( arg ) )
        //{
        //    result << arg;
        //}
        //else
        if( arg.find( '\'' ) == string::npos )
        {
            result << '\'' << arg << '\'';
        }
        else
        {
            result << '\'';
            int pos = 0;

            while(1)
            {
                size_t p = arg.find( '\'', pos );
                if( p == string::npos )
                {
                    result.write( arg.data() + pos, arg.length() - pos );
                    result << '\'';
                    break;
                }

                result.write( arg.data() + pos, p - pos );
                result << "'\\'";
                
                pos = p + 1;
                if( pos == arg.length() )  break;

                result << '\'';
            }
        }
        /*
        else
        {
            result << '\"';

            for( int j = 0; j < arg.length(); j++ )
            {
                switch( char c = arg[j] )
                {
                    case '$':
                    case '`':
                    case '\\':
                        result << '\\';
                    default: 
                        result << c;
                }
            }

            result << '\"';
        */
    }

    return result;
}

//---------------------------------------------------------------------------------Process::Process
    
Process::Process()
:
    _zero_(this+1)
{
}

//--------------------------------------------------------------------------------Process::~Process
    
Process::~Process()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG( "Process::close(): " << x << "\n" ); }
}

//-----------------------------------------------------------------------------------Process::close

void Process::close()
{
    if( _kill_at_end_signal  &&  _working_pid )
    {
        if( Log_ptr log = "" )
        {
            log << Z_FUNCTION << " kill(" << _working_pid << "," << _kill_at_end_signal << ")";
            int err = ::kill( _working_pid, _kill_at_end_signal );

            if( err )  log << "  ERRNO-" << errno << " " << strerror( errno );
            log << "\n";
        }

        call_waitpid( true );
    }
    else
    if( _working_pid )  call_waitpid( false );          // Um Zombie zu beenden

    //_pid = 0;
    _working_pid = 0;
    _exit_code = 0;
    _termination_signal = 0;
    _terminated = false;
}

//-----------------------------------------------------------------------------------Process::start

void Process::start( const string& command_line )
{
#   ifndef SYSTEM_WINDOWS
        Z_LOG( "signal(SIGCHLD,SIG_DFL)\n" );
        ::signal( SIGCHLD, SIG_DFL );                 // Java verändert das Signal-Verhalten, so dass waitpid() ohne diesen Aufruf versagte.
#   endif


    start( argv_from_command_line( command_line ) );
}

//-------------------------------------------------------------------------------z_clearenv_solaris
#ifdef Z_SOLARIS

int z_clearenv_solaris()
{
    int             result = 0;
    int             size = 0;
    vector<string>  names;

    for( char** e = environ; *e; e++ )  size++;
    names.reserve( size );
    
    for( char** e = environ; *e; e++ )
    {
        if( const char* eq = strchr( *e, '=' ) )  names.push_back( string( *e, eq - *e ) );
    }

    for( uint i = 0; i < names.size(); i++ )
    {
        string setting = names[i] + "=";
        //Z_LOG( Z_FUNCTION << " putenv( \"" << setting << "\" )\n" );
        int err = putenv( strdup( setting.c_str() ) );
        if( err )  result = err;
    }

    return result;
}

#endif
//---------------------------------------------------------------------------------------z_clearenv

int z_clearenv()
{
#   ifdef Z_SOLARIS
        return z_clearenv_solaris();
#   else
        return clearenv();
#   endif
}

//-----------------------------------------------------------------------------------Process::start

void Process::start( const vector<string>& command )
{
    if( _working_pid )  throw_xc( "Z-PROCESS-002" );
    if( command.size()      == 0 )  throw_xc( "Z-4009" );
    if( command[0].length() == 0 )  throw_xc( "Z-4009" );

    if( _command_line == "" )  _command_line = join( " ", command );

    
    //Z_LOG( "fork(), execv( \"" << command << " )\n" );
    Z_LOG( "fork()\n" );

    _working_pid = fork();
    if( _working_pid == 0 )
    {
        Log_ptr::disable_logging(); // fork() kann gesperrte Mutex übernehmen, was zum Deadlock führt (stimmt das?)
        // Z_LOG() ist jetzt wirkunglos. Kann cerr auch gesperrt sein? Wenigstens ist es unwahrscheinlich, weil cerr kaum benutzt wird.

        try
        {
            if( _own_process_group ) 
            {
                //Z_LOG( "setpgid(0,0)\n" );
                setpgid(0,0);
            }

            {
                //int environment_size = _environment.size();
                //if( _inherit_environment )  for( const char** e = environ; e; e++ )  environment_size++;

                //char** environment = (char**)malloc( environment_size + 1 );
                //char** env_ptr     = environment;

                //if( _inherit_environment )  for( const char* e = environ; e; e++ )  *env_ptr++ = e;
                //*env_ptr++ = NULL;
                //assert( env_ptr - environment <= environment_size );
                //string entry;
                //Z_FOR_EACH( Environment, _environment, e )  entry = e->first + "=" + e->second, *env_ptr += strdup( env.c_str() );

                if( !_inherit_environment )  z_clearenv();
                Z_FOR_EACH( Environment, _environment, e )  set_environment_variable( e->first, e->second );
            }

            if( _priority_set )  setpriority( PRIO_PROCESS, getpid(), _priority );

            if( _stdout_handle != -1 )  dup2( _stdout_handle, STDOUT_FILENO );

            int n = sysconf( _SC_OPEN_MAX );
            for( int i = 3; i < n; i++ )  ::close(i);
            //::close( STDIN_FILENO );
            int new_stdin = ::open( "/dev/null", O_RDONLY );
            if( new_stdin != -1  &&  new_stdin != STDIN_FILENO )  dup2( new_stdin, STDIN_FILENO ),  ::close( new_stdin );




            S log;
            log << "execvp(";

            const char** argv = new const char*[ command.size() + 1 ];
            for( int i = 0; i < command.size(); i++ )
            {
                argv[i] = command[i].c_str();
                log << ",\"" << argv[i] << "\"";
            }
            argv[ command.size() ] = NULL;

            log << ")\n";


            execvp( argv[0], (char**)argv );
            
            fprintf( stderr, "execv errno=%d %s\ncommand=%s", errno, strerror(errno), ((string)log).c_str() );
        }
        catch( exception& x )  { fprintf( stderr, "Exception in forked process: %s\n", x.what() ); }
        catch( ... )  { fprintf( stderr, "Unknown exception in forked process\n" ); }

        _exit(99);
    }
    else
    if( _working_pid == -1 )  
        throw_errno( errno, "fork" );

    _pid = _working_pid;

    Z_LOG( "pid=" << _pid << "\n" );
}

//----------------------------------------------------------------------------Process::set_priority

bool Process::set_priority( int priority ) //, Has_log* log )
{
    try
    {
        if( _pid )
        {
            int error = setpriority( PRIO_PROCESS, _pid, priority );     // oder PRIO_PROCESS? S.a. getpriority()!
            if( error )  throw_errno( errno, "setpriority" );
        }
        else
        {
            _priority = priority;
            _priority_set = true;
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

//--------------------------------------------------------------------------------Process::priority

int Process::priority()
{
    errno = 0;

    if( _pid == 0 )  throw_xc( "Z-4012", "Process.priority()" );

    int result = getpriority( PRIO_PROCESS, _pid );
    if( result == -1  &&  errno != 0 )  throw_errno( errno, "getpriority" );

    return result;
}

//----------------------------------------------------------------------Process::set_priority_class

bool Process::set_priority_class( const string& priority ) //, Has_log* log )
{
    return set_priority( priority_from_string( priority ) );
}

//--------------------------------------------------------------------------Process::priority_class
// Siehe auch priority_from_string()

string Process::priority_class()
{
    int priority = this->priority();

    if( priority >= idle_priority )  return "idle";
    if( priority <= high_priority )  return "high";
    
    if( priority >  0 )  return "below_normal";
    if( priority <  0 )  return "above_normal";
    
    return "normal";
}

//--------------------------------------------------------------------------Process::raise_priority
/*
bool Process::raise_priority( int difference )
{
    return set_priority( priority() - difference );
}
*/
//----------------------------------------------------------------------------Process::call_waitpid

bool Process::call_waitpid( bool wait )
{
    bool ok     = false;
    int  status = 0;

    if( wait )  Z_LOG( "pid=" << _working_pid << " waitpid() ...\n" );
    int ret = waitpid( _working_pid, &status, wait? 0 : WNOHANG );

    int err = ret == -1? errno : 0;
    Z_LOG( "pid=" << _working_pid << " waitpid() => " << ret << " status=" << printf_string( "0x%x", status ) << "\n" );

    if( ret == _working_pid )
    {
        if( WIFEXITED  ( status ) )  _exit_code          = WEXITSTATUS( status ),  ok = true;
        if( WIFSIGNALED( status ) )  _termination_signal = WTERMSIG   ( status ),  ok = true;  

        _working_pid = 0;
        _terminated = true;
    }
    else
    {
        if( ret == -1 )
        {
            Z_LOG( "pid=" << _working_pid << " waitpid()  ERRNO-" << err << "  " << strerror(err) << "\n" );
            if( err == ECHILD )  _working_pid = 0;
        }
    }

    return ok;
}

//------------------------------------------------------------------------------------Process::wait

void Process::wait()
{
    if( _working_pid )
    {
        call_waitpid( true );
    }
}

//------------------------------------------------------------------------------------Process::wait

bool Process::wait( double seconds )
{
    if( _working_pid )
    {
        double until = double_from_gmtime() + seconds;
        struct timespec w;  w.tv_sec = 0, w.tv_nsec = 100*1000*1000;  // 0.1s

        for( int i = 0;; i++ )
        {
            //Z_LOG2( "zschimmer", i << ". call_waitpid(false)\n" );
            if( call_waitpid( false ) )  return true;
            if( seconds <= 0 )  break;
            
            if( i == 20 )  w.tv_sec = 1, w.tv_nsec = 0;  // 1s
            if( double_from_gmtime() >= until )  break;

            nanosleep( &w, NULL );
        }

        return false;
    }
    else
        return true;
}

//------------------------------------------------------------------------------Process::terminated

bool Process::terminated()
{
    if( _working_pid )  call_waitpid( false );
    return _working_pid == 0;
}

//-------------------------------------------------------------------------------Process::exit_code

int Process::exit_code()
{
    assert_terminated();

    return _exit_code;
}

//----------------------------------------------------------------------Process::termination_signal

int Process::termination_signal()
{
    assert_terminated();

    return _termination_signal;
}

//------------------------------------------------------------------------------------Process::kill

void Process::kill( int kill_signal )
{
    if( kill_signal == 0 )  kill_signal = SIGKILL;

    if( _working_pid )
    {
        if( _own_process_group )  kill_process_group_immediately( _working_pid );
                            else  kill_process_immediately( _working_pid );
    }
}

//--------------------------------------------------------------------------------Process::try_kill

bool Process::try_kill( int kill_signal )
{
    if( kill_signal == 0 )  kill_signal = SIGKILL;

    bool result = false;


    if( _working_pid )
    {
        if( _own_process_group )  try_kill_process_group_immediately( _working_pid );
                            else  try_kill_process_immediately( _working_pid );
    }

    return result;
}

//--------------------------------------------------------------------------------Process::obj_name
/*
string Process::obj_name()
{
    return "pid=" + as_string( _working_pid );
}
*/
//-------------------------------------------------------------------------------------------------

} //namespace posix
} //namespace zschimmer


#endif
