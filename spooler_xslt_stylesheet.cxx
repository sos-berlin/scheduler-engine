// $Id$

#include "spooler.h"


namespace sos {
namespace scheduler {


//-------------------------------------------------------------------------------------------------

Xslt_stylesheet::Class_descriptor    Xslt_stylesheet::class_descriptor ( &typelib, "Spooler.Xslt_stylesheet", Xslt_stylesheet::_methods );

//------------------------------------------------------------------------Xslt_stylesheet::_methods

const Com_method Xslt_stylesheet::_methods[] =
{ 
#ifdef COM_METHOD
    COM_PROPERTY_GET( Xslt_stylesheet,  1, Java_class_name       , VT_BSTR    , 0 ),
    COM_METHOD      ( Xslt_stylesheet,  2, Close                 , VT_EMPTY   , 0 ),
    COM_METHOD      ( Xslt_stylesheet,  3, Load_xml              , VT_DISPATCH, 0, VT_BSTR ),
    COM_METHOD      ( Xslt_stylesheet,  4, Load_file             , VT_DISPATCH, 0, VT_BSTR ),
    COM_METHOD      ( Xslt_stylesheet,  5, Apply_xml             , VT_BSTR    , 0, VT_BSTR  ),
#endif
    {}
};

//-----------------------------------------------------------------Xslt_stylesheet::Create_instance
    
HRESULT Xslt_stylesheet::Create_instance( zschimmer::com::object_server::Session*, ptr<Object>*, const IID& iid, ptr<IUnknown>* result )
{
    if( iid == spooler_com::IID_Ixslt_stylesheet )
    {
        ptr<Xslt_stylesheet> instance = Z_NEW( Xslt_stylesheet );
        *result = static_cast<Ixslt_stylesheet*>( +instance );
        return S_OK;
    }

    return E_NOINTERFACE;
}

//-----------------------------------------------------------------Xslt_stylesheet::Xslt_stylesheet

Xslt_stylesheet::Xslt_stylesheet() 
: 
    Idispatch_implementation( &class_descriptor ),
    _zero_(this+1)
{
}

//-----------------------------------------------------------------Xslt_stylesheet::Create_instance
/*
HRESULT Xslt_stylesheet::Create_instance( zschimmer::com::object_server::Server*, ptr<Object>*, const IID& iid, ptr<IUnknown>* result )
{
    if( iid == spooler_com::IID_Ixslt_stylesheetproxy )
    {
        ptr<Xslt_stlyesheet> instance = Z_NEW( Xslt_stylesheet );
        *result = static_cast<IDispatch*>( static_cast<Proxy*>( +instance ) );
        return S_OK;
    }

    return E_NOINTERFACE;
}
*/
//-----------------------------------------------------------------Xslt_stylesheet::Xslt_stylesheet
/*
Xslt_stylesheet::Xslt_stylesheet( const string& xml_or_filename ) 
: 
    Idispatch_implementation( &class_descriptor ),
    xml::Xslt_stylesheet( xml_or_filename ),
    _zero_(this+1)
{
}

//-----------------------------------------------------------------Xslt_stylesheet::Xslt_stylesheet

Xslt_stylesheet::Xslt_stylesheet( const BSTR xml_or_filename_bstr ) 
: 
    Idispatch_implementation( &class_descriptor ),
    xml::Xslt_stylesheet( xml_or_filename_bstr ),
    _zero_(this+1)
{
}
*/
//----------------------------------------------------------------Xslt_stylesheet::~Xslt_stylesheet

Xslt_stylesheet::~Xslt_stylesheet() 
{
    Z_LOG2( "joacim", __FUNCTION__ << "\n" );
}

//------------------------------------------------------------------Xslt_stylesheet::QueryInterface

STDMETHODIMP Xslt_stylesheet::QueryInterface( const IID& iid, void** result )
{
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, spooler_com::Ixslt_stylesheet    , result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, spooler_com::Ihas_java_class_name, result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, IDispatch                        , result );

    return Idispatch_implementation::QueryInterface( iid, result );
}

//---------------------------------------------------------------------------Xslt_stylesheet::Close

STDMETHODIMP Xslt_stylesheet::Close()
{
    HRESULT hr = S_OK;

    try
    {
        release();
    }
    catch( exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------------------------Xslt_stylesheet::Load_file

STDMETHODIMP Xslt_stylesheet::Load_xml( BSTR xml_bstr, Ixslt_stylesheet** result )
{
    HRESULT hr = S_OK;

    Z_LOG2( "joacim", __FUNCTION__ << "\n" );

    try
    {
        load( xml::Document_ptr( xml_bstr ) );
    }
    catch( exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    *result = this;
    (*result)->AddRef();

    return hr;
}

//-----------------------------------------------------------------------Xslt_stylesheet::Load_file

STDMETHODIMP Xslt_stylesheet::Load_file( BSTR filename_bstr, Ixslt_stylesheet** result )
{
    HRESULT hr = S_OK;

    Z_LOG2( "joacim", __FUNCTION__ << "\n" );

    try
    {
        load_file( string_from_bstr( filename_bstr ) );
    }
    catch( exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    *result = this;
    (*result)->AddRef();

    return hr;
}

//-----------------------------------------------------------------------Xslt_stylesheet::Apply_xml

STDMETHODIMP Xslt_stylesheet::Apply_xml( BSTR xml_or_file_bstr, BSTR* result )
{
    HRESULT hr = S_OK;

    Z_LOG2( "joacim", __FUNCTION__ << "\n" );

    try
    {
        hr = String_to_bstr( apply( xml::Document_ptr( xml_or_file_bstr ) ).xml(), result );
    }
    catch( exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------------Xslt_stylesheet::Transform_xml_to_file
/*
STDMETHODIMP Xslt_stylesheet::Transform_xml_to_file( BSTR xml_or_file_bstr, BSTR filename_bstr )
{
    HRESULT hr = S_OK;

    try
    {
        hr = String_to_bstr( apply( Document_ptr( xml_or_file_bstr ) ).xml(), result );
        load( string_from_bstr( xml_or_file ) );
        write_result_to_file( result_document, string_from_bstr( filename ) );
    }
    catch( exception& x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}
*/
//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
