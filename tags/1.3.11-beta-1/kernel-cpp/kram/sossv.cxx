#include "precomp.h"
//#define MODULE_NAME "sossv"
/* sossv.cpp                                            (c) SOS GmbH Berlin
                                                            Joacim Zschimmer

Schnittstelle zwischen soswin.h und StarView (sv.hxx).
Versuch, alle Aufrufe von StarView hier zu konzentrieren, so daß alle
anderen Module frei von StarView sind.

*/

#include "sysdep.h"

#if defined SYSTEM_WIN  &&  defined SYSTEM_STARVIEW         // Dieses Modul benötigt StarView

#include <string.h>
#include <ctype.h>

#if defined SYSTEM_WIN
#   include <svwin.h>
#endif
#if defined __BORLANDC__
#   include <cstring.h>
#   include <except.h>
#endif


#include "../kram/sysdep.hxx"
#include "../kram/sv.hxx"
#include "../kram/sos.h"
#include "../kram/sosappl.h"
#include "../kram/log.h"
#include "../kram/sossv.h"

using namespace std;
namespace sos {


struct Sos_window_sv
{
                                Sos_window_sv   ();
                               ~Sos_window_sv   ();
    void                        handle_key_event( const Key_event& );
};


struct __huge Sos_application_sv : Abs_sos_application,
                                   Application /*StarView*/
{
                                Sos_application_sv();
                               ~Sos_application_sv();

    virtual void                Main            ( int, char*[] );
    void                        UserEvent       ( ULONG event, void* data );
    virtual void                user_event_function( User_event_function* f ) { _user_event_function = f; }

  protected:
    virtual void                _main           ( int argc, char* argv[] );

    virtual void                _execute        ();
    virtual Reschedule_status   _reschedule     ();
    virtual void                _busy           ( Bool );

  private:
    int                        _busy_depth;
    User_event_function*       _user_event_function;
};

//--------------------------------------------------------------------Variablen

#if !defined SYSTEM_SOLARIS
extern Sos_application  sos_application;        // Das zu implementierende Anwendungsobjekt
Sos_application* const  sos_application_ptr = &sos_application;
#endif
Sos_application_sv      aMyApp;                 // Name ist von StarView festgelegt

#ifdef MAC
    Application* pApp = &aMyApp;
#endif

//-----------------------------------------------------------Key_code::Key_code

Key_code::Key_code( USHORT sv_key_code, USHORT sv_modifier, char c )
 :  _char( c )
{
    static const struct
    {
        USHORT          sv_key_code;
        Key_code::Value sos_value;
    } 
    tab [] = {
        { KEY_0         ,  '0'                    },
        { KEY_1         ,  '1'                    },
        { KEY_2         ,  '2'                    },
        { KEY_3         ,  '3'                    },
        { KEY_4         ,  '4'                    },
        { KEY_5         ,  '5'                    },
        { KEY_6         ,  '6'                    },
        { KEY_7         ,  '7'                    },
        { KEY_8         ,  '8'                    },
        { KEY_9         ,  '9'                    },

        { KEY_A         ,  'A'                    },
        { KEY_B         ,  'B'                    },
        { KEY_C         ,  'C'                    },
        { KEY_D         ,  'D'                    },
        { KEY_E         ,  'E'                    },
        { KEY_F         ,  'F'                    },
        { KEY_G         ,  'G'                    },
        { KEY_H         ,  'H'                    },
        { KEY_I         ,  'I'                    },
        { KEY_J         ,  'J'                    },
        { KEY_K         ,  'K'                    },
        { KEY_L         ,  'L'                    },
        { KEY_M         ,  'M'                    },
        { KEY_N         ,  'N'                    },
        { KEY_O         ,  'O'                    },
        { KEY_P         ,  'P'                    },
        { KEY_Q         ,  'Q'                    },
        { KEY_R         ,  'R'                    },
        { KEY_S         ,  'S'                    },
        { KEY_T         ,  'T'                    },
        { KEY_U         ,  'U'                    },
        { KEY_V         ,  'V'                    },
        { KEY_W         ,  'W'                    },
        { KEY_X         ,  'X'                    },
        { KEY_Y         ,  'Y'                    },
        { KEY_Z         ,  'Z'                    },

        { KEY_F1        ,  Key_code::f1           },
        { KEY_F2        ,  Key_code::f2           },
        { KEY_F3        ,  Key_code::f3           },
        { KEY_F4        ,  Key_code::f4           },
        { KEY_F5        ,  Key_code::f5           },
        { KEY_F6        ,  Key_code::f6           },
        { KEY_F7        ,  Key_code::f7           },
        { KEY_F8        ,  Key_code::f8           },
        { KEY_F9        ,  Key_code::f9           },
        { KEY_F10       ,  Key_code::f10          },
        { KEY_F11       ,  Key_code::f11          },
        { KEY_F12       ,  Key_code::f12          },
        { KEY_F13       ,  0                      },
        { KEY_F14       ,  0                      },
        { KEY_F15       ,  0                      },
        { KEY_F16       ,  0                      },
        { KEY_F17       ,  0                      },
        { KEY_F18       ,  0                      },
        { KEY_F19       ,  0                      },
        { KEY_F20       ,  0                      },
        { KEY_F21       ,  0                      },
        { KEY_F22       ,  0                      },
        { KEY_F23       ,  0                      },
        { KEY_F24       ,  0                      },

        { KEY_DOWN      ,  Key_code::down         },
        { KEY_UP        ,  Key_code::up           },
        { KEY_LEFT      ,  Key_code::left         },
        { KEY_RIGHT     ,  Key_code::right        },
        { KEY_HOME      ,  Key_code::home         },
        { KEY_END       ,  Key_code::end          },
        { KEY_PAGEUP    ,  Key_code::page_up      },
        { KEY_PAGEDOWN  ,  Key_code::page_down    },

        { KEY_RETURN    ,  '\r'                   },
        { KEY_ESCAPE    ,  '\x1B'                 },
        { KEY_TAB       ,  '\t'                   },
        { KEY_BACKSPACE ,  '\b'                   },
        { KEY_SPACE     ,  ' '                    },
        { KEY_INSERT    ,  Key_code::insert       },
        { KEY_DELETE    ,  Key_code::del          },
    };

    for( int i = 0; i < NO_OF( tab ); i++ ) {
        if( tab[ i ].sv_key_code == sv_key_code )  break;
    }

    static char *alt_gr_codes = "@æ|\\~}][{ýü";
// VORSICHT: folgendes gilt nur fuer deutsche Tastaturen!!!
    if ( sv_modifier & KEY_MOD1 &&
         sv_modifier & KEY_MOD2 &&
         strchr( alt_gr_codes, _char ) != 0 ) {
      // StarView liefert CTRL-ALT fuer ALT-GR !!!
      _value = 0;
    } else {
      _value =  ( i < NO_OF( tab )        ? tab[ i ].sos_value : 0 )
              | ( sv_modifier & KEY_SHIFT ? Key_code::shift    : 0 )
              | ( sv_modifier & KEY_MOD1  ? Key_code::cntrl    : 0 )
              | ( sv_modifier & KEY_MOD2  ? Key_code::alt      : 0 );
    };

    if( sv_modifier == KEY_MOD2
     || _char == '\t'  &&  shift_pressed() )  _char = 0;
}

//----------------------------------------------------------Key_code::printable

Bool Key_code::printable() const
{
    return   ( isprint( _char )  ||  ( (Byte)_char >= 0xA0 && (Byte)_char <= 0xFE ) )
        && ! ( _value & ( cntrl | alt ) );
}

//---------------------------------------------------------Key_event::Key_event

Key_event::Key_event( const KeyEvent& sv_ke )
 :  _count    ( sv_ke.GetRepeat() + 1/*??*/ ),
    _key_code ( sv_ke.GetKeyCode().GetCode(), sv_ke.GetKeyCode().GetModifier(), sv_ke.GetCharCode() )
{
}

//-------------------------------------------------Sos_window_sv::Sos_window_sv

Sos_window_sv::Sos_window_sv()
{
}

//-------------------------------------------Sos_application::window_system_ptr

Abs_sos_application* Sos_application::window_system_ptr()
{
    return &aMyApp;
}

//---------------------------------------Sos_application_sv::Sos_application_sv

Sos_application_sv::Sos_application_sv()
  : _busy_depth ( 0 ),
    _user_event_function( 0 )
{
}

//--------------------------------------Sos_application_sv::~Sos_application_sv

Sos_application_sv::~Sos_application_sv()
{
}

//----------------------------------------------------------------Sos_application_sv::UserEvent

void Sos_application_sv::UserEvent( ULONG event, void* data )
{
    if( _user_event_function )  (*_user_event_function)( event, data );
}

//-----------------------------------------------------------------Sos_application_sv::Main
#if !defined SYSTEM_SOLARIS

extern HINSTANCE _hinstance;     //jz 25.7.96??  = 0;

void Sos_application_sv::Main( int argc, char* argv[] )
{
    _hinstance = Sysdepen::GethInst();

    Sos_appl appl ( false );

    try {
        appl.init();

        sos_application.main( argc, argv );
/*
        if( exception() ) {
            SHOW_EXCEPTION( "" );
            return;
        }
*/
    }

    catch( const Xc& x )
    {
        SHOW_ERR( "Fehler " << x );
    }

#   if defined __BORLANDC__
        catch( const xmsg& x )
        {
            SHOW_ERR( "Fehler "    << __throwExceptionName
                   << "( "         << x.why().c_str()
                   << " ) in "     << __throwFileName
                   << ", Zeile "   << __throwLineNumber );
        }
#   endif

    catch( ... )
    {
#       if defined __BORLANDC__
            SHOW_ERR( "Fehler "    << __throwExceptionName
                   << " in "       << __throwFileName
                   << ", Zeile "   << __throwLineNumber );
#        else
            SHOW_ERR( "Unbekannte Exception" );
#       endif
    }
}

#endif
//----------------------------------------------------Sos_application_sv::_main

void Sos_application_sv::_main( int, char* [] )
{
    // Dummy (?)
}

//-------------------------------------------------Sos_application_sv::_execute

void Sos_application_sv::_execute()
{
    Execute();
}

//----------------------------------------------Sos_application_sv::_reschedule

Abs_sos_application::Reschedule_status Sos_application_sv::_reschedule()
{
    Reschedule_status r = reschedule_normal;

    if( InSendMessage() )  return r;

    while(1) {
#       if defined( SYSTEM_WIN )
        {
            MSG msg;
            if( !PeekMessage( &msg, NULL, 0, 0, 0 ) )  break;
            if( msg.message == WM_QUIT )  {
                r = reschedule_terminate;
                break;
            }
        }
#       endif

        Reschedule();
        r = GetAppWindow() && GetAppWindow()->IsVisible()? reschedule_normal
                                                         : reschedule_terminate;
        if( r == reschedule_terminate )  break;

#       if !defined( SYSTEM_WIN )
            break;
#       endif
    }
    return r;
}

//----------------------------------------------------Sos_application_sv::_busy

void Sos_application_sv::_busy( Bool b )
{
    if( b ) {
        if( _busy_depth == 0 )  Wait( TRUE );
        _busy_depth++;
    } else {
        _busy_depth--;
        if( _busy_depth == 0 )  Wait( FALSE );
    }
}


} //namespace sos

#endif                  // SYSTEM_STARVIEW
