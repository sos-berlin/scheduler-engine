// $Id$

#include "zschimmer.h"
#include "system_information.h"
#include "z_com_server.h"

#ifdef Z_HPUX
#   define _LARGEFILE64_SOURCE         // Für statvfs64
#   include <sys/param.h>
#   include <sys/pstat.h>
#endif

#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>


namespace zschimmer {

using namespace zschimmer::com;

//-------------------------------------------------------------------------------------call_statvfs

static struct statvfs64 call_statvfs( const string& path )
{
    struct statvfs64  result;

    //unsigned long int     f_bsize;
    //unsigned long int     f_frsize;
    //__fsblkcnt64_t        f_blocks;
    //__fsblkcnt64_t        f_bfree;
    //__fsblkcnt64_t        f_bavail;
    //__fsfilcnt64_t        f_files;
    //__fsfilcnt64_t        f_ffree;
    //__fsfilcnt64_t        f_favail;
    //unsigned long int     f_fsid;
    //int                   __f_unused;
    //unsigned long int     f_flag;
    //unsigned long int     f_namemax;
    //int                   __f_spare[6];

    int error = statvfs64( path.c_str(), &result );
    if( error )  throw_errno( errno, "statvfs", path.c_str() );

    return result;
}

//-------------------------------------------------------------------------------------call_sysinfo
#ifdef Z_LINUX

static struct sysinfo call_sysinfo()
{
    struct sysinfo  result;
    memset( &result, 0, sizeof result );

    int error = sysinfo( &result );
    if( error )  throw_errno( errno, "sysinfo" );

    return result;
}

#endif
//-----------------------------------------------------------------------------call_pstat_getvminfo
#ifdef Z_HPUX

static struct pst_vminfo call_pstat_getvminfo()
{
    struct pst_vminfo vminfo;

    errno = 0;
    int count = pstat_getvminfo( &vminfo, sizeof vminfo, 1, 0 );
    if( count < 1 )  throw_errno( errno, "pstat_getvminfo" );

    return vminfo;
}

#endif
//-----------------------------------------------------------------------------call_pstat_getstatic
#ifdef Z_HPUX
/*
static struct pst_static call_pstat_getstatic()
{
    struct pst_static result;

    errno = 0;
    int count = pstat_getstatic( &result, sizeof result, 1, 0 );
    if( count < 1 )  throw_errno( errno, "pstat_getstatic" );

    return result;
}
*/
#endif
//---------------------------------------------------------------------------------------set_result

static void set_result( VARIANT* result, uint64 value )
{
    result->vt = VT_I8;
    V_I8( result ) = (int64)value;
}

//----------------------------------------------------------------zschimmer::get_System_information

STDMETHODIMP get_System_information( BSTR what_bstr, VARIANT* parameter_vt, VARIANT* result )
{
    HRESULT hr = S_OK;;

    try
    {
        string what      = string_from_bstr( what_bstr );
        string parameter = string_from_variant( *parameter_vt );

#if defined Z_LINUX || defined Z_SOLARIS || defined Z_HPUX || defined Z_AIX
        if( what == "disk_space" )
        {
            struct statvfs64  s = call_statvfs( parameter );
            set_result( result, s.f_blocks * s.f_frsize );
        }
        else
        if( what == "free_disk_space" )
        {
            struct statvfs64  s = call_statvfs( parameter );
            set_result( result, s.f_bavail * s.f_frsize );
        }
        else
        if( what == "total_free_disk_space" )
        {
            struct statvfs64  s = call_statvfs( parameter );
            set_result( result, s.f_bfree * s.f_frsize );
        }
        else
#endif

#if defined Z_LINUX || defined Z_SOLARIS || defined Z_AIX
        if( what == "free_memory" )
        {
            set_result( result, (int64)sysconf( _SC_AVPHYS_PAGES ) * sysconf( _SC_PAGESIZE ) );
        }
        else
#elif defined Z_HPUX
        if( what == "free_memory" )
        {
            set_result( result, (int64)call_pstat_getvminfo().psv_cfree * sysconf( _SC_PAGE_SIZE ) );

            // Größe des Arbeitsspeichers:
            //pst_static s = call_pstat_getstatic();
            //set_result( result, (int64)s.physical_memory * s.page_size );
        }
        else
#endif

#if defined Z_LINUX 
        if( what == "free_swap" )
        {
            set_result( result, (int64)call_sysinfo().freeswap );
        }
        else
#elif defined Z_HPUX
        if( what == "free_swap" )
        {
            int                 total_free_swap_pages = 0;
            struct pst_swapinfo swapinfo;
            int                 i = 0;

            while( pstat_getswap( &swapinfo, sizeof swapinfo, 1, i++ ) == 1 )
                total_free_swap_pages += swapinfo.pss_nfpgs;

            set_result( result, (int64)total_free_swap_pages * sysconf( _SC_PAGE_SIZE ) );
        }
        else
#endif
      //if( what == "free_virtual" )
      //{
      //    set_result( result, (int64)call_sysinfo().totalswap * sysconf( _SC_PAGESIZE ) );        // + freeram?
      //}
      //else
            hr = E_INVALIDARG;
    }
    catch( const exception & x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
