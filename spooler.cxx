// $Id: spooler.cxx,v 1.1 2001/01/02 10:49:02 jz Exp $

//#include <precomp.h>

#include "../kram/sos.h"
#include "../kram/sleep.h"
#include "spooler.h"



namespace sos {

extern const Bool _dll = false;

namespace spooler {

/*  ABLAUF
    
    Konfiguration über SAX laden
    dispatch()

    
    dispatch() =
        Laufenden Job anhand Priorisierung auswählen, step() (also get() und process())

        Neuer Job:
            Nächsten nicht laufenden Job auswählen
            Skript der Objektemengeklasse laden und Parameter (Stufenintervall) setzen
            Objektemenge nicht leer (!eof)? Neuer Job!



    


    - Über alle Jobs:
        - 
        - 
        - process_all() vorhanden?
            - process_all() rufen
          else
            - open()
            - eof()? Job nicht starten
            - Schleife: get(), process()
            - finish() wenn vorhanden
            - close()

    
*/

//---------------------------------------------------------------------------------------Job::start

void Job::start()
{
    _script_site = new Script_site;
    _script_site->_engine_name = _object_set._object_set_class->_script_language;
    _script_site->init_engine();

    _script_site->parse( _object_set._object_set_class->_script );

    _script_site->call( "spooler_open" );

    _running = true;
}

//-----------------------------------------------------------------------------------------Job::end

void Job::end()
{
    _script_site->call( "spooler_close" );
    _script_site->close_engine();
    _script_site = NULL;
    _running = false;
}

//----------------------------------------------------------------------------------------Job::step

bool Job::step()
{
    CComVariant result = _script_site->call( "spooler_get" );

    if( result.vt == VT_EMPTY )  return false;
    if( result.vt != VT_DISPATCH )  throw_xc( "SPOOLER-102", _object_set._object_set_class_name );
    if( result.pdispVal == NULL )  return false;
    
    com_invoke( result.pdispVal, "process", _output_level );

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