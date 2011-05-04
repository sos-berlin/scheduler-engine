// $Id$

#include "zschimmer.h"
#include "z_windows_window.h"
#include "z_com.h"
#include "log.h"

using namespace std;

namespace zschimmer {
namespace windows {

//-------------------------------------------------------------------------------Window::class_name
    
string Window::class_name()
{
    char buffer [1000+1];
    int length = GetClassName( _handle, buffer, NO_OF( buffer ) );
    return length? buffer : "";
}

//----------------------------------------------------------------------------Window::internal_text

string Window::internal_text()
{
    string   result;
    int      size    = 10000+1;
    OLECHAR* wbuffer = new OLECHAR[ size ];
    int      length  = InternalGetWindowText( _handle, wbuffer, size );

    if( length )  result = com::string_from_ole( wbuffer );

    delete [] wbuffer;
    return result;
}

//-------------------------------------------------------------------------------------Window::text

string Window::text()       
{
    string   result;
    int      size   = 10000+1;
    char*    buffer = new char[ size ];
    int      length = GetWindowText( _handle, buffer, size );      // GetWindowText liefert nicht den Inhalt eines Edit-Controls!

    if( length )  result = buffer;

    delete [] buffer;
    return result;
}

//-------------------------------------------------------------------------------------Window::info

WINDOWINFO Window::info()
{
    WINDOWINFO result;

    BOOL ok = GetWindowInfo( _handle, &result );
    if( !ok )  throw_mswin( "GetWindowInfo" );

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace windows
} //namespace zschimmer
