// $Id: spooler.cxx,v 1.2 2001/01/02 12:50:24 jz Exp $

//#include <precomp.h>

#include "../kram/sos.h"
#include "../kram/sleep.h"
#include "spooler.h"



namespace sos {

extern const Bool _dll = false;

namespace spooler {

//---------------------------------------------------------------------------------------Job::start

void Job::start()
{
    _script_site = new Script_site;
    _script_site->_engine_name = _object_set_descr._class->_script_language;
    _script_site->init_engine();

    _script_site->parse( _object_set_descr._class->_script );

    CComVariant object_set_vt = _script_site->call( "spooler_make_object_set()" );
    if( object_set_vt.vt != VT_DISPATCH )  throw_xc( "SPOOLER-103", _object_set_descr._class_name );
    _object_set = object_set_vt.pdispVal;

    com_invoke( _object_set, "spooler_open()" );

    _running = true;
}

//-----------------------------------------------------------------------------------------Job::end

void Job::end()
{
    com_invoke( _object_set, "spooler_close()" );

    _script_site->close_engine();
    _script_site = NULL;
    _running = false;
}

//----------------------------------------------------------------------------------------Job::step

bool Job::step()
{
    CComVariant object;

    while(1)
    {
        object = com_invoke( _object_set, "spooler_get()" );

        if( object.vt == VT_EMPTY )  return false;
        if( object.vt != VT_DISPATCH )  throw_xc( "SPOOLER-102", _object_set_descr._class_name );
        if( object.pdispVal == NULL )  return false;

        // Level im gültigen Bereich?
        CComVariant level = com_invoke( object.pdispVal, "level" );
        level.ChangeType( VT_INT );
        if( level.intVal >= _object_set_descr._level_interval._low_level 
         && level.intVal < _object_set_descr._level_interval._high_level )  break;
    }
    
    com_invoke( object.pdispVal, "process()", _output_level );

    return true;
}

//------------------------------------------------------------------------------------Spooler::step

bool Spooler::step()
{
    FOR_EACH( Job_list, _job_list, it )
    {
        Job* job = *it;

        if( !job->_running )
        {
            if( time(NULL) >= job->_next_try )
            {
                job->start();
                job->_next_try = time(NULL) + _next_try_period;
            }
        }
        else
        {
            bool ok = job->step();
            if( !ok ) 
            {
                job->end();
                //it = _job_list.erase( it );
            }
        }
    }

    return true;
}

//------------------------------------------------------------------------------------Spooler::load

void Spooler::load()
{
    _next_try_period = 1;   // Sekunden
    load_xml();
}

//-------------------------------------------------------------------------------------Spooler::run

void Spooler::run()
{
    while(1)
    {
        step();
        sos_sleep(1);
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler

//-----------------------------------------------------------------------------------------sos_main

int sos_main( int argc, char** argv )
{
    HRESULT hr = CoInitialize(NULL);
    if( FAILED(hr) )  throw_ole( hr, "CoInitialize" );

    spooler::Spooler spooler;

    spooler.load();
    spooler.run();

    CoUninitialize();
    return 0;
}


}// namespace sos