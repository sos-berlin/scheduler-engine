#include "precomp.h"
//#define MODULE_NAME "sosstat"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "../kram/sos.h"
#include "../kram/sosstat.h"
#include "../kram/sosclien.h"
#include "../file/absfile.h"
#include "../file/sosdb.h"
#include "../kram/log.h"


using namespace std;
namespace sos {

DEFINE_SOS_STATIC_PTR( Sos_client )

//-----------------------------------------------------------------------------------Std_client

struct Std_client : Sos_client
{
};

//------------------------------------------------------------------------------init_std_client

void init_std_client()
{
    Sos_ptr<Std_client> client = SOS_NEW_PTR( Std_client );
    client->_name = "standard-client";
    sos_static_ptr()->_std_client = +client;
}

//-----------------------------------------------------------------------Sos_client::Sos_client

Sos_client::Sos_client()
{
    _session_array.obj_const_name( "Sos_client::_session_array" );
    _session_array.first_index( 1 );
}

//----------------------------------------------------------------------Sos_client::~Sos_client

Sos_client::~Sos_client()
{
    close();
}

//----------------------------------------------------------------------------Sos_client::close

void Sos_client::close()
{
    for( int i = _session_array.last_index(); i >= _session_array.first_index(); i-- ) {
        Sos_ptr<Sos_database_session> s = _session_array[ i ];
        if( s ) {
            _session_array[ i ] = NULL;
            try {
                LOGI( "Sos_client::close()  close db=" << s->_db_name
                      << ", user=" << s->_user << '\n' );
                s->close( close_error );
            }
            catch(...) {}
        }
    }
/*
    for( int i = _static_array.last_index(); i >= _static_array.first_index(); i-- )
    {
        Sos_database_static* s = _static_array[ i ];
        if( s ) {
            _static_array[ i ] = 0;
            try {
                s->close_all_sessions();
            }
            catch(...) {}
        }
    }
*/
}

//-----------------------------------------------------------------------Sos_client::_obj_print

void Sos_client::_obj_print( ostream* s ) const
{
    *s << _name;

    for( int i = _session_array.first_index(); i <= _session_array.last_index(); i++ )
    {
        Sos_database_session* session = _session_array[ i ];
        if( session ) {
            *s << ", DB session " << i << ": ";
            session->obj_print( s );
            *s << "; ";
        }
    }
}

} //namespace sos
