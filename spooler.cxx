// $Id: spooler.cxx,v 1.3 2001/01/02 13:51:36 jz Exp $

//#include <precomp.h>

#include "../kram/sos.h"
#include "../kram/sleep.h"
#include "spooler.h"



namespace sos {

extern const Bool _dll = false;

namespace spooler {

//----------------------------------------------------------------------------Spooler_object::level

Level Spooler_object::level()
{
    CComVariant level = com_invoke( _dispatch, "level" );
    level.ChangeType( VT_INT );

    return level.intVal;
}

//--------------------------------------------------------------------------Spooler_object::process

void Spooler_object::process( Level output_level )
{
    com_invoke( _dispatch, "process()", output_level );
}

//---------------------------------------------------------------------------------Object_set::open

void Object_set::open()
{
    _script_site = new Script_site;
    _script_site->_engine_name = _object_set_descr->_class->_script_language;
    _script_site->init_engine();

    _script_site->parse( _object_set_descr->_class->_script );

    CComVariant object_set_vt = _script_site->call( "spooler_make_object_set()" );
    if( object_set_vt.vt != VT_DISPATCH )  throw_xc( "SPOOLER-103", _object_set_descr->_class_name );
    _dispatch = object_set_vt.pdispVal;

    com_invoke( _dispatch, "spooler_open()" );
}

//--------------------------------------------------------------------------------Object_set::close

void Object_set::close()
{
    com_invoke( _dispatch, "spooler_close()" );

    _script_site->close_engine();
    _script_site = NULL;
}

//----------------------------------------------------------------------------------Object_set::get

Spooler_object Object_set::get()
{
    Spooler_object object;

    while(1)
    {
        CComVariant obj = com_invoke( _dispatch, "spooler_get()" );

        if( obj.vt == VT_EMPTY    )  return Spooler_object(NULL);
        if( obj.vt != VT_DISPATCH )  throw_xc( "SPOOLER-102", _object_set_descr->_class_name );
        
        object = obj.pdispVal;

        if( obj.pdispVal == NULL )  break;  // EOF
        if( _object_set_descr->_level_interval.is_in_interval( object.level() ) )  break;
    }

    return object;
}

//---------------------------------------------------------------------------------------Job::start

void Job::start()
{
    _object_set = SOS_NEW( Object_set( &_job_descr->_object_set_descr ) );
    _object_set->open();

    _running = true;
}

//-----------------------------------------------------------------------------------------Job::end

void Job::end()
{
    _object_set->close();

    _running = false;
}

//----------------------------------------------------------------------------------------Job::step

bool Job::step()
{
    Spooler_object object = _object_set->get();

    if( object.is_null() )  return false;
    
    object.process( _job_descr->_output_level );

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

//-----------------------------------------------------------------------------------Spooler::start

void Spooler::start()
{
    FOR_EACH( Job_descr_list, _job_descr_list, it )
    {
        _job_list.push_back( SOS_NEW( Job( *it ) ) );
    }
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
    spooler.start();
    spooler.run();

    CoUninitialize();
    return 0;
}


}// namespace sos