// $Id: spooler_module_remote_server.cxx,v 1.1 2003/05/23 06:40:28 jz Exp $
/*
    Hier sind implementiert

    Remote_module_instance_server
*/



#include "spooler.h"

#if 0
#include "spooler_module_remote_server.h"

namespace sos {
namespace spooler {

using namespace zschimmer::com::object_server;

//-------------------------------------------------------------------------------------------------


//------------------------------------------------Remote_module_instance_server::_spooler_construct

STDMETHODIMP Remote_module_instance_server::_spooler_construct( SAFEARRAY* safearray )
{
    HRESULT hr = NOERROR;

    try
    {
        Locked_safearray params ( safearray );

        for( int i = 0; i < params.count(); i++ )
        {
            if( params[i].vt != VT_BSTR )  throw_xc( "_spooler_construct" );

            const OLECHAR* value = wcschr( V_BSTR( &params[i] ), '=' );
            if( !value )  throw_xc( "_spooler_construct" );
            value++;

            if( olestring_begins_with( V_BSTR( &params[i] ), "language="   )  _language       = string_from_ole( value );
            else                                                                         
            if( olestring_begins_with( V_BSTR( &params[i] ), "com_class="  )  _com_class_name = string_from_ole( value );
            else                                                                         
            if( olestring_begins_with( V_BSTR( &params[i] ), "filename="   )  _filename       = string_from_ole( value );
            else
            if( olestring_begins_with( V_BSTR( &params[i] ), "java_class=" )  _java_class     = string_from_ole( value );
            else
            if( olestring_begins_with( V_BSTR( &params[i] ), "recompile="  )  _recompile      = value[0] == '1';
            else
            if( olestring_begins_with( V_BSTR( &params[i] ), "script="     )  _script         = string_from_ole( value );
            else
                throw_xc( "_spooler_construct" );
        }
    }
    catch( const exception& x ) { hr = com_set_error( x, "Remote_module_instance_server::_spooler_construct" );

    return hr;
}

//--------------------------------------------------Remote_module_instance_server::_spooler_add_obj

STDMETHODIMP Remote_module_instance_server::_spooler_add_obj( IDispatch*, BSTR name )
{
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
#endif
