#include "precomp.h"
//#define MODULE_NAME "sosmswin"
/*                                              (c) SOS GmbH Berlin
*/

#include "../kram/sysdep.h"
#if defined SYSTEM_WIN

#if defined SYSTEM_BORLAND
#   include <dir.h>            // MAX_PATH
#endif

#if defined SYSTEM_MICROSOFT
#   pragma warning( disable:4201 )  // warning C4201: nonstandard extension used : nameless struct/union
#endif

#ifndef STRICT
#   define STRICT
#endif
//#if defined SYSTEM_STARVIEW
//    #include <svwin.h>
// #else
#   include <windows.h>
//#endif

#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/sosmswin.h"

using namespace std;
namespace sos {

extern HINSTANCE _hinstance;

//---------------------------------------------------------------------------------Mswin::hinstance

HINSTANCE Mswin::hinstance()
{
    // Hier nicht LOG aufrufen!
  //return Sysdepen::GethInst();
    return _hinstance;
}

//-----------------------------------------------------Mswin::Callback_instance::~Callback_instance

Mswin::Callback_instance::~Callback_instance()
{
    if( _instance ) {
        FreeProcInstance( _instance );
    }
}

//-------------------------------------------------------Mswin::Callback_instance::operator FARPROC

Mswin::Callback_instance::operator FARPROC ()
{
    if( !_instance ) {
        _instance = MakeProcInstance( _procedure, Mswin::hinstance() );
        if( !_instance ) {
            throw( Mswin::Error( "MakeProcInstance" ));
        }
    }
    return _instance;
}

//--------------------------------------------------------------------operator << ( HINSTANCE )

ostream& operator << ( ostream& s , HINSTANCE hinstance )
{
    char name [ MAXPATH + 1 ];
    int len = GetModuleFileName( hinstance, name, sizeof name );
    if( len )  s << name;
    else 
    {
        s << "HINSTANCE=" << (void*)hinstance;

#   if defined SYSTEM_WIN32
        long         error = GetLastError();
        char         code  [ 20 ];
        Dynamic_area text ( 500 );

        sprintf( code, "MSWIN-%08lX", (long)error );

        text.char_ptr()[ 0 ] = '\0';

        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,
                       NULL,
                       error,
                       MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), //The user default language
                       text.char_ptr(), text.size(), NULL );

        char* p = text.char_ptr() + text.length();
        if( p > text.char_ptr()  &&  p[-1] == '\n' )  *--p = '\0';
        if( p > text.char_ptr()  &&  p[-1] == '\r' )  *--p = '\0';

        s << "[Fehler " << code << ' ' << text.char_ptr() << ']';
#   endif
    }

    return s;
}

} //namespace sos

#endif
