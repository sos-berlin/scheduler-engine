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

//------------------------------------------------------------------------Spooler::start_subprocess
    
ptr<Subprocess> Spooler::start_subprocess( const string& cmd_line )
{
}

//---------------------------------------------------------------------------Subprocess::Subprocess

Subprocess::Subprocess()
{
}

//--------------------------------------------------------------------------------Subprocess::close

void Subprocess::close()
{
    Process::close();
}

//--------------------------------------------------------------------------------Subprocess::start

void Subprocess::start( const string& cmd_line )
{
    // stderr und stdout in temporäre Datei


    Process::start( cmd_line );

    after_start();
}

//--------------------------------------------------------------------------------Subprocess::start

void Subprocess::start( const vector<string>& cmd_line )
{
    Process::start( cmd_line );

    after_start();
}

//--------------------------------------------------------------------------Subprocess::after_start

void Subprocess::after_start()
{
    _task_com.register_subprocess_pid( pid() );
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
