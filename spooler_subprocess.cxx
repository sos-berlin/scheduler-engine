// $Id: spooler_process.cxx 3305 2005-01-12 09:15:50Z jz $

/*
    Subprocess

    Objekte im Task-Prozess.
    Registriert Pid (mit ein paar Angaben für <show_state>) im Scheduler (im Objekt Task).


    Scheduler beendet Task erst, wenn alle Subprozesse beendet sind.
    Zustand s_ending_waiting_for_subprocesses.
    Task-Prozess beendet sich, aber ohne CloseHandle() bzw. waitpid(), bleibt also stehen.

    <kill_task> bricht alle Subprozesse ab (wie add_pid).

    stdout und stderr ins Task-Protokoll übernehmen.

    Fehler im Subprozess (signal oder Exit code) -> Task-Fehler

    Subprozesse, für die ein wait() erfolgreich ausgeführt worden ist, 
    werden nicht mehr vom Scheduler beobachtet.
*/


#include "spooler.h"


namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

Subprocess::Class_descriptor    Subprocess::class_descriptor ( &typelib, "Spooler.Subprocess", Subprocess::_methods );

//-----------------------------------------------------------------------------Subprocess::_methods

const Com_method Subprocess::_methods[] =
{ 
    COM_METHOD      ( Subprocess,  1, Close         , VT_EMPTY , 0, {}          ),
    COM_METHOD      ( Subprocess,  2, Start         , VT_EMPTY , 0, { VT_BYREF|VT_VARIANT }  ),
    COM_PROPERTY_PUT( Subprocess,  3, Priority      ,            0, { VT_INT }  ),
    COM_PROPERTY_GET( Subprocess,  3, Priority      , VT_INT   , 0, {}          ),
    COM_METHOD      ( Subprocess,  4, Raise_priority, VT_EMPTY , 0, { VT_INT  } ),
    COM_METHOD      ( Subprocess,  5, Lower_priority, VT_EMPTY , 0, { VT_INT  } ),
    COM_PROPERTY_GET( Subprocess,  6, Pid           , VT_INT   , 0, {}          ),
    COM_PROPERTY_GET( Subprocess,  7, Terminated    , VT_BOOL  , 0, {}          ),
    COM_PROPERTY_GET( Subprocess,  8, Exit_code     , VT_INT   , 0, {}          ),
    COM_PROPERTY_GET( Subprocess,  9, Stdout_path   , VT_BSTR  , 0, {}          ),
    COM_PROPERTY_GET( Subprocess, 10, Stderr_path   , VT_BSTR  , 0, {}          ),
    COM_PROPERTY_PUT( Subprocess, 11, Ignore_error  ,            0, { VT_BOOL } ),
    COM_PROPERTY_GET( Subprocess, 11, Ignore_error  , VT_BOOL  , 0, {}          ),
    COM_PROPERTY_PUT( Subprocess, 12, Ignore_signal ,            0, { VT_BSTR } ),
    COM_PROPERTY_GET( Subprocess, 12, Ignore_signal , VT_BOOL  , 0, { VT_BOOL } ),
    COM_METHOD      ( Subprocess, 13, Wait          , VT_BOOL  , 1, { VT_BYREF|VT_VARIANT } ),
    COM_METHOD      ( Subprocess, 14, Kill          , VT_EMPTY , 0, { VT_INT  } ),
    {}
};

//---------------------------------------------------------------------------Subprocess::Subprocess

Subprocess::Subprocess( Subprocess_register* subprocess_register, Com_task_proxy* task_proxy ) 
: 
    Idispatch_implementation( &class_descriptor ),
    _zero_(this+1),
    _subprocess_register(subprocess_register),
    _task_proxy(task_proxy)
{
}

//--------------------------------------------------------------------------Subprocess::~Subprocess

Subprocess::~Subprocess() 
{
    try
    {
        close();
    }
    catch( exception& x )  { Z_LOG( "Subprocess::close():  " << x.what() << "\n" ); }
}

//--------------------------------------------------------------------------------Subprocess::close

void Subprocess::close()
{
    if( _registered )  _subprocess_register->remove( this );
    _process.close();
}

//--------------------------------------------------------------------------------Subprocess::Start

STDMETHODIMP Subprocess::Start( VARIANT* program_and_parameters )
{
    // stderr und stdout in temporäre Datei

    HRESULT hr = S_OK;
    
    try
    {
        if( program_and_parameters->vt == VT_BSTR )
        {
            _process.start( string_from_variant( *program_and_parameters ) );
        }
        else
        if( program_and_parameters->vt == VT_ARRAY )
        {
            Locked_safearray params ( V_ARRAY( program_and_parameters ) );
            vector<string>   args   ( params.count() );

            for( int i = 0; i < params.count(); i++ )  args[ i ] = string_from_variant( params[ i ] );
                
            _process.start( args );
        }
        else
            hr = DISP_E_TYPEMISMATCH;

        _subprocess_register->add( this );

        if( _task_proxy )
        {
            _task_proxy->call( "Add_subprocess", _process.pid(), "never", ignore_error(), ignore_signal(), _process.command_line() ); //, subprocess->stdout_path(), subprocess->stderr_path() );
        }
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }
    
    return hr;
}

//---------------------------------------------------------------------------------Subprocess::Wait

STDMETHODIMP Subprocess::Wait( VARIANT* seconds, VARIANT_BOOL* result )
{ 
    HRESULT hr = S_OK;

    try
    {
        if( variant_is_missing( *seconds ) )
        {
            _process.wait();
            *result = VARIANT_TRUE;
        }
        else
        {
            bool ok = _process.wait( double_from_variant( *seconds ) );
            *result = ok? VARIANT_TRUE : VARIANT_FALSE;
        }
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//--------------------------------------------------------Subprocess_register::~Subprocess_register

Subprocess_register::~Subprocess_register()
{
    while( !_subprocess_map.empty() )  remove( _subprocess_map.begin()->second );
}

//------------------------------------------------------------Subprocess_register::Start_subprocess

STDMETHODIMP Subprocess_register::Start_subprocess( VARIANT* program_and_parameters, spooler_com::Isubprocess** result, 
                                                    Com_task_proxy* task_proxy )
{
    Z_LOGI( "Subprocess_register::Start_subprocess()\n" );
    HRESULT hr = S_OK;
    
    try
    {
        ptr<Subprocess> subprocess = Z_NEW( Subprocess( this, task_proxy ) );

        if( !variant_is_missing( *program_and_parameters ) )
        {
            hr = subprocess->Start( program_and_parameters );
        }

        *result = subprocess.take();
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }
    
    return hr;
}

//-------------------------------------------------------------------------Subprocess_register::add

void Subprocess_register::add( Subprocess* subprocess )
{
    ptr<Subprocess>& entry = _subprocess_map[ subprocess->_process.pid() ];

    if( entry )
    {
        Z_LOG( "Subprocess_register::add(" << subprocess->_process.pid() << "): Prozess mit gleicher Pid wird entfernt: " << subprocess->_process.obj_name() << "\n" );
        entry->_registered = false;
    }

    _subprocess_map[ subprocess->_process.pid() ] = subprocess;
    subprocess->_registered = true;
}

//----------------------------------------------------------------------Subprocess_register::remove

void Subprocess_register::remove( Subprocess* subprocess )
{
    if( subprocess->_registered )
    {
        subprocess->_registered = false;
        _subprocess_map.erase( subprocess->_process.pid() );
    }
}

//------------------------------------------------------------------------Subprocess_register::wait

void Subprocess_register::wait()
{
    Subprocess* error_subprocess = NULL;
    Subprocess* signal_subprocess = NULL;

    Z_FOR_EACH( Subprocess_map, _subprocess_map, s )
    {
        Subprocess* subprocess = s->second;

        subprocess->_process.wait();

        if( !subprocess->ignore_error()   &&  subprocess->_process.exit_code()          )  error_subprocess = subprocess;
        if( !subprocess->ignore_signal()  &&  subprocess->_process.termination_signal() )  signal_subprocess = subprocess;
    }


    if( signal_subprocess || error_subprocess )
    {
        Subprocess* p = signal_subprocess? signal_subprocess : error_subprocess;

        Xc x ( signal_subprocess? "SCHEDULER-219" : "SCHEDULER-218" );
        x.insert( signal_subprocess? signal_subprocess->_process.termination_signal()
                                      : error_subprocess->_process.exit_code()           );

        x.insert( p->_process.pid() );
        x.insert( p->_process.command_line() );
        
        throw_xc( x );
    }
    //if( signal_subprocess )  throw_xc( "SCHEDULER-219", "si=" + as_string( signal_subprocess->_process.termination_signal() ) + "  " + signal_subprocess->_process.command_line(), signal_subprocess->_process.pid()  );
    //if( error_subprocess  )  throw_xc( "SCHEDULER-218", as_string( error_subprocess ->_process.exit_code()          ), as_string( error_subprocess ->_process.pid() ), error_subprocess ->_process.command_line() );
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
