#include "precomp.h"
//#define MODULE_NAME "sosstat"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "sysdep.h"

#if defined SYSTEM_WIN
#   include <windows.h>
#endif

#include "sos.h"
#include "sosstat.h"
#include "sosclien.h"
#include "licence.h"
#include "sostimer.h"
#include "log.h"
#include "sosprof.h"
#include "sosstat.h"
#include "env.h"
#include "version.h"

#include "../zschimmer/zschimmer.h"
#include "../zschimmer/log.h"

using namespace std;


namespace sos {

//-------------------------------------------------------------------------------------------static

int                _argc            = 1;
static char*        default_argv0   = "(unknown)";
char**             _argv            = &default_argv0;

z::Mutex            hostware_mutex  ( "hostware" ); // Allgmeine Semaphore für Hostole (und vielleicht andere, z.B. hostphp und C++-Hostware-Programme)
z::Mutex            sosstat_mutex   ("Sos_stat");

//-------------------------------------------------------------------------------------------static

const Hostware_version hostware_version ( VER_PRODUCTVERSION );



#if defined SYSTEM_WINDLL  &&  defined SYSTEM_WIN16
    struct Sos_static_task_entry
    {                                                       // Ohne Konstruktor!
        HTASK                   _htask;
        Sos_static*             _ptr;
    };

    static HGLOBAL                _task_array_handle = 0;      // von GlobalAlloc
    static Sos_static_task_entry* _task_array = 0;             // GMEM_SHARE
    static int                    _task_array_size = 0;        // Anzahl der Einträge
    static int                    _last_entry_index = 0;

  //Sos_static::Mswin Sos_static::mswin;
#else
    Sos_static*                  Sos_static::_sos_static;
    //Sos_static* Sos_static::_sos_static_ptr;
    //Sos_ptr<Sos_static> sos_static_ptr2;
/*
    struct Static_init
    {
        Static_init()
        {
            Sos_static::_sos_static_ptr = new Sos_static;
        }
       ~Static_init()
        {
            delete Sos_static::_sos_static_ptr;
            Sos_static::_sos_static_ptr = 0;  // (SOS_DELETE setzt den Zeiger zu früh auf 0)
        }
    };
    const static Static_init x;
*/
#endif

DEFINE_SOS_STATIC_PTR( Sos_licence )

//-----------------------------------------------------------------------sos_static_ptr_static

Sos_static* sos_static_ptr_static() {
    if (!Sos_static::_sos_static) {
        Z_MUTEX(sosstat_mutex) {
            if (!Sos_static::_sos_static) {
                Sos_static::_sos_static = new Sos_static();
                Sos_static::_sos_static->init0();
                Sos_static::_sos_static->init();
            }
        }
    }
    return Sos_static::_sos_static;
}

//---------------------------------------------------------------------notify_register_callback
/*
extern "C" BOOL FAR PASCAL _export notify_register_callback( WORD id, DWORD )
{
    if( id == NFY_EXITTASK )
    {
        HTASK                  htask = GetCurrentTask();
        Sos_static_task_entry* e     = _task_array;
        Sos_static_task_entry* e_end = _task_array + _task_array_size;

        while( e < e_end )
        {
            if( e->_htask == htask )  break;
            e++;
        }

        if( e == e_end ) {
            LOG_ERR( "notify_register_callback( NFY_EXITTASK ): Task nicht gefunden\n" );
            return 0;
        }

        LOGI( "delete Sos_static\n" );
        delete e->_ptr;
        e->_htask = 0;
        e->_ptr   = 0;
    }

    return 0;
}
*/

//---------------------------------------------------------------------Hostware_version::operator = 

Hostware_version& Hostware_version::operator = ( const string& version )
{
    vector<string> v;
    
    v = zschimmer::vector_split( "\\.", version, 3 );

    int major = v.size() <= 0? 0 : as_int( v[0] );
    int minor = v.size() <= 1? 0 : as_int( v[1] );
    int count = v.size() <= 2? 0 : as_int( v[2] );

    _major = major;
    _minor = minor;
    _count = count;

    return *this;
}

//----------------------------------------------------------------Hostware_version::operator string

Hostware_version::operator string () const
{
    char buffer [100];

    sprintf( buffer, "%d.%d.%d", _major, _minor, _count );

    return buffer;
}

//----------------------------------------------------------------------------Hostware_version::cmp

int Hostware_version::cmp( const Hostware_version& v ) const
{
    if( _major > v._major )  return +1;
    if( _major < v._major )  return -1;

    if( _minor > v._minor )  return +1;
    if( _minor < v._minor )  return -1;

    if( _count > v._count )  return +1;
    if( _count < v._count )  return -1;

    return 0;
}

//-------------------------------------------------------------------Sos_static_0::Sos_static_0

Sos_static_0::Sos_static_0()
:
    _zero_        ( this+1 ),
    _valid        ( true ),
    _log_lock     ( "Sos_static_0::_log_lock", zschimmer::Mutex::kind_recursive_dont_log )
{
    _log_ptr = 0;

#   if defined SYSTEM_WIN
        _log_context._indent_tls_index = TlsAlloc();
#   endif

}

//------------------------------------------------------------------Sos_static_0::~Sos_static_0

Sos_static_0::~Sos_static_0()
{
    close0();
}

//-----------------------------------------------------------------------------Sos_static_0::init0

void Sos_static_0::init0()
{
    _log_context._version   = 1;
    _log_context._mutex     = &_log_lock._system_mutex; 
    _log_context._log_write = Log_ptr::log_write;
    _log_context_ptr  = &_log_context;
    zschimmer::Log_ptr::set_log_context( &sos_static_ptr()->_log_context_ptr );
    //z::Log_ptr::set_indent_tls_index( _log_context._indent_tls_index );

    //_log_ptr = (ostream*)log_create();
}

//-----------------------------------------------------------------------------Sos_static_0::close0

void Sos_static_0::close0()
{
    if( !_valid )  return;
    _valid = false;

    try {
/*
        if( _object_count ) {
#           if 0 //defined SYSTEM_UNIX
                SHOW_ERR( '(' << (int4)_object_count << " interne Objekte übrig)" );
#           else
                LOG_ERR( (int4)_object_count << " Objekte übrig\n" );
#           endif
        }
*/

        //extern int4 sos_alloc_count;
        //extern int4 sos_alloc_size;
        //extern int4 sos_alloc_cache_hits;

        if( _log_ptr ) {
            //jz 22.1.2001 hostole stürzt hier ab, offenbar weil iostream::num_put() schon beendet ist:  sos_alloc_log_statistic( _log_ptr );
            *_log_ptr << "~Sos_static_0 fertig\n\n";
        }

        log_stop();
        //Z_MUTEX( _log_lock ) 
        //{
        //    SOS_DELETE( _log_ptr );
        //}

#       if defined SYSTEM_WIN
            TlsFree( _log_context._indent_tls_index );
            _log_context._indent_tls_index = 0;
#       endif

        zschimmer::zschimmer_terminate();
    }
    catch( const Xc& ) {
        if( _log_ptr )  *_log_ptr << "  (Abbruch wegen Fehlers)\n";
    }
}

//-----------------------------------------------------------------------Sos_static::Sos_static

Sos_static::Sos_static()
:
    _zero_        ( this+1 ),
#   if defined SYSTEM_WIN
#       if defined SYSTEM_WIN32
            _htask            ( GetCurrentProcess() ),
#        else
            _htask            ( GetCurrentTask() ),
#       endif
        _hinstance        (0),
#   endif
    _index      ( 1 ),
    _use_version( "1.5.35" )
{
#   if defined SYSTEM_WINDLL  &&  defined SYSTEM_WIN16
#   else
        //init();
#   endif
}

//----------------------------------------------------------------------Sos_static::~Sos_static

Sos_static::~Sos_static()
{
    //2008-09-22  Folgendes auskommentiert, weil unter hostole.dll am Ende des Prozesses DLLs bereits entladen sein können,
    //2008-09-22  Destruktoren können diese DLLs aufrufen und abstürzen, weil sie ins Leere greifen.
    //
    //if( !_valid )  return;

    //try {
    //    LOGI( "~Sos_static\n" );
    //    close();
    //  //SOS_DELETE( _error_text_file );
    //}
    //catch( const Xc& ) {}
}

//-----------------------------------------------------------------------------Sos_static::init

void Sos_static::init()
{
    set_log_category_default ( "exception.*"    );
    set_log_category_explicit( "exception.D310" );    // Eof_error
    set_log_category_explicit( "exception.D311" );    // Not_found_error


    init_std_client();

    {
        Sos_ptr<Sos_licence> licence = SOS_NEW( Sos_licence );
        _licence = +licence;
        if( !_dont_check_licence )  licence->check();
    }

    {
        Sos_ptr<Sos_timer_manager> timer_manager = SOS_NEW( Sos_timer_manager );
        _timer_manager = +timer_manager;
    }

/*  Funktioniert (in Windows NT), ist aber vorsichtshalber nicht freigegeben:
    {   // jz 11.4.00: Damit libctleasy.dll von EBO gelesen werden kann
        Sos_string add_path = read_profile_string( "", "hostWare", "add-path" );
        if( !empty( add_path ) ) {
            Sos_string setpath = "PATH=";
            setpath += getenv( "PATH" );
#           if defined SYSTEM_WIN
                setpath += ';';
#            else
                setpath += ':';
#           endif
            setpath += add_path;
            putenv( c_str( setpath ) );
        }
    }
*/

    zschimmer::zschimmer_init();
}

//----------------------------------------------------------------------------Sos_static::close

void Sos_static::close()
{
    clear_environment_from_sos_ini();

    SOS_DELETE( _fle370 );
    SOS_DELETE( _hostapi );
    SOS_DELETE( _ddeml_instance_ptr );

    SOS_DELETE( _std_client );

    SOS_DELETE( _sossql );
    SOS_DELETE( _comfile );
    SOS_DELETE( _fs_common );
    SOS_DELETE( _socket_manager_ptr );
    SOS_DELETE( _mswin_msg_window_manager_ptr );
    SOS_DELETE( _odbcfile );
    SOS_DELETE( _sosdb );
    SOS_DELETE( _wbtrint2_ptr );
    SOS_DELETE( _msg_queue_ptr );
    SOS_DELETE( _sosprof );
    SOS_DELETE( _factory_agent );
    SOS_DELETE( _ebo );                     // Eichenauer
    SOS_DELETE( _cache_file );
    SOS_DELETE( _mail );
    SOS_DELETE( _file_type_common_head );
    SOS_DELETE( _type_register );
    SOS_DELETE( _init_parameters_ptr );
    SOS_DELETE( _error_text_file );
}

//--------------------------------------------------------------------------Sos_static::use_version

void Sos_static::use_version( const string& version )
{
    Hostware_version v = version;

    if( v > hostware_version )  throw_xc( "SOS-1460", version, hostware_version );
    
    _use_version = v;

/*
    vector<string> v;
    
    v = zschimmer::vector_split( "\\.", version, 3 );

    _use_version_major = v.size() <= 0? 0 : as_int( v[0] );
    _use_version_minor = v.size() <= 1? 0 : as_int( v[1] );
    _use_version_count = v.size() <= 2? 0 : as_int( v[2] );
*/
}
//----------------------------------------------------------------------Sos_static::get_use_version
/*
string Sos_static::get_use_version()
{
    char buffer [100];

    sprintf( buffer, "%d.%d.%d", _use_version_major, _use_version_minor, _use_version_count );

    return buffer;
}
*/
//------------------------------------------------------------------------Sos_static::since_version

bool Sos_static::since_version( const string& version )
{
    return since_version( Hostware_version( version ) );
}

//------------------------------------------------------------------------Sos_static::since_version

bool Sos_static::since_version( const Hostware_version& version )
{
    return _use_version >= version;
}

//-------------------------------------------------------------------------Sos_static::need_version

void Sos_static::need_version( const string& version )
{
    if( Hostware_version(version) > hostware_version )  throw_xc( "SOS-1460", version, hostware_version );
}

//------------------------------------------------------------------Sos_static::is_version_or_later

bool Sos_static::is_version_or_later( const string& version )
{
    return hostware_version >= Hostware_version(version);
}

//------------------------------------------------------------------------Sos_static::since_version
/*
bool Sos_static::since_version( int major, int minor, int count )
{
    if( _use_version_major > major )  return true;
    if( _use_version_major < major )  return false;

    if( _use_version_minor > minor )  return true;
    if( _use_version_minor < minor )  return false;

    if( _use_version_count > count )  return true;
    if( _use_version_count < count )  return false;

    return true;
}
*/
//------------------------------------------------------------------------------Sos_static::version

Hostware_version Sos_static::version()
{
    return hostware_version;
}

//-----------------------------------------------------------------------Sos_static::static_ptr
#if defined SYSTEM_WINDLL  &&  defined SYSTEM_WIN16

Sos_static* Sos_static::static_ptr()
{
    if( !_task_array ) {
        int s = 100;
        _task_array_handle = GlobalAlloc( GMEM_SHARE, s * sizeof (Sos_static_task_entry) );
        _task_array = (Sos_static_task_entry*)GlobalLock( _task_array_handle );
        if( !_task_array ) {
            SHOW_ERR( "Kein Speicher mehr verfügbar" );  // Folgende Exception scheint sich aufzuhängen
            throw_no_memory_error();
        }
        memset( _task_array, 0, s * sizeof (Sos_static_task_entry) );
        _task_array_size = s;
    }

    HTASK                  htask = GetCurrentTask();
    Sos_static_task_entry* e     = _task_array + _last_entry_index;

    if( htask != e->_htask )     // Nicht der zuletzt verwendete Eintrag?
    {
        Sos_static_task_entry* e_end = _task_array + _task_array_size;

        for( e = _task_array; e < e_end; e++ )  if( e->_htask == htask )  break;

        if( e == e_end )        // Neue Task?
        {
            for( e = _task_array; e < e_end; e++ )  if( !e->_htask )  break;

            if( e == e_end ) {           // Kein freier Eintrag? Array vergrößern
                int s = _task_array_size + 100;
                GlobalUnlock( _task_array_handle );
                _task_array_handle = GlobalReAlloc( (HGLOBAL)_task_array_handle, s, 0 );
                if( !_task_array_handle )  throw_no_memory_error();
                _task_array = (Sos_static_task_entry*)GlobalLock( _task_array_handle );

                memset( _task_array + _task_array_size, 0,
                        ( s - _task_array_size ) * sizeof (Sos_static_task_entry) );

                e = _task_array + _task_array_size;   // Array hat neue Adresse
                _task_array_size = s;
            }

            if( !e->_htask ) {           // Neue Task
                e->_htask = htask;
                e->_ptr   = new Sos_static();
                e->_ptr->_index = 1 + e - _task_array;

                //jz 25.7.97 e->_ptr->init();
                //Absturz: LOGI( "Sos_static( index=" << e->_ptr->_index << " )\n" );
                //NotifyRegister( htask, notify_register_callback, NF_NORMAL );
            }
        }

        _last_entry_index = e - _task_array;   // Diese Task kommt bestimmt gleich wieder
    }

    return e->_ptr;
}

#endif
//---------------------------------------------------------------Sos_static::close_current_task
#if defined SYSTEM_WINDLL  &&  defined SYSTEM_WIN16

void Sos_static::close_current_task()
{
    HTASK                  htask = GetCurrentTask();
    Sos_static_task_entry* e     = _task_array;
    Sos_static_task_entry* e_end = _task_array + _task_array_size;

    while( e < e_end )
    {
        if( e->_htask == htask )  break;
        e++;
    }

    if( e == e_end ) {
        LOG_ERR( "Sos_static::close_current_task: Task nicht gefunden\n" );
        return;
    }

    {
        LOG( "delete Sos_static( index=" << e->_ptr->_index << " )\n" );  // Hier nicht LOGI verwenden!
    }

    delete e->_ptr;
    e->_htask = 0;
    e->_ptr   = 0;
}

//---------------------------------------------------------------Has_static_ptr::Has_static_ptr

Has_static_ptr::Has_static_ptr()
:
    _static_ptr ( ::sos_static_ptr() )
{
}

#endif




} //namespace sos
