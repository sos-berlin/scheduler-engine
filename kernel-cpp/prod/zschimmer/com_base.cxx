// $Id$

#ifndef _WIN32

#include "zschimmer.h"

//--------------------------------------------------------------------------------------------const

extern DEFINE_GUID( GUID_NULL               , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
extern DEFINE_GUID( IID_NULL                , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
extern DEFINE_GUID( CLSID_NULL              , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
extern DEFINE_GUID( IID_invalid_for_uuidof  , 0x0BADF00D, 0x0BAD, 0x0BAD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF );

extern DEFINE_GUID( IID_IUnknown            , 0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 );
extern DEFINE_GUID( IID_IDispatch           , 0x00020400, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 );

//-------------------------------------------------------------------------------hresult_from_win32

HRESULT hresult_from_win32( LONG x ) 
{ 
    return x <= 0 ? x 
                  : x & 0x0000FFFF | ( (HRESULT)FACILITY_WIN32 << 16 ) | 0x80000000;
}

//-------------------------------------------------------------------------------------------------

#endif

