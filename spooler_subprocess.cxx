// $Id$

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
    COM_PROPERTY_GET( Subprocess,  1, Java_class_name, VT_BSTR   , 0, {}          ),
    COM_METHOD      ( Subprocess,  2, Close          , VT_EMPTY  , 0, {}          ),
    COM_METHOD      ( Subprocess,  3, Start          , VT_EMPTY  , 0, { VT_BYREF|VT_VARIANT }  ),
  //COM_PROPERTY_PUT( Subprocess,  4, Priority       ,             0, { VT_BYREF|VT_VARIANT }  ),
  //COM_PROPERTY_GET( Subprocess,  4, Priority       , VT_VARIANT, 0, {}          ),
    COM_PROPERTY_GET( Subprocess,  6, Pid            , VT_INT    , 0, {}          ),
    COM_PROPERTY_GET( Subprocess,  7, Terminated     , VT_BOOL   , 0, {}          ),
    COM_PROPERTY_GET( Subprocess,  8, Exit_code      , VT_INT    , 0, {}          ),
  //COM_PROPERTY_GET( Subprocess,  9, Stdout_path    , VT_BSTR   , 0, {}          ),
  //COM_PROPERTY_GET( Subprocess, 10, Stderr_path    , VT_BSTR   , 0, {}          ),
    COM_PROPERTY_PUT( Subprocess, 11, Ignore_error   ,             0, { VT_BOOL } ),
    COM_PROPERTY_GET( Subprocess, 11, Ignore_error   , VT_BOOL   , 0, {}          ),
    COM_PROPERTY_PUT( Subprocess, 12, Ignore_signal  ,             0, { VT_BSTR } ),
    COM_PROPERTY_GET( Subprocess, 12, Ignore_signal  , VT_BOOL   , 0, { VT_BOOL } ),
    COM_PROPERTY_PUT( Subprocess, 13, Timeout        ,             0, { VT_R8 } ),
    COM_METHOD      ( Subprocess, 14, Wait_for_termination, VT_BOOL   , 1, { VT_BYREF|VT_VARIANT } ),
    COM_METHOD      ( Subprocess, 15, Kill           , VT_EMPTY  , 0, { VT_INT  } ),
    {}
};

//---------------------------------------------------------------------------Subprocess::Subprocess

Subprocess::Subprocess( Subprocess_register* subprocess_register, IDispatch* task ) 
: 
    Idispatch_implementation( &class_descriptor ),
    _zero_(this+1),
    _subprocess_register(subprocess_register),
    _task(task),                // Itask oder Itask_proxy
    _timeout( INT_MAX )
{
}

//--------------------------------------------------------------------------Subprocess::~Subprocess

Subprocess::~Subprocess() 
{
    //Z_LOG2( "joacim", "~Subprocess()\n" );

    try
    {
        close();
    }
    catch( exception& x )  { Z_LOG( "Subprocess::close():  " << x.what() << "\n" ); }
}

//--------------------------------------------------------------------------------Subprocess::close

void Subprocess::close()
{
    deregister();
    _process.close();
}

//---------------------------------------------------------------------------Subprocess::deregister

void Subprocess::deregister()
{
    if( _registered )
    {
        Update_register_entry();
        _subprocess_register->remove( this );
    }
}

//--------------------------------------------------------------------------------Subprocess::Start

STDMETHODIMP Subprocess::Start( VARIANT* program_and_parameters )
{
    // stderr und stdout in temporäre Datei

    HRESULT hr = S_OK;
    
#   ifdef Z_WINDOWSxxx // Test
        UINT previous_error_mode = 0;
        if( _ignore_error ) 
        {
            // Das System soll sich Messageboxen verkneifen
            Z_LOG( "SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX | SEM_NOGPFAULTERRORBOX )\n" );
            previous_error_mode = SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX | SEM_NOGPFAULTERRORBOX );    
        }
#   endif


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
            return DISP_E_TYPEMISMATCH;

        _subprocess_register->add( this );

        hr = Update_register_entry();
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }


#   ifdef Z_WINDOWSxxx // Test
        if( _ignore_error )
        {
            Z_LOG( "SetErrorMode(" << previous_error_mode << ")\n" );
            SetErrorMode( previous_error_mode );
        }
#   endif


    return hr;
}

//-----------------------------------------------------------------Subprocess::Wait_for_termination

STDMETHODIMP Subprocess::Wait_for_termination( VARIANT* seconds, VARIANT_BOOL* result )
{ 
    HRESULT hr = S_OK;

    try
    {
        bool ok;

        if( variant_is_missing( *seconds ) )
        {
            wait_for_termination();
            ok = true;
        }
        else
        {
            ok = wait_for_termination( double_from_variant( *seconds ) );
        }

        *result = ok? VARIANT_TRUE : VARIANT_FALSE;
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------------------Subprocess::wait_for_termination

void Subprocess::wait_for_termination()
{ 
    _process.wait();
    deregister();
}

//-----------------------------------------------------------------Subprocess::wait_for_termination

bool Subprocess::wait_for_termination( double seconds )
{ 
    bool terminated = _process.wait( seconds );
    if( terminated )  deregister();
    return terminated;
}

//---------------------------------------------------------------------Subprocess::put_Ignore_error

STDMETHODIMP Subprocess::put_Ignore_error( VARIANT_BOOL b )
{ 
    _ignore_error = b != 0;  
    return Update_register_entry();
}

//---------------------------------------------------------------------Subprocess::get_Ignore_error

STDMETHODIMP Subprocess::get_Ignore_error( VARIANT_BOOL* result )
{ 
    *result = _ignore_error? VARIANT_TRUE: VARIANT_FALSE;  
    return S_OK;
}

//--------------------------------------------------------------------Subprocess::put_Ignore_signal

STDMETHODIMP Subprocess::put_Ignore_signal( VARIANT_BOOL b )                      
{ 
    _ignore_signal= b != 0;  
    return S_OK; 
}

//--------------------------------------------------------------------Subprocess::get_Ignore_signal

STDMETHODIMP Subprocess::get_Ignore_signal( VARIANT_BOOL* result )                
{ 
    *result = _ignore_signal? VARIANT_TRUE: VARIANT_FALSE;  
    return Update_register_entry();
}

//--------------------------------------------------------------------------Subprocess::put_Timeout

STDMETHODIMP Subprocess::put_Timeout( double timeout )
{
    _timeout = timeout;
    return Update_register_entry();
}

//----------------------------------------------------------------Subprocess::Update_register_entry

HRESULT Subprocess::Update_register_entry()
{
    HRESULT hr = S_OK;
    
    try
    {
        if( !_process.terminated() )
        {
            vector<Variant> variant_array;

            // Rückwärts!
            variant_array.push_back( _process.command_line() );
            variant_array.push_back( ignore_signal() );
            variant_array.push_back( ignore_error() );
            variant_array.push_back( (int)( _timeout + 0.999 ) );
            variant_array.push_back( _process.pid() );

            com_invoke( DISPATCH_METHOD, _task, "Add_subprocess", &variant_array );
        }
        else
        {
            vector<Variant> variant_array;

            // Rückwärts!
            variant_array.push_back( _process.pid() );

            com_invoke( DISPATCH_METHOD, _task, "Remove_pid", &variant_array );
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

//-----------------------------------------------------------Subprocess_register::Create_subprocess

STDMETHODIMP Subprocess_register::Create_subprocess( VARIANT* program_and_parameters, spooler_com::Isubprocess** result, 
                                                     IDispatch* task )
{
    Z_LOGI( "Subprocess_register::Create_subprocess(" << debug_string_from_variant( *program_and_parameters ) << ")\n" );
    HRESULT hr = S_OK;
    
    try
    {
        ptr<Subprocess> subprocess = Z_NEW( Subprocess( this, task ) );

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
        
        for( Subprocess_map::iterator s = _subprocess_map.begin(); s != _subprocess_map.end(); )
        {
            if( s->second == subprocess )  
            {
                _subprocess_map.erase( s );
                break;
            }
            else  
                s++;
        }
    }
}

//------------------------------------------------------------------------Subprocess_register::wait

void Subprocess_register::wait()
{
    Subprocess* error_subprocess  = NULL;
    Subprocess* signal_subprocess = NULL;


    Subprocess_map copy = _subprocess_map;

    Z_FOR_EACH( Subprocess_map, copy, s )
    {
        Subprocess* subprocess = s->second;

        subprocess->wait_for_termination();

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
