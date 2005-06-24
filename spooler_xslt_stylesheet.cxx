// $Id: spooler_subprocess.cxx 3668 2005-05-17 10:47:25Z jz $

#include "spooler.h"


namespace sos {
namespace spooler {


//-------------------------------------------------------------------------------------------------

Xslt_stylesheet::Class_descriptor    Xslt_stylesheet::class_descriptor ( &typelib, "Spooler.Xslt_stylesheet", Xslt_stylesheet::_methods );

//------------------------------------------------------------------------Xslt_stylesheet::_methods

const Com_method Xslt_stylesheet::_methods[] =
{ 
#ifdef COM_METHOD
    COM_PROPERTY_GET( Xslt_stylesheet,  1, Java_class_name       , VT_BSTR    , 0 ),
    COM_METHOD      ( Xslt_stylesheet,  2, Close                 , VT_EMPTY   , 0 ),
    COM_METHOD      ( Xslt_stylesheet,  3, Load_xml              , VT_EMPTY   , 0, VT_BSTR ),
    COM_METHOD      ( Xslt_stylesheet,  4, Load_file             , VT_EMPTY   , 0, VT_BSTR ),
    COM_METHOD      ( Xslt_stylesheet,  5, Transform_xml         , VT_BSTR    , 0, VT_BSTR  ),
  //COM_METHOD      ( Xslt_stylesheet,  6, Transform_xml_to_file , VT_EMPTY   , 0, VT_BSTR, VT_BSTR  ),
#endif
    {}
};

//-----------------------------------------------------------------Xslt_stylesheet::Xslt_stylesheet

Xslt_stylesheet::Xslt_stylesheet() 
: 
    Idispatch_implementation( &class_descriptor ),
    _zero_(this+1)
{
}

//-----------------------------------------------------------------Xslt_stylesheet::Xslt_stylesheet

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

//----------------------------------------------------------------Xslt_stylesheet::~Xslt_stylesheet

Xslt_stylesheet::~Xslt_stylesheet() 
{
}

//------------------------------------------------------------------Xslt_stylesheet::QueryInterface

STDMETHODIMP Xslt_stylesheet::QueryInterface( const IID& iid, void** result )
{
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, spooler_com::Ixslt_stylesheet    , result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, spooler_com::Ihas_java_class_name, result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, IDispatch                        , result );

    return Idispatch_implementation::QueryInterface( iid, result );
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
