// $Id$

#ifndef __ZSCHIMMER_WINDOWS_WINDOW_H
#define __ZSCHIMMER_WINDOWS_WINDOW_H


#include "z_windows.h"


namespace zschimmer {
namespace windows {

//-------------------------------------------------------------------------------------------Window

struct Window
{
                                Window                      ( HWND window = NULL )          : _handle(window) {}

    Window&                     operator =                  ( HWND window )                 { _handle = window;  return *this; }
                                operator HWND               () const                        { return _handle; }

    string                      class_name                  ();
    string                      internal_text               ();
    string                      text                        ();
    WINDOWINFO                  info                        ();


    HWND                       _handle;
};

//-------------------------------------------------------------------------------------------------

} //namespace windows
} //namespace zschimmer

#endif
