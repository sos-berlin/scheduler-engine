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

Sos_static*                  Sos_static::_sos_static;

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
        if( _log_ptr ) {
            //jz 22.1.2001 hostole stürzt hier ab, offenbar weil iostream::num_put() schon beendet ist:  sos_alloc_log_statistic( _log_ptr );
            *_log_ptr << "~Sos_static_0 fertig\n\n";
        }

        log_stop();

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
        _htask            ( GetCurrentProcess() ),
        _hinstance        (0),
#   endif
    _index      ( 1 ),
    _use_version( "1.5.35" )
{
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

    zschimmer::zschimmer_init();
}

//----------------------------------------------------------------------------Sos_static::close

void Sos_static::close()
{
    //2011-08-24 Wegen Absturzes bei statischem Sos_static nicht: clear_environment_from_sos_ini();

    SOS_DELETE( _std_client );
    SOS_DELETE( _comfile );
    SOS_DELETE( _odbcfile );
    SOS_DELETE( _sosdb );
    SOS_DELETE( _msg_queue_ptr );
    SOS_DELETE( _sosprof );
    SOS_DELETE( _factory_agent );
    SOS_DELETE( _mail );
    SOS_DELETE( _file_type_common_head );
    SOS_DELETE( _init_parameters_ptr );
}

//--------------------------------------------------------------------------Sos_static::use_version

void Sos_static::use_version( const string& version )
{
    Hostware_version v = version;

    if( v > hostware_version )  throw_xc( "SOS-1460", version, hostware_version );
    
    _use_version = v;
}
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

//------------------------------------------------------------------------------Sos_static::version

Hostware_version Sos_static::version()
{
    return hostware_version;
}

} //namespace sos
