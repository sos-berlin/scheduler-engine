// $Id$

#include "zschimmer.h"
#ifndef Z_NOTHREADS

#include "mutex.h"
#include "threads.h"
#include "log.h"

namespace zschimmer {

//-------------------------------------------------------------------------------------------static

static bool                 do_log       = false;
static int                  do_log_state = -1;
static int                  recursive    = 0;

//-------------------------------------------------------------------------------Mutex_guard::enter_

void Mutex_guard::enter_( Mutex* m, const string& function, const char* file, int line_nr )  
{ 
    __assume(m); 

    _function = function;
    _file     = file;
    _line_nr  = line_nr;

    if( !m->_dont_log )
    {
        if( do_log_state != static_log_categories.modified_counter() )     // Eine Veränderung der static_log_categories bemerken wir.
        {                                                           // Damit kann Mutex vor dem ersten Setzen der Log_categories verwendet werden.
            recursive++;
            if( recursive == 1 )
            {
                do_log       = log_category_is_set( "mutex" );
                do_log_state = static_log_categories.modified_counter(); 
            }
            recursive--;
        }
    }

    if( do_log  && !m->_dont_log ) 
    {
        //fprintf( stderr, "%s %s %s %s %d\n", Z_FUNCTION.c_str(), m->_name.c_str(), _function.c_str(), _file, _line_nr );
        if( !m->try_enter() )
        {
            Z_LOG( "Mutex_guard::enter() " << m->_name << " in " << _function << "(), " << _file << ":" << _line_nr << " gesperrt von Thread " << m->locking_thread_id() << "...\n" );
            m->enter();
            Z_LOG( "Mutex_guard::enter() " << m->_name << " in " << _function << "(), ok\n" );
        }
        
    }
    else
    {
        m->enter();
    }

    _mutex = m;
}

//------------------------------------------------------------------------------Mutex_guard::leave_

void Mutex_guard::leave_()
{
    if( _mutex )
    {
        //if( do_log )  Z_LOG( "Mutex_guard::leave()  in " << _function << "  ok\n" );
        //if( do_log && !_mutex->_dont_log )  fprintf( stderr, "%s %s %s %s %d\n", Z_FUNCTION.c_str(), _mutex->_name.c_str(), _function.c_str(), _file, _line_nr );
        _mutex->leave();
        _mutex = NULL;
    }
}

//-------------------------------------------------------------------My_thread_only::My_thread_only
#ifndef Z_NOTHREADS

My_thread_only::My_thread_only()
: 
    _owners_thread_id( current_thread_id() ) 
{
}

//-----------------------------------------------------------------My_thread_only::is_owners_thread

bool My_thread_only::is_owners_thread() const
{ 
    bool result = current_thread_id() == _owners_thread_id; 

    if( !result )
        Z_LOG( "Objekt My_thread_only(0x" << hex_from_int(_owners_thread_id) << ") wird von Thread 0x" << hex_from_int(current_thread_id()) << " benutzt.\n" );

    return result;
}

//----------------------------------------------------------My_thread_only::assert_is_owners_thread

void My_thread_only::assert_is_owners_thread() const
{
    if( !is_owners_thread() )  throw_xc( "Z-4008", _owners_thread_id, current_thread_id() );
}

#endif
//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
