// $Id$

#include "../zschimmer/zschimmer.h"
#include "../zschimmer/com.h"
#include "../zschimmer/z_com.h"
#include "../zschimmer/log.h"
#include "hostole_ptr.h"

using namespace zschimmer;
using namespace zschimmer::com;

namespace sos {
namespace hostole {

//------------------------------------------------------------------------------------throw_hostole

void throw_hostole( HRESULT hr, const char* function )
{
    if( hr == DISP_E_EXCEPTION )
    {
        ptr<IErrorInfo> error_info;

        HRESULT my_hr = GetErrorInfo( 0, error_info.pp() );
        if( !FAILED(my_hr) )
        {
            Bstr description_bstr;
            Bstr source_bstr;

            error_info->GetDescription( &description_bstr._bstr );
            error_info->GetSource     ( &source_bstr._bstr      );

            Xc x;
            x._name = string_from_bstr( source_bstr );

            string description = string_from_bstr( description_bstr );
            size_t pos = description.find( ' ' );
            if( pos != string::npos  &&  pos > 0 )
            {
                //x._code = description.substr( 0, pos );
                //while( pos < description.length()  &&  description[pos] == ' ' )  pos++;
                //x._what = description.substr( pos );
                x._what = description;
            }
            else
            {
                x._code = "SOS";
                x._what = description;
            }

            throw x;
        }
    }

    throw_com( hr, function );
}

//------------------------------------------------------------------------Hostware::set_com_context

void Hostware::set_com_context( const z::com::Com_context* c ) const
{ 
    HRESULT hr;

    if( try_qi_ptr<Imodule_interface> module_interface = _ptr )
    {
        hr = module_interface->putref_Com_context( c );
    }
    else
    if( try_qi_ptr<Imodule_interface2> module_interface = _ptr )
    {
        hr = module_interface->putref_Com_context( c );
    }
    else
    {
        hr = E_NOTIMPL;
    }

    if( FAILED(hr) )  throw_com( hr, "Hostware::set_com_context" );
}

//------------------------------------------------------------------------Hostware::set_com_context

void Hostware::set_com_context() const
{ 
    set_com_context( static_com_context_ptr? static_com_context_ptr : &com_context );
}

//----------------------------------------------------------------------------------Record::is_null

bool Record::is_null( const char* name ) const
{
    try
    {
        return field( name ).is_null();
    }
    catch( Xc& x ) 
    { 
        x.append_text( string("Feld ") + name );  
        throw x; 
    }
}

//----------------------------------------------------------------------------------Record::is_null

bool Record::is_null( int nr ) const
{
    try
    {
        return field( Variant( nr ) ).is_null();
    }
    catch( Xc& x ) 
    { 
        x.append_text( "Feld " + as_string( nr ) );  
        throw x; 
    }
}

//--------------------------------------------------------------------------------Record::as_string

string Record::as_string( const char* name ) const
{
    try
    {
        return field( Variant( name ) ).as_string();
    }
    catch( Xc& x ) 
    { 
        x.append_text( string("Feld ") + name );  
        throw x; 
    }
}

//--------------------------------------------------------------------------------Record::as_string

string Record::as_string( int nr ) const
{
    try
    {
        return field( Variant( nr ) ).as_string();
    }
    catch( Xc& x ) 
    { 
        x.append_text( "Feld " + as_string( nr ) );  
        throw x; 
    }
}

//--------------------------------------------------------------------------------Record::as_string
/*
string Record::as_string( const char* name, const char* deflt )
{
    try
    {   
        Variant result;
        get_field( name, &result );
        return result.as_string();
    }
    catch( Xc& x ) 
    { 
        x.append_text( string("Feld ") + name );  
        throw x; 
    }
}

//--------------------------------------------------------------------------------Record::as_string

string Record::as_string( int nr, const char* deflt )
{
    try
    {
        return field( Variant( nr ) ).as_string();
    }
    catch( Xc& x ) 
    { 
        x.append_text( "Feld " + as_string( nr ) );  
        throw x; 
    }
}
*/
//-----------------------------------------------------------------------------------Record::as_int

int Record::as_int( const char* name ) const
{
    try
    {
        return field( name ).as_int();
    }
    catch( Xc& x ) 
    { 
        x.append_text( string("Feld ") + name );  
        throw x; 
    }
}

//-----------------------------------------------------------------------------------Record::as_int

int Record::as_int( int nr ) const
{
    try
    {
        return field( nr ).as_int();
    }
    catch( Xc& x ) 
    { 
        x.append_text( "Feld " + as_string( nr ) );  
        throw x; 
    }
}

//-----------------------------------------------------------------------------------Record::as_int

int Record::as_int( const char* name, int value_for_null ) const
{
    try
    {
        Variant value;
        get_field( name, &value );
        return value.is_null_or_empty_string()? value_for_null : value.as_int();
    }
    catch( Xc& x ) 
    { 
        x.append_text( string("Feld ") + name );  
        throw x; 
    }
}

//-----------------------------------------------------------------------------------Record::as_int

int Record::as_int( int nr, int value_for_null ) const
{
    try
    {
        Variant value;
        get_field( nr, &value );
        return value.is_null_or_empty_string()? value_for_null : value.as_int();
    }
    catch( Xc& x ) 
    { 
        x.append_text( "Feld " + as_string( nr ) );  
        throw x; 
    }
}

//----------------------------------------------------------------------------------Record::as_bool

bool Record::as_bool( const char* name ) const
{
    try
    {
        return field( Variant( name ) ).as_int64() != 0;
    }
    catch( Xc& x ) 
    { 
        x.append_text( string("Feld ") + name );  
        throw x; 
    }
}

//----------------------------------------------------------------------------------Record::as_bool

bool Record::as_bool( int nr ) const
{
    try
    {
        return field( Variant( nr ) ).as_int64() != 0;
    }
    catch( Xc& x ) 
    { 
        x.append_text( "Feld " + as_string( nr ) );  
        throw x; 
    }
}

//--------------------------------------------------------------------------------Hostware::use_log

void Hostware::use_log() const
{

    if( is_version_or_later( "1.6.131" ) )
    {
        Log_context** log_context = NULL;

        HRESULT hr = _ptr->get_Log_context( (void***)&log_context );
        if( FAILED(hr) )  throw_hostole( hr, "get_Log_context" );

        Log_ptr::set_log_context( log_context );
    }
    else
    {
        /* veraltet
        ostream**     ostr = NULL;
        System_mutex* mutex = NULL;

        HRESULT hr = _ptr->Get_log_( (void***)&ostr, (void**)&mutex );
        if( FAILED(hr) )  throw_hostole( hr, "Get_log_" );
    
        Log_ptr::set_stream_and_system_mutex( ostr, mutex );

        if( is_version_or_later( "1.6.68" ) )
        {
            uint tls_index = 0;
            _ptr->get_Log_indent_tls_index( &tls_index );
            Log_ptr::set_indent_tls_index( tls_index );
        }
        */
    }
}

//----------------------------------------------------------------Hostware::file_as_string_or_empty

void Hostware::file_as_string_or_empty( BSTR filename, BSTR* result ) const
{
    try
    {
        file_as_string( filename, result );
    }
    catch( const Xc& x )
    {
        if( x.name() != "NOTEXIST" )  throw;
        *result = NULL;
    }
}

//----------------------------------------------------------------Hostware::file_as_string_or_empty

string Hostware::file_as_string_or_empty( const string& filename ) const 
{ 
    Bstr result; 
    file_as_string_or_empty( Bstr(filename), &result._bstr );  
    return string_from_bstr( result ); 
}

//----------------------------------------------------------------Factory_processor::set_parameters

void Factory_processor::set_parameters( Ivariables2* parameters ) const
{
    ptr<Ivariables> ivariables;
    HRESULT         hr; 

    if( parameters )  
    {
        hr = parameters->QueryInterface( IID_Ivariables, ivariables.void_pp() );
        if( FAILED(hr) )  throw_hostole( hr, __FUNCTION__ );
    }

    set_parameters( ivariables );
}

//-------------------------------------------------------------------------------------------------

} //namespace hostware
} //namespace sos
