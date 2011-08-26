// $Id: register.cxx 13198 2007-12-06 14:13:38Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"


namespace sos {
namespace scheduler {
/*

//---------------------------------------------------------------------------Registered::Registered
    
Registered::Registered( Register* reg, IUnknown* iunknown, Scheduler_object::Type_code type_code, const string& name )
:
    Scheduler_object( reg->_spooler, iunknown, type_code ),
    _zero_(this+1),
    _register(reg)
{
    if( name != "" )  set_name( name );
}

//--------------------------------------------------------------------------Registered::~Registered

Registered::~Registered()
{
}

//-----------------------------------------------------------------------------Registered::set_name

void Registered::set_name( const string& name )
{
    _spooler->check_name( name );

    if( name != _name )
    {
        if( _name != "" )  z::throw_xc( "SCHEDULER-243", "Registered.name" );
      //if( is_added() )  z::throw_xc( "SCHEDULER-243", "Registered.name" );

        _name = name;
    }

    _log->set_prefix( obj_name() );
}

//------------------------------------------------------------------------Registered::is_registered

bool Registered::is_registered() const
{
    bool result = false;

    if( Registered* registered = _register->registered_or_null_( path() ) )
    {
        if( registered == this )  result = true;
    }

    return result;
}

//-----------------------------------------------------------------------------Registered::obj_name

string Registered::obj_name() const
{
    S result;

    result << "Registered " << path();

    return result;
}

//-----------------------------------------------------------------------------Registered::put_Name

STDMETHODIMP Registered::put_Name( BSTR name_bstr )
{
    HRESULT hr = S_OK;

    try
    {
        set_name( string_from_bstr( name_bstr ) );
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-------------------------------------------------------------------------------Registered::remove

void Registered::remove()
{
    _register->remove_registered_( this );
}

//-------------------------------------------------------------------------------Registered::Remove

STDMETHODIMP Registered::Remove()
{
    HRESULT hr = S_OK;

    try
    {
        remove();
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-------------------------------------------------------------------------------Register::Register

Register::Register( Scheduler* scheduler, const string& registered_type_name )
:
    _zero_(this+1),
    _registered_type_name(registered_type_name),
    _spooler(scheduler)
{
}


//----------------------------------------------------------------------------Register::registered_

Registered* Register::registered_( const string& name )
{
    Registered* result = registered_or_null_( name );
    if( !result )  z::throw_xc( "SCHEDULER-401", _registered_type_name + " " + name );
    return result;
}

//-------------------------------------------------------------------------Register::get_Registered

STDMETHODIMP Register::get_Registered_( BSTR path_bstr, Registered** result )
{
    HRESULT hr = S_OK;

    try
    {
        *result = registered_( string_from_bstr( path_bstr ) );
        if( result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//----------------------------------------------------------------Register::get_Registered_or_null_

STDMETHODIMP Register::get_Registered_or_null_( BSTR path_bstr, Registered** result )
{
    HRESULT hr = S_OK;

    try
    {
        *result = registered_or_null_( string_from_bstr( path_bstr ) );
        if( result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//---------------------------------------------------------------------Register::Create_registered_

STDMETHODIMP Register::Create_registered_( Registered** result )
{
    HRESULT hr = S_OK;

    try
    {
        *result = new_registered_().take();
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//------------------------------------------------------------------------Register::Add_registered_

STDMETHODIMP Register::Add_registered_( Registered* registered )
{
    HRESULT hr = S_OK;

    try
    {
        add_registered_( registered );
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-------------------------------------------------------------------------------------------------
*/

} //namespace scheduler
} //namespace sos
