// $Id: threads_base.cxx 13322 2008-01-27 15:27:19Z jz $

#include "zschimmer.h"
#include "mutex.h"
#include "threads_base.h"
#include "log.h"
#include "java.h"

namespace zschimmer {


//--------------------------------------------------------------------------Event_base::~Event_base

Event_base::~Event_base()
{
}

//------------------------------------------------------------------------------Event_base::as_text

string Event_base::as_text() const
{ 
    string result = "Event " + _name; 
    if( _signaled  &&  !_signal_name.empty() )  result += " \"" + _signal_name + "\"";
    return result;
}

void Event_base::set_signaled_then_callback(const string& name) {
    _signaled = true;
    _signal_name = name;
    if (Call* c = _call) 
        c->call();
}

//--------------------------------------------------------------------Thread_base::thread_call_main

void Thread_base::thread_call_main()
{ 
    thread_init();
    if (javabridge::Vm* vm = javabridge::Vm::get_vm(false))
        vm->attach_thread("C++ " + _thread_name);

    int exit_code = call_thread_main();  

    if (javabridge::Vm* vm = javabridge::Vm::get_vm(false))
        vm->detach_thread();
    thread_exit( exit_code ); 
}

//--------------------------------------------------------------------Thread_base::call_thread_main

int Thread_base::call_thread_main()
{
    try
    {
        Z_LOG( "Thread " << _thread_name << " starts\n" );

        int ret = thread_main();

        Z_LOG( "Thread " << _thread_name << " terminated\n" );

        return ret;
    }
    catch( const exception& x )
    {
        string msg = "\n\n*** Thread " + /*as_string(thread_id()) + " " +*/ thread_name() + " bricht mit Fehler ab: " + x.what() + "***\n\n\n";
        Z_LOG( msg.c_str() );
        fputs( msg.c_str(), stderr );
        return 99;
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
