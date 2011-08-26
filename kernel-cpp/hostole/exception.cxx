// hostole.cxx                                  (C)1996-98 SOS GmbH Berlin
// $Id: exception.cxx 11394 2005-04-03 08:30:29Z jz $

#include "precomp.h"

#include "../kram/sos.h"
#include "../kram/thread.h"

#include "hostole.h"
#include "hostole2.h"

#if 0

namespace sos {

DESCRIBE_CLASS( &hostole_typelib, Hostware_exception, exception, CLSID_Exception, "hostWare.Exception", "1.0" );

//-------------------------------------------------------------------------------Hostware_exception

struct Hostware_exception : Iexception, Sos_ole_object
{                           
    void*                       operator new            ( size_t size )                             { return sos_alloc( size, "Hostware_exception" ); }
    void                        operator delete         ( void* ptr )                               { sos_free( ptr ); }

                              //Hostware_exception      ();
                                Hostware_exception      ( const Xc& );
                               ~Hostware_exception      ()                                          { delete _xc; }

    USE_SOS_OLE_OBJECT

    STDMETHODIMP                get_name                ( BSTR* result );
    STDMETHODIMP                get_code                ( BSTR* result );
    STDMETHODIMP                get_text                ( BSTR* result );


    Xc*                        _xc;
};

//-------------------------------------------------------------------Hostware_exception_thread_data

struct Hostware_exception_thread_data : Thread_container::Data
{                               
                                Hostware_exception_thread_data()                                    : _last_xc(NULL) {}
                               ~Hostware_exception_thread_data()                                    { delete _last_xc; }

    ptr<Iexception>            _last_exception;
    Xc*                        _last_xc;
};

//--------------------------------------------------------------------------------------thread_data

static Hostware_exception_thread_data* thread_data()
{
    Thread_container* tc = thread_container();

    if( !tc->_hostole_exception )  tc->_hostole_exception = SOS_NEW( Hostware_exception_thread_data );

    return +tc->_hostole_exception;
}

//----------------------------------------------------------------------------------make_iexception
/*
Iexception make_iexception( const Xc& x )
{
    return Hostware_exception( x );
}
*/
//-----------------------------------------------------------Hostware_exception::Hostware_exception
/*
Hostware_exception::Hostware_exception()
: 
    Sos_ole_object( exception_class_ptr, this )  
{ 
    _xc = NULL; 
}
*/
//-----------------------------------------------------------Hostware_exception::Hostware_exception

Hostware_exception::Hostware_exception( const Xc& x )                             
: 
    Sos_ole_object( exception_class_ptr, this )  
{ 
    _xc = new Xc(x); 
}

//---------------------------------------------------------------------Hostware_exception::get_name

STDMETHODIMP Hostware_exception::get_name( BSTR* result )
{
    *result = SysAllocString_string( _xc->name() );
    return NOERROR;
}

//---------------------------------------------------------------------Hostware_exception::get_code

STDMETHODIMP Hostware_exception::get_code( BSTR* result )
{
    *result = SysAllocString_string( _xc->code() );
    return NOERROR;
}

//---------------------------------------------------------------------Hostware_exception::get_text

STDMETHODIMP Hostware_exception::get_text( BSTR* result )
{
    *result = SysAllocString_string( _xc->what() );
    return NOERROR;
}

//--------------------------------------------------------------------------Hostware::get_Exception

HRESULT Hostware::get_Exception( Iexception** exception )
{
    if( !thread_data()->_last_exception ) 
    {
        thread_data()->_last_exception = new Hostware_exception( *thread_data()->_last_xc );
    }

    *exception = thread_data()->_last_exception;
    (*exception)->AddRef();

    return NOERROR;
}

//--------------------------------------------------------------------------Hostware::put_Exception

HRESULT Hostware::put_Exception( Iexception* exception )
{
    thread_data()->_last_exception = exception;

    return NOERROR;
}

//----------------------------------------------------------------------------set_hostole_exception

void set_hostole_exception( const Xc& x )
{
    delete thread_data()->_last_xc;
    thread_data()->_last_xc = new Xc( x );
}

//-------------------------------------------------------------------------------------------------

} //namespace sos

#endif
