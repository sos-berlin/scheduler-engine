// $Id: spooler_com.cxx,v 1.36 2002/04/06 16:31:51 jz Exp $
/*
    Hier sind implementiert

    Com_log                     COM-Hülle für Task_log
    Com_object_set              COM-Hülle für Object_set
    Com_task                    COM-Hülle für Task
    Com_spooler                 COM-Hülle für Spooler
*/


#include "../kram/sos.h"
#include "spooler.h"


#ifdef SYSTEM_WIN

namespace sos {
namespace spooler {

using namespace std;
using namespace spooler_com;

//------------------------------------------------------------------------------------Typbibliothek

Typelib_descr spooler_typelib ( spooler_com::LIBID_spooler_com, "Spooler", "1.0" );

DESCRIBE_CLASS( &spooler_typelib, Com_error       , error       , spooler_com::CLSID_error       , "Spooler.Error"       , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_variable_set, variable_set, spooler_com::CLSID_Variable_set, "Spooler.Variable_set", "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_log         , log         , spooler_com::CLSID_log         , "Spooler.Log"         , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_job         , job         , spooler_com::CLSID_job         , "Spooler.Job"         , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_task        , task        , spooler_com::CLSID_Task        , "Spooler.Task"        , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_object_set  , object_set  , spooler_com::CLSID_object_set  , "Spooler.Object_set"  , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_thread      , thread      , spooler_com::CLSID_thread      , "Spooler.Thread"      , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_spooler     , spooler     , spooler_com::CLSID_spooler     , "Spooler.Spooler"     , "1.0" )

//--------------------------------------------------------------------------------time_from_variant

Time time_from_variant( const VARIANT& vt )
{
    string  str = variant_as_string( vt );

    if( str.find( ':' ) != string::npos )
    {
        Sos_optional_date_time dt;
        dt.set_time( str );
        return dt.time_as_double();
    }
    else
        return as_double( str );
}

//-----------------------------------------------------------------------------Com_error::Com_error

Com_error::Com_error( const Xc_copy& x )
: 
    Sos_ole_object( error_class_ptr, this ),
    _xc(x) 
{
}

//---------------------------------------------------------------------------------Com_error::close

void Com_error::close()
{ 
    THREAD_LOCK( _lock )
    {
        _xc = NULL; 
    }
}

//------------------------------------------------------------------------------Com_error::is_error

STDMETHODIMP Com_error::get_is_error( VARIANT_BOOL* result )
{
    THREAD_LOCK( _lock )
    {
        *result = _xc != NULL;
    }

    return NOERROR;
}

//----------------------------------------------------------------------------------Com_error::code

STDMETHODIMP Com_error::get_code( BSTR* code_bstr )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_xc )  *code_bstr = NULL;
              else  *code_bstr = SysAllocStringLen_char( _xc->code(), strlen( _xc->code() ) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Error::code" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Error::code" ); }

    return hr;
}

//----------------------------------------------------------------------------------Com_error::text

STDMETHODIMP Com_error::get_text( BSTR* text_bstr )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_xc )  *text_bstr = NULL;
              else  *text_bstr = SysAllocString_string( _xc->what() );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Error::text" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Error::text" ); }

    return hr;
}

//---------------------------------------------------------------Com_variable_set::Com_variable_set

Com_variable_set::Com_variable_set()
:
    Sos_ole_object( variable_set_class_ptr, this )
{
}

//------------------------------------------------------------------------Com_variable_set::set_xml

void Com_variable_set::set_xml( const xml::Element_ptr& params )
{
    HRESULT hr;

    THREAD_LOCK( _lock )
    {
        for( xml::Element_ptr e = params->firstChild; e; e = e->nextSibling )
        {
            if( e->tagName == "param" ) 
            {
                CComVariant name  = e->getAttribute( "name" );
                hr = name.ChangeType( VT_BSTR );                    if( FAILED(hr) )  throw_ole( hr, "ChangeType" );

                CComVariant value = e->getAttribute( "value" );

                hr = put_var( name.bstrVal, &value );               if( FAILED(hr) )  throw_ole( hr, "Ivariable_set::put_var" );
            }
        }
    }
}

//------------------------------------------------------------------------Com_variable_set::put_var

STDMETHODIMP Com_variable_set::put_var( BSTR name, VARIANT* value )
{
    THREAD_LOCK( _lock )  _map[name] = *value;
    return NOERROR;
}

//------------------------------------------------------------------------Com_variable_set::get_var

STDMETHODIMP Com_variable_set::get_var( BSTR name, VARIANT* value )
{
    THREAD_LOCK( _lock )  return VariantCopy( value, &_map[name] );
    return NOERROR;
}

//----------------------------------------------------------------------Com_variable_set::get_count

STDMETHODIMP Com_variable_set::get_count( int* result )
{
    THREAD_LOCK( _lock )  *result = _map.size();
    return NOERROR;
}

//------------------------------------------------------------------------Com_variable_set::get_dom

STDMETHODIMP Com_variable_set::get_dom( xml::IXMLDOMDocument** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        *result = NULL;

        xml::Document_ptr doc = xml::Document_ptr( __uuidof(xml::DOMDocument30), NULL );
        doc->appendChild( doc->createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"iso-8859-1\"" ) );
    
        xml::Element_ptr varset = doc->createElement( "variable_set" );
        doc->appendChild( varset );

        for( Map::iterator it = _map.begin(); it != _map.end(); it++ )
        {
            if( it->second.vt != VT_EMPTY )
            {
                xml::Element_ptr var = doc->createElement( "variable" );
                var->setAttribute( "name" , (BSTR)it->first  );
                var->setAttribute( "value", it->second );
                varset->appendChild( var );
            }
        }

        *result = doc;
        (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::dom" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Variable_set::dom" ); }

    return hr;
}

//---------------------------------------------------------------------------------Com_log::Com_log

Com_log::Com_log( Prefix_log* log )
:
    Sos_ole_object( log_class_ptr, this ),
    _zero_(this+1),
    _log(log)
{ 
}

//----------------------------------------------------------------------------------------Com_log::
    
STDMETHODIMP Com_log::debug9( BSTR line )                       { return log( log_debug9, line ); }
STDMETHODIMP Com_log::debug8( BSTR line )                       { return log( log_debug8, line ); }
STDMETHODIMP Com_log::debug7( BSTR line )                       { return log( log_debug7, line ); }
STDMETHODIMP Com_log::debug6( BSTR line )                       { return log( log_debug6, line ); }
STDMETHODIMP Com_log::debug5( BSTR line )                       { return log( log_debug5, line ); }
STDMETHODIMP Com_log::debug4( BSTR line )                       { return log( log_debug4, line ); }
STDMETHODIMP Com_log::debug3( BSTR line )                       { return log( log_debug3, line ); }
STDMETHODIMP Com_log::debug2( BSTR line )                       { return log( log_debug2, line ); }
STDMETHODIMP Com_log::debug1( BSTR line )                       { return log( log_debug1, line ); }
STDMETHODIMP Com_log::debug ( BSTR line )                       { return log( log_debug1, line ); }
STDMETHODIMP Com_log::msg   ( BSTR line )                       { return log( log_info  , line ); }
STDMETHODIMP Com_log::info  ( BSTR line )                       { return log( log_info  , line ); }
STDMETHODIMP Com_log::warn  ( BSTR line )                       { return log( log_warn  , line ); }
STDMETHODIMP Com_log::error ( BSTR line )                       { return log( log_error , line ); }

//-------------------------------------------------------------------------------------Com_log::log

STDMETHODIMP Com_log::log( Log_level level, BSTR line )
{ 
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->log( level, bstr_as_string( line ) ); 
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::log" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::log" ); }

    return hr;
}

//-------------------------------------------------------------------------------------Com_log::log

STDMETHODIMP Com_log::get_mail( spooler_com::Imail** mail )
{ 
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *mail = _log->mail();
        if( *mail )  (*mail)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::mail" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::mail" ); }

    return hr;
}

//-----------------------------------------------------------------------Com_log::put_mail_on_error

STDMETHODIMP Com_log::put_mail_on_error( VARIANT_BOOL b )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->set_mail_on_error( b != 0 );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_error" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_error" ); }

    return hr;
}

//-----------------------------------------------------------------------Com_log::get_mail_on_error

STDMETHODIMP Com_log::get_mail_on_error( VARIANT_BOOL* b )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *b = _log->mail_on_error();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_error" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_error" ); }

    return hr;
}

    
//----------------------------------------------------------------------Com_log::put_mail_on_success

STDMETHODIMP Com_log::put_mail_on_success( VARIANT_BOOL b )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->set_mail_on_success( b != 0 );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_success" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_success" ); }

    return hr;
}

//---------------------------------------------------------------------Com_log::get_mail_on_success

STDMETHODIMP Com_log::get_mail_on_success( VARIANT_BOOL* b )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *b = _log->mail_on_success();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_success" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_success" ); }

    return hr;
}

//----------------------------------------------------------------------Com_log::put_mail_on_process

STDMETHODIMP Com_log::put_mail_on_process( VARIANT_BOOL b )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->set_mail_on_process( b != 0 );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_process" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_process" ); }

    return hr;
}

//---------------------------------------------------------------------Com_log::get_mail_on_process

STDMETHODIMP Com_log::get_mail_on_process( VARIANT_BOOL* b )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *b = _log->mail_on_process();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_process" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::mail_on_process" ); }

    return hr;
}

//-----------------------------------------------------------------------------------Com_log::level

STDMETHODIMP Com_log::put_level( int level )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->set_log_level( level );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::log_level" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::log_level" ); }

    return hr;
}

//-----------------------------------------------------------------------------------Com_log::level

STDMETHODIMP Com_log::get_level( int* level )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *level = _log->log_level();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::log_level" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::log_level" ); }

    return hr;
}

//--------------------------------------------------------------------------------Com_log::filename

STDMETHODIMP Com_log::get_filename( BSTR* filename_bstr )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *filename_bstr = SysAllocString_string( _log->filename() );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::filename" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::filename" ); }

    return hr;
}

//----------------------------------------------------------------------------Com_log::new_filename

STDMETHODIMP Com_log::put_new_filename( BSTR filename_bstr )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;
        _log->set_new_filename( bstr_as_string( filename_bstr ) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::new_filename" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::new_filename" ); }

    return hr;
}

//----------------------------------------------------------------------------Com_log::new_filename

STDMETHODIMP Com_log::get_new_filename( BSTR* filename_bstr )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *filename_bstr = SysAllocString_string( _log->new_filename() );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::new_filename" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::new_filename" ); }

    return hr;
}

//--------------------------------------------------------------------------Com_log::collect_within

STDMETHODIMP Com_log::put_collect_within( VARIANT* time )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->set_collect_within( time_from_variant(*time) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::collect_within" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::collect_within" ); }

    return hr;
}

//--------------------------------------------------------------------------Com_log::collect_within

STDMETHODIMP Com_log::get_collect_within( double* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *result = _log->collect_within();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::collect_within" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::collect_within" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_log::collect_max

STDMETHODIMP Com_log::put_collect_max( VARIANT* time )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        _log->set_collect_max( time_from_variant(*time) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::collect_max" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::collect_max" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_log::collect_max

STDMETHODIMP Com_log::get_collect_max( double* result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try 
    {
        if( !_log )  return E_POINTER;

        *result = _log->collect_max();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Log::collect_max" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Log::collect_max" ); }

    return hr;
}

//-------------------------------------------------------------------Com_object_set::Com_object_set

Com_object_set::Com_object_set( Object_set* object_set )
:
    Sos_ole_object( object_set_class_ptr, this ),
    _object_set(object_set)
{
}

//--------------------------------------------------------------------Com_object_set::get_low_level

STDMETHODIMP Com_object_set::get_low_level( int* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_object_set )  return E_POINTER;
        if( GetCurrentThreadId() != _object_set->thread()->_thread_id )  return E_ACCESSDENIED;

        *result = _object_set->_object_set_descr->_level_interval._low_level;
    }

    return NOERROR;
}

//-------------------------------------------------------------------Com_object_set::get_high_level

STDMETHODIMP Com_object_set::get_high_level( int* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_object_set )  return E_POINTER;
        if( GetCurrentThreadId() != _object_set->thread()->_thread_id )  return E_ACCESSDENIED;

        *result = _object_set->_object_set_descr->_level_interval._high_level;
    }
    return NOERROR;
}

//---------------------------------------------------------------------------------Com_job::Com_job

Com_job::Com_job( Job* job )
:
    Sos_ole_object( job_class_ptr, this ),
    _job(job)
{
}

//------------------------------------------------------------Com_job::start_when_directory_changed 

STDMETHODIMP Com_job::start_when_directory_changed( BSTR directory_name, BSTR filename_pattern )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  return E_POINTER;

        _job->start_when_directory_changed( bstr_as_string( directory_name ), bstr_as_string( filename_pattern ) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job::start_when_directory_changed" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job::start_when_directory_changed" ); }

    return hr;
}

//------------------------------------------------------------Com_job::clear_when_directory_changed 

STDMETHODIMP Com_job::clear_when_directory_changed()
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  return E_POINTER;
        _job->clear_when_directory_changed();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job::clear_when_directory_changed" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job::clear_when_directory_changed" ); }

    return hr;
}

//-----------------------------------------------------------------------------------Com_job::start

STDMETHODIMP Com_job::start( VARIANT* params, Itask** itask )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  return E_POINTER;

        Sos_ptr<Task>           task;

        CComPtr<Ivariable_set>  pars;
        Time                    start_at = 0; 

        if( params  &&  params->vt != VT_EMPTY  &&  params->vt != VT_NULL  &&  params->vt != VT_ERROR )
        {
            if( params->vt != VT_DISPATCH && params->vt != VT_UNKNOWN )  return DISP_E_TYPEMISMATCH;
            hr = params->punkVal->QueryInterface( IID_Ivariable_set, (void**)&pars );
            if( FAILED(hr) )  return hr;
        }

        CComVariant task_name_vt;
        if( pars )  pars->get_var( L"spooler_task_name", &task_name_vt );
        hr = task_name_vt.ChangeType( VT_BSTR );    if( FAILED(hr) )  throw_ole( hr, "ChangeType", "spooler_task_name" );

        CComVariant start_after_vt;
        if( pars )  pars->get_var( L"spooler_start_after", &start_after_vt );
        if( start_after_vt.vt != VT_EMPTY )
        {
            hr = start_after_vt.ChangeType( VT_R8 );    if( FAILED(hr) )  throw_ole( hr, "ChangeType", "spooler_start_after" );
            start_at = Time::now() + start_after_vt.dblVal;
        }

        THREAD_LOCK_LOG( _job->_lock, "Com_job::start" )
        {
            task = _job->start_without_lock( pars, bstr_as_string( task_name_vt.bstrVal ), start_at );
        }

        *itask = new Com_task( task );
        (*itask)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job::start" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job::start" ); }

    return hr;
}

//------------------------------------------------------------------------------------Com_job::wake

STDMETHODIMP Com_job::wake()
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  return E_POINTER;
        _job->wake();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job::start_when_directory_changed" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job::start_when_directory_changed" ); }

    return hr;
}

//------------------------------------------------------------------------------Com_job::get_thread

STDMETHODIMP Com_job::get_thread( spooler_com::Ithread** thread )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    {
        if( !_job )  return E_POINTER;

        *thread = _job->_thread->_com_thread;
        if( *thread )  (*thread)->AddRef();
    }

    return hr;
}

//------------------------------------------------------------------------Com_job::get_include_path

STDMETHODIMP Com_job::get_include_path( BSTR* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_job )  return E_POINTER;
        if( GetCurrentThreadId() != _job->thread()->_thread_id )  return E_ACCESSDENIED;

        *result = SysAllocString_string( _job->_thread->_include_path );
    }

    return NOERROR;
}

//--------------------------------------------------------------------------------Com_job::get_name

STDMETHODIMP Com_job::get_name( BSTR* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_job )  return E_POINTER;
        if( GetCurrentThreadId() != _job->thread()->_thread_id )  return E_ACCESSDENIED;

        *result = SysAllocString_string( _job->_name );
    }

    return NOERROR;
}

//--------------------------------------------------------------------------Com_job::put_state_text

STDMETHODIMP Com_job::put_state_text( BSTR text )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  throw_xc( "SPOOLER-122" );
        if( GetCurrentThreadId() != _job->thread()->_thread_id )  return E_ACCESSDENIED;

        _job->set_state_text( bstr_as_string( text ) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job.state_text" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job.state_text" ); }

    return hr;
}

//-------------------------------------------------------------------------------Com_job::get_title

STDMETHODIMP Com_job::get_title( BSTR* title )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  throw_xc( "SPOOLER-122" );

        *title = SysAllocString_string( _job->title() );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job.title" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job.title" ); }

    return hr;
}

//-------------------------------------------------------------------Com_job::put_delay_after_error

STDMETHODIMP Com_job::put_delay_after_error( int error_steps, VARIANT* time )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_job )  throw_xc( "SPOOLER-122" );

        _job->set_delay_after_error( error_steps, time_from_variant(*time) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Job.delay_after_error" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Job.delay_after_error" ); }

    return hr;
}



//-------------------------------------------------------------------------------Com_task::Com_task

Com_task::Com_task( Task* task )
:
    Sos_ole_object( task_class_ptr, this ),
    _task(task)
{
}

//-------------------------------------------------------------------------------Com_task::set_task

void Com_task::set_task( Task* task )
{ 
    THREAD_LOCK( _lock )
    {
        _task = task; 
    }
}

//-------------------------------------------------------------------------Com_task::get_object_set

STDMETHODIMP Com_task::get_object_set( Iobject_set** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );
        if( GetCurrentThreadId() != _task->_job->thread()->_thread_id )  return E_ACCESSDENIED;

        if( !_task->_job->object_set_descr() )  return E_ACCESSDENIED;
        THREAD_LOCK( _task->_job->_lock )  *result = (dynamic_cast<Object_set_task*>(+_task))->_com_object_set;
        if( *result )  (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task::object_set" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task::object_set" ); }

    return hr;
}

//----------------------------------------------------------------------------------Com_task::error

STDMETHODIMP Com_task::put_error( VARIANT* error_par )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );
        if( GetCurrentThreadId() != _task->_job->thread()->_thread_id )  return E_ACCESSDENIED;

        CComVariant error_vt = *error_par;
        hr = error_vt.ChangeType( VT_BSTR );        if( FAILED(hr) )  return hr;

        string error_text = bstr_as_string( error_vt.bstrVal );
        _task->_job->set_error( Xc( "SPOOLER-120", error_text.c_str() ) );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task::error" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task::error" ); }

    return hr;
}

//----------------------------------------------------------------------------------Com_task::error

STDMETHODIMP Com_task::get_error( Ierror** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );

        THREAD_LOCK( _task->_job->_lock )  *result = new Com_error( _task->_job->error() );
        (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.Error" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.Error" ); }

    return hr;
}

//--------------------------------------------------------------------------------Com_task::get_job

STDMETHODIMP Com_task::get_job( Ijob** com_job )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );

        *com_job = _task->_job->com_job();
        (*com_job)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.job" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.job" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_task::get_params

STDMETHODIMP Com_task::get_params( Ivariable_set** result )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );

        *result = _task->_params;
        (*result)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.params" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.params" ); }

    return hr;
}

//------------------------------------------------------------------Com_task::wait_until_terminated

STDMETHODIMP Com_task::wait_until_terminated( double wait_time, VARIANT_BOOL* ok )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( _task )  *ok = _task->wait_until_terminated( wait_time );
               else  *ok = true;
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.wait_until_terminated" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.wait_until_terminated" ); }

    return hr;
}

//------------------------------------------------------------------------------------Com_task::end

STDMETHODIMP Com_task::end()
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( _task )  _task->cmd_end();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.end" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.end" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_task::put_result

STDMETHODIMP Com_task::put_result( VARIANT* value )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );
        if( GetCurrentThreadId() != _task->_job->thread()->_thread_id )  return E_ACCESSDENIED;
        if( !_task->_job->its_current_task(_task) )  throw_xc( "SPOOLER-138" );

        THREAD_LOCK( _task->_job->_lock )  hr = _task->_result.Copy( value );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.result" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.result" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_task::get_result

STDMETHODIMP Com_task::get_result( VARIANT* value )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );

        VariantInit( value ); 
        THREAD_LOCK( _task->_job->_lock )  hr = VariantCopy( value, &_task->_result );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.result" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.result" ); }

    return hr;
}

//-----------------------------------------------------------------------------Com_task::put_repeat

STDMETHODIMP Com_task::put_repeat( double seconds )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );
        if( GetCurrentThreadId() != _task->_job->thread()->_thread_id )  return E_ACCESSDENIED;
        if( !_task->_job->its_current_task(_task) )  throw_xc( "SPOOLER-138" );

        _task->_job->set_repeat( seconds );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.repeat" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.repeat" ); }

    return hr;
}

//----------------------------------------------------------------------Com_task::put_history_field

STDMETHODIMP Com_task::put_history_field( BSTR name, VARIANT* value )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_task )  throw_xc( "SPOOLER-122" );
        if( GetCurrentThreadId() != _task->_job->thread()->_thread_id )  return E_ACCESSDENIED;
        if( !_task->_job->its_current_task(_task) )  throw_xc( "SPOOLER-138" );

        _task->set_history_field( bstr_as_string(name), *value );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Task.history_field" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Task.history_field" ); }

    return hr;
}

//---------------------------------------------------------------------------Com_thread::Com_thread

Com_thread::Com_thread( Thread* thread )
:
    Sos_ole_object( thread_class_ptr, this ),
    _thread(thread)
{
}

//------------------------------------------------------------------------------Com_thread::get_Log

STDMETHODIMP Com_thread::get_log( spooler_com::Ilog** com_log )
{
    THREAD_LOCK( _lock )
    {
        if( !_thread )  return E_POINTER;
        if( GetCurrentThreadId() != _thread->_thread_id )  return E_ACCESSDENIED;

        *com_log = _thread->_com_log;
        if( *com_log )  (*com_log)->AddRef();
    }

    return NOERROR;
}

//---------------------------------------------------------------------------Com_thread::get_script

STDMETHODIMP Com_thread::get_script( IDispatch** script_object )
{
    THREAD_LOCK( _lock )
    {
        if( !_thread )  return E_POINTER;
        if( GetCurrentThreadId() != _thread->_thread_id )  return E_ACCESSDENIED;

        *script_object = _thread->_script_instance.dispatch();
        if( *script_object )  (*script_object)->AddRef();
    }

    return NOERROR;
}

//---------------------------------------------------------------------Com_thread::get_include_path

STDMETHODIMP Com_thread::get_include_path( BSTR* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_thread )  return E_POINTER;
        if( GetCurrentThreadId() != _thread->_thread_id )  return E_ACCESSDENIED;

        *result = SysAllocString_string( _thread->_include_path );
    }

    return NOERROR;
}

//-----------------------------------------------------------------------------Com_thread::get_name

STDMETHODIMP Com_thread::get_name( BSTR* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_thread )  return E_POINTER;
        if( GetCurrentThreadId() != _thread->_thread_id )  return E_ACCESSDENIED;

        *result = SysAllocString_string( _thread->_name );
    }

    return NOERROR;
}

//-------------------------------------------------------------------------Com_spooler::Com_spooler

Com_spooler::Com_spooler( Spooler* spooler )
:
    Sos_ole_object( spooler_class_ptr, this ),
    _spooler(spooler)
{
}

//-----------------------------------------------------------------------------Com_spooler::get_Log

STDMETHODIMP Com_spooler::get_log( spooler_com::Ilog** com_log )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;

        *com_log = _spooler->_com_log;
        if( *com_log )  (*com_log)->AddRef();
    }

    return NOERROR;
}

//----------------------------------------------------------------------------------Com_spooler::id

STDMETHODIMP Com_spooler::get_id( BSTR* id_bstr )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;

        *id_bstr = SysAllocString_string( _spooler->id() );
    }

    return NOERROR;
}

//-------------------------------------------------------------------------------Com_spooler::param

STDMETHODIMP Com_spooler::get_param( BSTR* param_bstr )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;

        *param_bstr = SysAllocString_string( _spooler->_spooler_param );
    }

    return NOERROR;
}

//---------------------------------------------------------------------------Com_spooler::get_script

STDMETHODIMP Com_spooler::get_script( IDispatch** script_object )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;

        *script_object = _spooler->_script_instance.dispatch();
        if( *script_object )  (*script_object)->AddRef();
    }

    return NOERROR;
}

//-----------------------------------------------------------------------------Com_spooler::get_job

STDMETHODIMP Com_spooler::get_job( BSTR job_name, Ijob** com_job )
{
    HRESULT hr = NOERROR;

    THREAD_LOCK( _lock )
    try
    {
        if( !_spooler )  return E_POINTER;
        *com_job = _spooler->get_job( bstr_as_string( job_name ) )->com_job();
        (*com_job)->AddRef();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "Spooler.Spooler.job" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "Spooler.Spooler.job" ); }

    return hr;
}

//-----------------------------------------------------------------Com_spooler::create_variable_set

STDMETHODIMP Com_spooler::create_variable_set( Ivariable_set** result )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;

        *result = new Com_variable_set;
        (*result)->AddRef();
    }

    return NOERROR;
}

//--------------------------------------------------------------------Com_spooler::get_include_path

STDMETHODIMP Com_spooler::get_include_path( BSTR* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;
        THREAD_LOCK( _spooler->_lock )  *result = SysAllocString_string( _spooler->_include_path );
    }

    return NOERROR;
}

//-------------------------------------------------------------------------Com_spooler::get_log_dir

STDMETHODIMP Com_spooler::get_log_dir( BSTR* result )
{
    THREAD_LOCK( _lock )
    {
        if( !_spooler )  return E_POINTER;
        THREAD_LOCK( _spooler->_lock )  *result = SysAllocString_string( _spooler->_log_directory );
    }

    return NOERROR;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
