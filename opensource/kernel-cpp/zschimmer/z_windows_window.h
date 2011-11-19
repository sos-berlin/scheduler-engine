// $Id: z_windows_window.h 13199 2007-12-06 14:15:42Z jz $

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
