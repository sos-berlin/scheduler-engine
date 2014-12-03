#include "precomp.h"
#undef  MODULE_NAME
#define MODULE_NAME "sosarray"
#undef  COPYRIGHT
#define COPYRIGHT   "(c)1995 SOS GmbH Berlin"

#include "../kram/sysdep.h"

//#pragma implementation

#include "../kram/sos.h"
#include "../kram/sosarray.h"

#if !defined SYSTEM_INCLUDE_TEMPLATES
#   include "../kram/sosarray.tpl"
#endif

using namespace std;
namespace sos {

Sos_array_base::~Sos_array_base()
{
}


int Sos_array_base::add_empty()
{
    int4 new_last = last_index() + 1;
    last_index( new_last );
    return new_last;
}


void Sos_array_base::_check_index( int4 index ) const
{
    if( index >= first_index()  &&  index <= last_index() )  return;

    Xc x ( "SOS-1146" );
    x.insert( index );
    x.insert( first_index() );
    x.insert( last_index() );
    x.insert( _obj_const_name );
    
    throw x;
}

} //namespace sos
