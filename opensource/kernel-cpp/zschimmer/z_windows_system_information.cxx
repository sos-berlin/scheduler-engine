// $Id: z_windows_system_information.cxx 13199 2007-12-06 14:15:42Z jz $

#include "zschimmer.h"
#include "system_information.h"
#include "z_com_server.h"

namespace zschimmer {

using namespace zschimmer::com;

//--------------------------------------------------------------------------------get_memory_status

static MEMORYSTATUSEX get_memory_status()
{
    MEMORYSTATUSEX memory_status;
    // dwMemoryLoad
    // dwTotalPhys
    // dwAvailPhys
    // dwTotalPageFile
    // dwAvailPageFile
    // dwTotalVirtual
    // dwAvailVirtual

    //  DWORD dwLength;  DWORD dwMemoryLoad;  DWORDLONG ullTotalPhys;  DWORDLONG ullAvailPhys;  DWORDLONG ullTotalPageFile;  DWORDLONG ullAvailPageFile;  DWORDLONG ullTotalVirtual;  DWORDLONG ullAvailVirtual;  DWORDLONG ullAvailExtendedVirtual;

    memory_status.dwLength = sizeof memory_status;

    BOOL ok = GlobalMemoryStatusEx( &memory_status );
    if( !ok )  throw_mswin( "GlobalMemoryStatusEx" );

    return memory_status;
}

//---------------------------------------------------------------------------------------set_result

static void set_result( VARIANT* result, uint64 value )
{
    // Windows lässt ein VT_I8 verschwinden (jedenfalls JScript)
    //result->vt = VT_I8;
    //V_I8( result ) = (int64)value;

    result->vt = VT_R8;
    V_R8( result ) = (double)value;
}

//----------------------------------------------------------------zschimmer::get_System_information

STDMETHODIMP zschimmer::get_System_information( BSTR what_bstr, VARIANT* parameter_vt, VARIANT* result )
{
    HRESULT hr = S_OK;;

    try
    {
        string what      = string_from_bstr( what_bstr );
        string parameter = string_from_variant( *parameter_vt );

        if( what == "free_disk_space" )
        {
            if( parameter_vt->vt != VT_BSTR )  return DISP_E_TYPEMISMATCH;

            ULARGE_INTEGER large_integer;

            BOOL ok = GetDiskFreeSpaceExW( V_BSTR( parameter_vt ), &large_integer, NULL, NULL );
            if( !ok )  throw_mswin( "GetDiskFreeSpaceExU", parameter );

            set_result( result, large_integer.QuadPart );
        }
        else
        if( what == "disk_space" )
        {
            if( parameter_vt->vt != VT_BSTR )  return DISP_E_TYPEMISMATCH;

            ULARGE_INTEGER large_integer;

            BOOL ok = GetDiskFreeSpaceExW( V_BSTR( parameter_vt ), NULL, &large_integer, NULL );
            if( !ok )  throw_mswin( "GetDiskFreeSpaceExU", parameter );

            set_result( result, large_integer.QuadPart );
        }
        else
        if( what == "total_free_disk_space" )
        {
            if( parameter_vt->vt != VT_BSTR )  return DISP_E_TYPEMISMATCH;

            ULARGE_INTEGER large_integer;

            BOOL ok = GetDiskFreeSpaceExW( V_BSTR( parameter_vt ), NULL, NULL, &large_integer );
            if( !ok )  throw_mswin( "GetDiskFreeSpaceExU", parameter );

            set_result( result, large_integer.QuadPart );
        }
        else
        if( what == "free_memory" )
        {
            set_result( result, get_memory_status().ullAvailPhys );
        }
        else
        if( what == "free_swap" )
        {
            set_result( result, get_memory_status().ullAvailPageFile );
        }
        else
        if( what == "free_virtual" )
        {
            set_result( result, get_memory_status().ullAvailVirtual );
        }
        else
            hr = E_INVALIDARG;
    }
    catch( const exception & x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
