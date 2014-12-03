// $Id: reference.cxx 13199 2007-12-06 14:15:42Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "zschimmer.h"
#include "reference.h"
#include "log.h"

namespace zschimmer {

using namespace ::std;

//-----------------------------------------------------------------------------------assert_pointer

void* assert_pointer( void* p )
{
    if( !p )  throw_null_pointer_exception();
    return p;
}

//--------------------------------------------------------------------Is_referenced::~Is_referenced

//Is_referenced::~Is_referenced()
//{
//    release_all_references();
//}
//
////----------------------------------------------------------------Is_referenced::register_reference
//
//void Is_referenced::register_reference( Reference_base* source )
//{
//    Z_DEBUG_ONLY( assert( _reference_register.find( source ) == _reference_register.end() ) );
//    _reference_register.insert( source );
//}
//
////--------------------------------------------------------------Is_referenced::unregister_reference
//
//void Is_referenced::unregister_reference( Reference_base* source )
//{
//    Z_DEBUG_ONLY( assert( _reference_register.find( source ) != _reference_register.end() ) );
//    _reference_register.erase( source );
//}
//
////------------------------------------------------------------Is_referenced::release_all_references
//
//void Is_referenced::release_all_references()
//{
//    for( Set::iterator it = _reference_register.begin(); it != _reference_register.end(); )
//    {
//        Set::iterator next_it = it;
//        next_it++;
//
//        Reference_base* source = *it;
//        _reference_register.erase( it );
//        source->on_releasing_referenced_object();
//
//        it = next_it;
//    }
//}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
