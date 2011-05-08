#include "precomp.h"
//#define MODULE_NAME "sqlutil"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../file/anyfile.h"
#include "../kram/dynobj.h"
#include "../kram/stdfield.h"       // Text_type

using namespace std;
namespace sos {

//-------------------------------------------------------------------------------get_single_row

Dyn_obj get_single_row( const Sos_string& stmt, const Dyn_obj& param )
{
    Dyn_obj             o;
    Sos_ptr<Field_type> type;
    Record_type         param_record_type;    // muss close() gültig sein!
    Any_file            f;
    Dynamic_area        buffer;

    f.prepare( stmt, Any_file::Open_mode( Any_file::in | Any_file::seq ) );

    if( param.type() ) {
        if( !param.type()->obj_is_type( tc_Record_type ) ) {
            param_record_type.add_field( param.type(), "field", 0 );
            f.bind_parameters( &param_record_type, param.ptr() );
        } else {
            //Sos_ptr<Record_type> record_type = Record_type::create();
            //record_type->add_field( param.type(), "param1", 0 );
            //f.bind_parameters( SOS_CAST( Record_type, record_type ), param.ptr() );
            f.bind_parameters( SOS_CAST( Record_type, param.type() ), param.ptr() );
        }
    }

    f.open();


    try {
        f.get( &buffer );
    }
    catch( const Eof_error& )  { throw_not_found_error( "SOS-1251", &param ); }

    type = f.spec()._field_type_ptr;

    if( !type ) {
        // Text wird angenommen, oder besser Binary_type?
        // Binary_type, der als Text interpretiert wird, wenn nur abdruckbare Zeichen drin sind?
        Sos_ptr<Text_type> p = SOS_NEW( Text_type( buffer.length() ) );
        type = +p;
    }

    o.assign( type, buffer );

    f.close();

    return o;
}

} //namespace sos
