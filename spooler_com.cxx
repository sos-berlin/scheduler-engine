// $Id: spooler_com.cxx,v 1.1 2001/01/13 18:43:59 jz Exp $


#include "../kram/sos.h"
#include "spooler.h"

#ifdef SYSTEM_WIN

namespace sos {
namespace spooler {

using namespace std;
using namespace spooler_com;

//------------------------------------------------------------------------------------Typbibliothek

Typelib_descr spooler_typelib ( spooler_com::LIBID_spooler_com, "Spooler", "1.0" );

DESCRIBE_CLASS( &spooler_typelib, Com_log       , log       , spooler_com::CLSID_Log       , "Spooler.Log"       , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_task      , task      , spooler_com::CLSID_Task      , "Spooler.Task"      , "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_object_set, object_set, spooler_com::CLSID_Object_set, "Spooler.Object_set", "1.0" )
DESCRIBE_CLASS( &spooler_typelib, Com_spooler   , spooler   , spooler_com::CLSID_spooler   , "Spooler.Spooler"   , "1.0" )

//---------------------------------------------------------------------------------Com_log::Com_log

Com_log::Com_log( Log* log )
:
    Sos_ole_object( log_class_ptr, this ),
    _zero_(this+1),
    _log(log)
{ 
}

//---------------------------------------------------------------------------------Com_log::Com_log

Com_log::Com_log( Task* task )
:
    Sos_ole_object( log_class_ptr, this ),
    _zero_(this+1),
    _task(task)
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
        if( _task )  _task->_log.log( kind, bstr_as_string( line ) ); 
        else
        if( _log )  _log->log( kind, empty_string, bstr_as_string( line ) ); 
        else
            hr = E_POINTER;
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

//-------------------------------------------------------------------------------Com_task::Com_task

Com_task::Com_task( Task* task )
:
    Sos_ole_object( task_class_ptr, this ),
    _task(task)
{
}

//-------------------------------------------------------------------------Com_task::get_object_set

STDMETHODIMP Com_task::get_Object_set( Iobject_set** result )
{
    if( !_task )  return E_POINTER;

    *result = _task->_com_object_set;
    if( *result )  (*result)->AddRef();
    return NOERROR;
}

//------------------------------------------------------------Com_task::wake_when_directory_changed 

STDMETHODIMP Com_task::wake_when_directory_changed( BSTR directory_name )
{
    HRESULT hr = NOERROR;
    if( !_task )  return E_POINTER;

    try
    {
        _task->wake_when_directory_changed( bstr_as_string( directory_name ) );
    }
    catch( const Xc&   x )  { hr = _set_excepinfo(x); }
    catch( const xmsg& x )  { hr = _set_excepinfo(x); }

    return NOERROR;
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

STDMETHODIMP Com_spooler::get_Log( spooler_com::Ilog** com_log )
{
    if( !_spooler )  return E_POINTER;

    *com_log = _spooler->_com_log;
    if( *com_log )  (*com_log)->AddRef();
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

    *script_object = _spooler->_script_instance.dispatch();
    if( *script_object )  (*script_object)->AddRef();
    return NOERROR;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
