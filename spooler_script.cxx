// $Id: spooler_script.cxx,v 1.1 2001/01/25 20:28:48 jz Exp $
/*
    Hier sind implementiert

    Script
    Script_instance
*/


#include "../kram/sos.h"
#include "spooler.h"

using namespace std;

namespace sos {
namespace spooler {

//----------------------------------------------------------------------------Script_instance::init

void Script_instance::init( const string& language )
{
    _script_site = new Script_site;
    _script_site->_engine_name = language;
    _script_site->init_engine();
}

//----------------------------------------------------------------------------Script_instance::load

void Script_instance::add_obj( const CComPtr<IDispatch>& object, const string& name )
{
    CComBSTR name_bstr;
    name_bstr.Attach( SysAllocString_string( name ) );

    _script_site->add_obj( object, name_bstr );
}

//----------------------------------------------------------------------------Script_instance::load

void Script_instance::load( const Script& script )
{
    if( !_script_site )  init( script._language );
                   else  if( _script_site->_engine_name != script._language )  throw_xc( "SPOOLER-117" );

    _script_site->parse( script._text );

    _loaded = true;
}

//---------------------------------------------------------------------------Script_instance::close

void Script_instance::close()
{
    if( _script_site )
    {
        _script_site->close_engine();
        _script_site = NULL;
    }

    _loaded = false;
}

//------------------------------------------------------------------Script_instance::call_if_exists

CComVariant Script_instance::call_if_exists( const char* name )
{
    if( name_exists( name ) )  return call( name );
                         else  return CComVariant();
}

//----------------------------------------------------------------------------Script_instance::call

CComVariant Script_instance::call( const char* name )
{
    return _script_site->call( name );
}

//----------------------------------------------------------------------------Script_instance::call

CComVariant Script_instance::call( const char* name, int param )
{
    return _script_site->call( name, param );
}

//--------------------------------------------------------------------Script_instance::property_get

CComVariant Script_instance::property_get( const char* name )
{
    return _script_site->property_get( name );
}

//-----------------------------------------------------------Script_instance::optional_property_put

void Script_instance::optional_property_put( const char* name, const CComVariant& v )
{
    try 
    {
        property_put( name, v );
    }
    catch( const Xc& )
    {
        // Ignorieren, wenn das Objekt die Eigenschaft nicht kennt
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
