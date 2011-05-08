#include "precomp.h"
//#define MODULE_NAME "sosbeep"

// sosbeep.cpp									(c) SOS GmbH Berlin

#include "../kram/sysdep.h"

#if defined SYSTEM_WIN
#   include <windows.h>
#elif defined SYSTEM_STARVIEW
#   include <sv.hxx>
#endif

#include "../kram/sosbeep.h"

using namespace std;
namespace sos {


extern void sos_beep()
{
#    if defined SYSTEM_WIN
        MessageBeep( /*-1*/MB_ICONHAND );
#    elif defined SYSTEM_STARVIEW
        Sound::Beep();
#    else
        cerr << '\a';
#    endif
}

} //namespace sos
