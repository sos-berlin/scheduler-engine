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

Subprocess::Class_descriptor    Subprocess::class_descriptor ( &typelib, "Spooler.Subprocess" );

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

//--------------------------------------------------------------------------------Subprocess::Close
/*
STDMETHODIMP Subprocess::Close()
{
    HRESULT hr = S_OK;
    
    try
    {
        close();
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }
    
    return hr;
}
*/
//--------------------------------------------------------------------------------Subprocess::close

void Subprocess::close()
{
    if( _registered )  _subprocess_register->remove( this );
    Process::close();
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
            Process::start( string_from_variant( *program_and_parameters ) );
        }
        else
        if( program_and_parameters->vt == VT_ARRAY )
        {
            Locked_safearray params ( V_ARRAY( program_and_parameters ) );
            vector<string>   args   ( params.count() );

            for( int i = 0; i < params.count(); i++ )  args[ i ] = string_from_variant( params[ i ] );
                
            Process::start( args );
        }
        else
            hr = DISP_E_TYPEMISMATCH;

        _subprocess_register->add( this );

        if( _task_proxy )
        {
            _task_proxy->_proxy->call( "Add_subprocess", pid(), ignore_error(), ignore_signal(), command_line() ); //, subprocess->stdout_path(), subprocess->stderr_path() );
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

STDMETHODIMP Subprocess_register::Start_subprocess( VARIANT* program_and_parameters, spooler_com::Isubprocess** result )
{
    Z_LOGI( "Subprocess_register::Start_subprocess()\n" );
    HRESULT hr = S_OK;
    
    try
    {
        ptr<Subprocess> subprocess = Z_NEW( Subprocess( this ) );

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
    ptr<Subprocess>& entry = _subprocess_map[ subprocess->pid() ];

    if( entry )
    {
        Z_LOG( "Subprocess_register::add(" << subprocess->pid() << "): Prozess mit gleicher Pid wird entfernt: " << subprocess->obj_name() << "\n" );
        entry->_registered = false;
    }

    _subprocess_map[ subprocess->pid() ] = subprocess;
    subprocess->_registered = true;
}

//----------------------------------------------------------------------Subprocess_register::remove

void Subprocess_register::remove( Subprocess* subprocess )
{
    if( subprocess->_registered )
    {
        _subprocess_map.erase( subprocess->pid() );
        subprocess->_registered = false;
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

        subprocess->wait();

        if( !subprocess->ignore_error()   &&  subprocess->exit_code()          )  error_subprocess = subprocess;
        if( !subprocess->ignore_signal()  &&  subprocess->termination_signal() )  signal_subprocess = subprocess;
    }

    if( signal_subprocess )  throw_xc( "SCHEDULER-219", as_string( signal_subprocess->pid() ), signal_subprocess->command_line() );
    if( error_subprocess  )  throw_xc( "SCHEDULER-219", as_string( error_subprocess->pid() ),  error_subprocess->command_line() );
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
