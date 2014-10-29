// $Id$

#include "spooler.h"
#include "../javaproxy/java__lang__String.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__SpoolerC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__OrderC.h"


namespace sos {
namespace scheduler {

////---------------------------------------------------------------Scheduler_event2::Scheduler_event2
//
//Scheduler_event2::Scheduler_event2(Scheduler_object* object, const string& code) 
//: 
//    Scheduler_object( object->spooler(), this, Scheduler_object::type_scheduler_event2 ),
//    _zero_(this+1)
//{
//    Order* order = dynamic_cast<Order*>(object);
//    if (!order)  throw_xc(S() << Z_FUNCTION << ": " << object->obj_name());
//
//    _eventJ.assign( EventJ::new_instance(object->spooler()->j(), order->j(), code) );
//}
//
////--------------------------------------------------------------Scheduler_event2::~Scheduler_event2
//    
//Scheduler_event2::~Scheduler_event2() {}
//
////-----------------------------------------------------------------------Scheduler_event2::obj_name
//
//string Scheduler_event2::obj_name() const {
//    return _eventJ.toString();
//}



//Scheduler_event2::Class_descriptor Scheduler_event2::class_descriptor  ( &typelib, "sos.spooler.Scheduler_event", Scheduler_event2::_methods );
//
////-----------------------------------------------------------------------Scheduler_event2::_methods
//
//const Com_method Scheduler_event2::_methods[] =
//{
//#ifdef COM_METHOD
//    COM_PROPERTY_GET( Scheduler_event2, 1, Java_class_name, VT_BSTR     , 0 ),
//    COM_PROPERTY_GET( Scheduler_event2, 2, Object         , VT_DISPATCH, 0 ),
//    COM_PROPERTY_GET( Scheduler_event2, 3, Code           , VT_DISPATCH, 0 ),
//#endif
//    {}
//};
//
////---------------------------------------------------------------Scheduler_event2::Scheduler_event2
//    
//Scheduler_event2::Scheduler_event2( Scheduler_object* object, const string& code ) 
//: 
//    Scheduler_object( object->spooler(), (spooler_com::Iorder*)this, Scheduler_object::type_scheduler_event2 ),
//    Idispatch_implementation( &class_descriptor ),
//    _zero_(this+1),
//    _object(object), 
//    _code(code) 
//{}
//
////--------------------------------------------------------------Scheduler_event2::~Scheduler_event2
//    
//Scheduler_event2::~Scheduler_event2()
//{
//}
//
////-----------------------------------------------------------------Scheduler_event2::QueryInterface
//
//STDMETHODIMP Scheduler_event2::QueryInterface( const IID& iid, void** result )
//{
//    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, spooler_com::Ischeduler_event2   , result );
//    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, spooler_com::Ihas_java_class_name, result );
//    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, IDispatch                        , result );
//
//    return Idispatch_implementation::QueryInterface( iid, result );
//}
//
////-----------------------------------------------------------------------Scheduler_event2::obj_name
//    
//string Scheduler_event2::obj_name() const
//{
//    return "Scheduler_event2";
//}
//
////---------------------------------------------------------------------Scheduler_event2::get_Object
//
//STDMETHODIMP Scheduler_event2::get_Object( spooler_com::Iorder** result )
//{
//    HRESULT hr = S_OK;
//
//    try
//    {
//        *result = dynamic_cast<spooler_com::Iorder*>(_object);
//        if( *result )  (*result)->AddRef();
//    }
//    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }
//
//    return hr;
//}
//
////-----------------------------------------------------------------------Scheduler_event2::get_Code
//
//STDMETHODIMP Scheduler_event2::get_Code( BSTR* result )
//{
//    HRESULT hr = S_OK;
//
//    try
//    {
//        hr = String_to_bstr( _code, result );
//    }
//    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }
//
//    return hr;
//}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
