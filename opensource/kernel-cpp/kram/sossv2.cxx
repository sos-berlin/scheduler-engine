#include "precomp.h"
//#define MODULE_NAME "sossv2"
/* sossv2.cpp                                            (c) SOS GmbH Berlin
                                                            Joacim Zschimmer

Hilfsfunktionen, implementiert mit StarView. Ohne Sos_application.
Versuch, alle Aufrufe von StarView hier und in sossv.cpp zu konzentrieren, so daß alle
anderen Module frei von StarView sind.

*/
#include "../kram/sosstrng.h"
#include "../kram/sysdep.h"

#undef cerr   // sysdep.h soll cerr nur für win16 sperren

#include "../kram/sos.h"
#include "../kram/log.h"

using namespace std;
namespace sos {

Bool sos_gui = true;   // Wird von sosmain.cxx = false gesetzt

extern Sos_string   module_filename();

//--------------------------------------------------------------------show_msg()

typedef void (* Msg_func)( const char* text );

static Msg_func _msg_function = NULL;

void set_show_msg_function( Msg_func f )
{
    _msg_function = f;
}

extern void show_msg( const char* text_ptr )
{
    LOGI( "SHOW_MSG( \"" << text_ptr << "\" )\n" );

    if ( _msg_function != NULL ) {
        (*_msg_function)( text_ptr );
        return;
    }

#   if defined SYSTEM_WIN
        if( sos_gui ) {
            if( InSendMessage() ) {
                LOG( "in SendMessage()!\n" );
            } else {
                Sos_string m;
                Sos_string text;

                try {
                    Sos_string mod = module_filename();
                    //LOG( "mod=" << mod << "\n" );
                    const char* p = c_str( mod ) + length( mod );
                    const char* q = p;
                    while( p > c_str( mod )  &&  p[-1] != '/'  &&  p[-1] != '\\'  &&  p[-1] != ':' )  p--;
                    while( q > p  &&  q[-1] != '.' )  q--;
                    if( q > p )  q--;  // '.'
                    m = as_string( p, q - p );
                    //LOG( "m=" << m << "\n" );
                    if( q == p )  m = "SOS";
                    //LOG( "m=" << m << "\n" );
                }
                catch( const Xc& )
                {
                    m = "SOS";
                }

                // MessageBox bricht nur an Blanks um. Also Blanks einfügen:

                {
                    const char* p = text_ptr;
                    int         n = 0;

                    while( *p ) {
                        //LOG( "*p=" << *p << ", n=" << n << "\n" );
                        text += *p++;
                        if( *p == ' ' || *p == '\n' )  n = 0;
                                                 else  n++;
                        if( n >= 50 ) {
                            text += ' ';
                            n = 0;
                        }
                    }
                }

                //LOG( "text=" << text << ", m=" << m << "\n" );

                MessageBox( 0/*hwndParent*/, c_str( text ), c_str( m )/*caption*/, MB_TASKMODAL /*| MB_ICONINFORMATION*/ );
            }
        }
        else
        {
            cerr << text_ptr << '\n';
        }
#   else
        cerr << text_ptr << '\n';
#   endif
}


} //namespace sos
