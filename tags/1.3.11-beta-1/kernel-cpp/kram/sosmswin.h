/* sosmswin.h                                       (c) SOS GmbH Berlin
   MS-Windows-Spezialitäten
*/

#if defined SYSTEM_WIN && !defined __SOSMSWIN_H
#define __SOSMSWIN_H

//SOS_DECLARE_MSWIN_HANDLE( HINSTANCE );
#include <windows.h>

namespace sos {


struct Mswin
{
    struct Error : Xc
    {
                                Error                   ( const char* s )
                                                        : Xc( Msg_code( "WIN-", s )) {}
    };

    struct Callback_instance
    {
        /* MakeProcInstance() und FreeProcInstance().
           MakeProcInstance() wird erst bei der ersten Verwendung aufgerufen,
           um bei einer statischen Variablen nicht unnötig Windows-Ressourcen zu belegen
           oder Fehler beim Programmstart hervorzurufen.
        */
                                Callback_instance       ( FARPROC );
                               ~Callback_instance       ();

                                operator FARPROC        ();

      private:
        FARPROC                _instance;
        FARPROC                _procedure;
    };

    static HINSTANCE            hinstance               ();
};

ostream& operator << ( ostream&, HINSTANCE ); 

//==========================================================================================inlines

inline Mswin::Callback_instance::Callback_instance( FARPROC procedure )
 : _procedure ( procedure ),
   _instance  ( 0 )
{
}


} //namespace sos

#endif

