// $Id: spooler_com.cxx,v 1.8 2001/02/06 09:22:26 jz Exp $
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
DESCRIBE_CLASS( &spooler_typelib, Com_spooler     , spooler     , spooler_com::CLSID_spooler     , "Spooler.Spooler"     , "1.0" )

//-----------------------------------------------------------------------------Com_error::Com_error

Com_error::Com_error( const Xc_copy& x )
: 
    Sos_ole_object( error_class_ptr, this ),
    _xc(x) 
{
}

//------------------------------------------------------------------------------Com_error::is_error

STDMETHODIMP Com_error::get_is_error( VARIANT_BOOL* result )
{
    *result = _xc != NULL;
    return NOERROR;
}

//----------------------------------------------------------------------------------Com_error::code

STDMETHODIMP Com_error::get_code( BSTR* code_bstr )
{
    HRESULT hr = NOERROR;

    try
    {
        if( !_xc )  *code_bstr = NULL;
              else  *code_bstr = SysAllocStringLen_char( _xc->code(), strlen( _xc->code() ) );
    }
    catch( const Xc&   x )  { hr = _set_excepinfo(x); }
    catch( const xmsg& x )  { hr = _set_excepinfo(x); }

    return hr;
}

//----------------------------------------------------------------------------------Com_error::text

STDMETHODIMP Com_error::get_text( BSTR* text_bstr )
{
    HRESULT hr = NOERROR;

    try
    {
        if( !_xc )  *text_bstr = NULL;
              else  *text_bstr = SysAllocString_string( _xc->what() );
    }
    catch( const Xc&   x )  { hr = _set_excepinfo(x); }
    catch( const xmsg& x )  { hr = _set_excepinfo(x); }

    return hr;
}

//---------------------------------------------------------------Com_variable_set::Com_variable_set

Com_variable_set::Com_variable_set()
:
    Sos_ole_object( variable_set_class_ptr, this )
{
}

//------------------------------------------------------------------------Com_variable_set::put_var

STDMETHODIMP Com_variable_set::put_var( BSTR name, VARIANT* value )
{
    _map[name] = *value;
    return NOERROR;
}

//------------------------------------------------------------------------Com_variable_set::get_var

STDMETHODIMP Com_variable_set::get_var( BSTR name, VARIANT* value )
{
    return VariantCopy( value, &_map[name] );
}

//----------------------------------------------------------------------Com_variable_set::get_count

STDMETHODIMP Com_variable_set::get_count( int* result )
{
    *result = _map.size();
    return NOERROR;
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
    
STDMETHODIMP Com_log::msg  ( BSTR line )                       { return log( log_msg, line ); }
STDMETHODIMP Com_log::warn ( BSTR line )                       { return log( log_warn, line ); }
STDMETHODIMP Com_log::error( BSTR line )                       { return log( log_error, line ); }

//-------------------------------------------------------------------------------------Com_log::log

STDMETHODIMP Com_log::log( Log_kind kind, BSTR line )
{ 
    HRESULT hr = NOERROR;

    try 
    {
        if( !_log )  return E_POINTER;

        if( _log )  _log->log( kind, bstr_as_string( line ) ); 
    }
    catch( const Xc&   x )  { hr = _set_excepinfo(x); }
    catch( const xmsg& x )  { hr = _set_excepinfo(x); }

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
    if( !_object_set )  return E_POINTER;

    *result = _object_set->_object_set_descr->_level_interval._low_level;
    return NOERROR;
}

//-------------------------------------------------------------------Com_object_set::get_high_level

STDMETHODIMP Com_object_set::get_high_level( int* result )
{
    if( !_object_set )  return E_POINTER;

    *result = _object_set->_object_set_descr->_level_interval._high_level;
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

STDMETHODIMP Com_job::start_when_directory_changed( BSTR directory_name )
{
    HRESULT hr = NOERROR;
    if( !_job )  return E_POINTER;

    try
    {
        _job->start_when_directory_changed( bstr_as_string( directory_name ) );
    }
    catch( const Xc&   x )  { hr = _set_excepinfo(x); }
    catch( const xmsg& x )  { hr = _set_excepinfo(x); }

    return hr;
}

//-----------------------------------------------------------------------------------Com_job::start

STDMETHODIMP Com_job::start( VARIANT* params, Itask** itask )
{
    HRESULT hr = NOERROR;
    if( !_job )  return E_POINTER;

    try
    {
        CComPtr<Ivariable_set> pars;

        if( params  &&  params->vt != VT_EMPTY  &&  params->vt != VT_NULL  &&  params->vt != VT_ERROR )
        {
            if( params->vt != VT_DISPATCH && params->vt != VT_UNKNOWN )  return DISP_E_TYPEMISMATCH;
            hr = params->punkVal->QueryInterface( IID_Ivariable_set, (void**)&pars );
            if( FAILED(hr) )  return hr;
        }

        THREAD_SEMA( _job->_task_lock )
        {
            _job->start_without_lock( pars );
            *itask = new Com_task( _job->_task ); //_job->_com_task;
        }

        (*itask)->AddRef();
    }
    catch( const Xc&   x )  { hr = _set_excepinfo(x); }
    catch( const xmsg& x )  { hr = _set_excepinfo(x); }

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
    THREAD_SEMA( _lock )
    {
        _task = task; 
    }
}

//-------------------------------------------------------------------------Com_task::get_object_set

STDMETHODIMP Com_task::get_object_set( Iobject_set** result )
{
    HRESULT hr = NOERROR;

    try
    {
        THREAD_SEMA( _lock )
        {
            if( !_task )  throw_xc( "SPOOLER-122" );

            *result = _task->_com_object_set;
            if( *result )  (*result)->AddRef();
        }
    }
    catch( const Xc&   x )  { hr = _set_excepinfo(x); }
    catch( const xmsg& x )  { hr = _set_excepinfo(x); }

    return hr;
}

//----------------------------------------------------------------------------------Com_task::error

STDMETHODIMP Com_task::put_error( VARIANT* error_par )
{
    HRESULT hr = NOERROR;

    try
    {
        THREAD_SEMA( _lock )
        {
            if( !_task )  throw_xc( "SPOOLER-122" );

            CComVariant error_vt = *error_par;
            hr = error_vt.ChangeType( VT_BSTR );        if( FAILED(hr) )  return hr;

            string error_text = bstr_as_string( error_vt.bstrVal );
            _task->_job->error( Xc( "SPOOLER-120", error_text.c_str() ) );
        }
    }
    catch( const Xc&   x )  { hr = _set_excepinfo(x); }
    catch( const xmsg& x )  { hr = _set_excepinfo(x); }

    return hr;
}

//----------------------------------------------------------------------------------Com_task::error

STDMETHODIMP Com_task::get_error( Ierror** result )
{
    HRESULT hr = NOERROR;

    try
    {
        THREAD_SEMA( _lock )
        {
            if( !_task )  throw_xc( "SPOOLER-122" );

            *result = new Com_error( _task->_job->_error );   // _task? _task->_error : _error
            (*result)->AddRef();
        }
    }
    catch( const Xc&   x )  { hr = _set_excepinfo(x); }
    catch( const xmsg& x )  { hr = _set_excepinfo(x); }

    return hr;
}

//--------------------------------------------------------------------------------Com_task::get_job

STDMETHODIMP Com_task::get_job( Ijob** com_job )
{
    HRESULT hr = NOERROR;

    try
    {
        THREAD_SEMA( _lock )
        {
            if( !_task )  throw_xc( "SPOOLER-122" );

            *com_job = _task->_job->_com_job;
            (*com_job)->AddRef();
        }
    }
    catch( const Xc&   x )  { hr = _set_excepinfo(x); }
    catch( const xmsg& x )  { hr = _set_excepinfo(x); }

    return hr;
}

//-----------------------------------------------------------------------------Com_task::get_params

STDMETHODIMP Com_task::get_params( Ivariable_set** result )
{
    HRESULT hr = NOERROR;

    try
    {
        THREAD_SEMA( _lock )
        {
            if( !_task )  throw_xc( "SPOOLER-122" );

            *result = _task->_job->_params;
            (*result)->AddRef();
        }
    }
    catch( const Xc&   x )  { hr = _set_excepinfo(x); }
    catch( const xmsg& x )  { hr = _set_excepinfo(x); }

    return hr;
}

//------------------------------------------------------------------Com_task::wait_until_terminated

STDMETHODIMP Com_task::wait_until_terminated( double wait_time, VARIANT_BOOL* ok )
{
    HRESULT hr = NOERROR;

    try
    {
        THREAD_SEMA( _lock )
        {
            if( _task )  *ok = _task->wait_until_terminated( wait_time );
                   else  *ok = true;
        }
    }
    catch( const Xc&   x )  { hr = _set_excepinfo(x); }
    catch( const xmsg& x )  { hr = _set_excepinfo(x); }

    return hr;
}

//-----------------------------------------------------------------------------Com_task::put_result

STDMETHODIMP Com_task::put_result( VARIANT* value )
{
    HRESULT hr = NOERROR;

    try
    {
        THREAD_SEMA( _lock )
        {
            if( !_task )  throw_xc( "SPOOLER-122" );

            hr = _task->_result.Copy( value );
        }
    }
    catch( const Xc&   x )  { hr = _set_excepinfo(x); }
    catch( const xmsg& x )  { hr = _set_excepinfo(x); }

    return hr;
}

//-----------------------------------------------------------------------------Com_task::get_result

STDMETHODIMP Com_task::get_result( VARIANT* value )
{
    HRESULT hr = NOERROR;

    try
    {
        THREAD_SEMA( _lock )
        {
            if( !_task )  throw_xc( "SPOOLER-122" );

            VariantInit( value ); 
            hr = VariantCopy( value, &_task->_result);
        }
    }
    catch( const Xc&   x )  { hr = _set_excepinfo(x); }
    catch( const xmsg& x )  { hr = _set_excepinfo(x); }

    return hr;
}

//-----------------------------------------------------------------------------Com_task::put_repeat

STDMETHODIMP Com_task::put_repeat( double seconds )
{
    HRESULT hr = NOERROR;

    try
    {
        THREAD_SEMA( _lock )
        {
            if( !_task )  throw_xc( "SPOOLER-122" );

            _task->_job->set_repeat( seconds );
        }
    }
    catch( const Xc&   x )  { hr = _set_excepinfo(x); }
    catch( const xmsg& x )  { hr = _set_excepinfo(x); }

    return hr;
}

//-------------------------------------------------------------------------Com_spooler::Com_spooler

Com_spooler::Com_spooler( Spooler* spooler )
:
    Sos_ole_object( spooler_class_ptr, this ),
    _spooler(spooler)
{
}

//--------------------------------------------------------------------Com_spooler::get_current_task
/*
STDMETHODIMP Com_spooler::get_current_task( spooler_com::Itask* com_task )
{
    if( !_spooler )  return E_POINTER;

    *com_task = _task->_com_task;
    return NOERROR;
}
*/
//-----------------------------------------------------------------------------Com_spooler::get_Log

STDMETHODIMP Com_spooler::get_log( spooler_com::Ilog** com_log )
{
    if( !_spooler )  return E_POINTER;

    *com_log = _spooler->_com_log;
    if( *com_log )  (*com_log)->AddRef();
    return NOERROR;
}

//----------------------------------------------------------------------------------Com_spooler::id

STDMETHODIMP Com_spooler::get_id( BSTR* id_bstr )
{
    if( !_spooler )  return E_POINTER;

    *id_bstr = SysAllocString_string( _spooler->_spooler_id );
    return NOERROR;
}

//-------------------------------------------------------------------------------Com_spooler::param

STDMETHODIMP Com_spooler::get_param( BSTR* param_bstr )
{
    if( !_spooler )  return E_POINTER;

    *param_bstr = SysAllocString_string( _spooler->_spooler_param );
    return NOERROR;
}

//-----------------------------------------------------------------------------Com_spooler::get_Log

STDMETHODIMP Com_spooler::get_script( IDispatch** script_object )
{
    if( !_spooler )  return E_POINTER;
    return E_POINTER;
/*
    *script_object = _spooler->_script_instance.dispatch();
    if( *script_object )  (*script_object)->AddRef();
    return NOERROR;
*/
}

//-----------------------------------------------------------------------------Com_spooler::get_job

STDMETHODIMP Com_spooler::get_job( BSTR job_name, Ijob** com_job )
{
    HRESULT hr = NOERROR;

    if( !_spooler )  return E_POINTER;

    try
    {
        *com_job = _spooler->get_job( bstr_as_string( job_name ) )->_com_job;
        (*com_job)->AddRef();
    }
    catch( const Xc&   x )  { hr = _set_excepinfo(x); }
    catch( const xmsg& x )  { hr = _set_excepinfo(x); }

    return hr;
}

//------------------------------------------------------------------Com_spooler::create_variable_set

STDMETHODIMP Com_spooler::create_variable_set( Ivariable_set** result )
{
    *result = new Com_variable_set;
    (*result)->AddRef();
    return NOERROR;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
