// $Id: async.cxx 14178 2011-01-18 15:10:17Z rb $

#include "zschimmer.h"
#include "async.h"
#include "log.h"
#include "z_sockets.h"
#include "string_list.h"

using namespace std;

namespace zschimmer {

//-------------------------------------------------------------------------------------------static

Sync_operation                      dummy_sync_operation;

//------------------------------------------------------------------sync_operation::Async_operation

Async_operation::Async_operation( Async_manager* m )
: 
    _zero_(this+1), 
    _manager(m), 
    //_signaled(true),
    _next_gmtime(double_time_max) 
{
}

//----------------------------------------------------------------Async_operation::~Async_operation

Async_operation::~Async_operation()
{ 
    if( _child )  
    {
        Z_LOG( "*** Async_operation::set_async_parent(NULL): Superoperation wird geschlossen vor " + _child->async_state_text() + "\n" );

        _child->async_abort();

        if( _child )  _child->_parent = NULL; 
    }

    if( _manager )  _manager->remove_operation( this );
}

//---------------------------------------------------------------Async_operation::set_async_manager

void Async_operation::set_async_manager( Async_manager* manager )
{
    if( _manager )  _manager->remove_operation( this );
    _manager = manager;
    if( _manager )  _manager->add_operation( this );
}

//-----------------------------------------------------------------Async_operation::set_async_delay

void Async_operation::set_async_delay( double delay )
{
    set_async_next_gmtime( delay >= double_time_max? double_time_max :
                           delay <= 0              ? 0
                                                   : double_from_gmtime() + delay );
}

//-----------------------------------------------------------Async_operation::set_async_next_gmtime

void Async_operation::set_async_next_gmtime( double t )
{ 
    if( t != _next_gmtime )
    {
        if( _manager )  _manager->remove_operation( this );
        _next_gmtime = min( t, double_time_max ); 
        if( _manager )  _manager->add_operation( this );
    }
}

//-------------------------------------------------------Async_operation::async_next_gmtime_reached

bool Async_operation::async_next_gmtime_reached() const
{
    if( _next_gmtime <= 0               )  return true;     // Sofort
    if( _next_gmtime >= double_time_max )  return false;    // Nie

    return _next_gmtime <= double_from_gmtime();         // Erst jetzt die Uhr ablesen
}

//-----------------------------------------------------------------Async_operation::set_async_child

void Async_operation::set_async_child( Async_operation* child )
{ 
    if( _child )  _child->_parent = NULL;
    if(  child )   child->_parent = this;
    _child = child;  
}

//--------------------------------------------------------------------Async_operation::async_finish

void Async_operation::async_finish_()
{
    while( !async_finished() )  async_continue( cont_wait );
}

//------------------------------------------------------------------Async_operation::async_continue

bool Async_operation::async_continue( Continue_flags flags )
{
    ptr<Async_operation> holder = this;

    bool something_done = false;

    try
    {
        if( _child )        // async_child() == NULL bei _call_state == c_begin oder c_end
        {
            something_done = _child->async_continue( flags & cont_wait );

            if( flags & cont_wait )  _child->async_finish();
            else  
            if( !_child->async_finished() )  return something_done;

            if( _child->async_has_error() )  return something_done;
        }


#       ifdef Z_DEBUG
            if( Log_ptr log = static_log_categories_cache._async )
            {
                log << Z_FUNCTION << "(";
                if( flags & cont_next_gmtime_reached )  log << "cont_next_gmtime_reached ";
                if( flags & cont_wait                )  log << "wait ";
                log << ")  ";
                log << async_state_text();
                log << "\n";
            }
#       endif

        if( _next_gmtime == 0 )  set_async_next_gmtime( double_time_max );  // 2006-06-19

        something_done = async_continue_( flags );

        if( !something_done )
        {
            if( Log_ptr log = static_log_categories_cache._async )
            {
                log << Z_FUNCTION << "(";
                if( flags & cont_next_gmtime_reached )  log << "cont_next_gmtime_reached ";
                if( flags & cont_wait                )  log << "wait ";
                log << ")  ";
                log << async_state_text();
                log << " => nothing done!\n";
            }
        }
    }
    catch( const Xc& x )
    {
        Z_LOG( Z_FUNCTION << " ERROR " << x.what() << ", " << async_state_text() << "\n" );
        
        string what = x.what2();  // JS-540 ...
        std::string::size_type found =  what.find_first_of("server::construct   [5] [java_class_path]");
        if(found != string::npos)
            Z_LOG2( "scheduler", "Version of Remote Scheduler must be 1.3.9 or higher " << x.what() << "\n" );  // ... JS-540

        _error = true;
        _error_name = x.name();
        _error_code = x.code();
        _error_what = x.what();

        async_clear_signaled();
        something_done = true;
    }
    catch( const exception& x )
    {
        Z_LOG( Z_FUNCTION << " ERROR " << x.what() << ", " << async_state_text() << "\n" );
        _error = true;
        _error_code = "Z-REMOTE-100";
        _error_what = x.what();

        async_clear_signaled();
        something_done = true;
    }


    if( !_child  &&  something_done  &&  _parent  &&  async_finished() )
    {
        // Wie haben zwei Verfahren, ein altes (für com_remote.cxx) und ein neues
        // 
        // _child != NULL: 
        // Die Vater-Operation ruft die Kind-Operation auf. Wenn diese finshed ist, wird die Vater-Operation fortgesetzt. Das passiert oben.

        // _child == NULL && _parent != NULL: 
        // Die Kind-Operation wird vom Async_manager fortgesetzt. 
        // Wenn die Kind-Operation finished() ist, wird die Vater-Operation als fortsetzbar gekennzeichnet. Das passiert hier.

        _parent->async_wake();        // Mutter-Operation fortsetzen!
    }

    
    return something_done;
}

//-----------------------------------------------------------------Async_operation::async_has_error

bool Async_operation::async_has_error() const
{ 
    return _error || async_has_error_() || ( _child? _child->async_has_error() : false ); 
}

//---------------------------------------------------------------Async_operation::async_check_error

void Async_operation::async_check_error( const string& text, bool check_finished )
{
    if( _error )  
    {
        Xc x;
        x.set_name( _error_name );
        x.set_code( _error_code );
        x.set_what( _error_what );
        if( text != "" )  x.append_text( text );
        throw_xc( x );
    }

    async_check_error_();

    if( _child )  _child->async_check_error();

    if( check_finished  &&  !async_finished_() )  throw_xc( "Z-REMOTE-114", "async_check_error", async_state_text() );
}

//--------------------------------------------------------------------Async_operation::async_abort_

void Async_operation::async_abort_()
{
    if( _child )  
    {
        _child->async_abort();  
        
        _child->_parent = NULL; 
        _child = NULL;

        return;
    }
    
    Z_LOG( "Async_operation::async_abort_() ist nicht implementiert: " << async_state_text() << "\n" ); 
    return; 
}

//---------------------------------------------------------------------Async_operation::async_kill_

bool Async_operation::async_kill_()
{ 
    if( _child )  return _child->async_kill();  
    
    Z_LOG( "Async_operation::async_kill_() ist nicht implementiert: " << async_state_text() << "\n" ); 
    return false; 
}

//----------------------------------------------------------------Async_operation::async_state_text

string Async_operation::async_state_text() const
{
    string text = async_state_text_();

    //if( async_signaled() )  text += ",signaled!";
    if( _next_gmtime < double_time_max )
    {
        text += _next_gmtime == 0? ", now!" 
                                 : ", at " + string_gmt_from_time_t( (time_t)_next_gmtime ) + " UTC";
    }

    if( _error )  text += ",ERROR=", text += _error_what;
    if( _child )  text += "|" + _child->async_state_text();

    return text;
}

//--------------------------------------------------------Async_operation::set_async_warning_issued

bool Async_operation::set_async_warning_issued()
{
    bool             result = false;
    Async_operation* o      = this;

    while( o->_parent  &&  !o->_async_warning_issued )  o = o->_parent;

    result = o->_async_warning_issued;

    o->_async_warning_issued = true;
       _async_warning_issued = true;
                
    return result;
}

//---------------------------------------------------------------Async_manager::add_timed_operation

void Async_manager::add_operation( Async_operation* operation )
{
    if( operation->async_next_gmtime() == 0 )   // Sofort ausführen?
    {
        //_signaled_operations_queue.push_back( operation );
        _timed_operations_queue.push_front( operation );
        _timed_operations_queue_modified = true;
    }
    else
    if( operation->async_next_gmtime() < double_time_max )  // Timer
    {
        Operations_queue::iterator op = _timed_operations_queue.begin();
        
        while( op != _timed_operations_queue.end() )
        {
            if( (*op)->async_next_gmtime() > operation->async_next_gmtime() )  break;
            op++;
        }

        _timed_operations_queue.insert( op, operation );
        _timed_operations_queue_modified = true;
    }
    else
    {
        _timed_operations_queue.push_back( operation );
        _timed_operations_queue_modified = true;
    }
}

//------------------------------------------------------------------Async_manager::remove_operation

void Async_manager::remove_operation( Async_operation* operation )
{
/*
    if( operation->async_next_gmtime() == 0 )
    {
        Operations_queue::iterator it = _signaled_operations_queue.begin();

        while( it != _signaled_operations_queue.end() )
        {
            if( *it == operation )  { it = _signaled_operations_queue.erase( it );  break; }
                              else  it++;
        }
    }
    else
*/
    //if( operation->async_next_gmtime() < double_time_max )
    {
        Z_FOR_EACH( Operations_queue, _timed_operations_queue, op )
        {
            if( *op == operation ) 
            {
                _timed_operations_queue.erase( op );
                _timed_operations_queue_modified = true;
                break;
            }
        }
    }
}

//-----------------------------------------------------------Async_manager::async_continue_selected

bool Async_manager::async_continue_selected( Operation_is_ok* operation_is_ok )
{
    bool something_done = false;

    /*
    while( !_signaled_operations_queue.empty() )
    {
        Operations_queue::iterator it = _signaled_operations_queue.begin();
        ptr<Async_operation> op = *it;

        if( ( operation_is_ok == NULL  ||  (*operation_is_ok)( op ) )  &&  op->async_signaled() )
        {
            (*it)->_next_gmtime = double_time_max;
            it = _signaled_operations_queue.erase( it );

            something_done |= op->async_continue();
        }
    }
    */

    //Damit verhungern Operation am Ende der _timed_operations_queue:
    //int max_operations = 10;    // Damit auch der Scheduler mal rankommt (könnte als Parameter übergeben werden) Es gibt Operationen, die immer wieder neue aktivieren 2007-01-10
    //int operation_count = 0;

    if( !_timed_operations_queue.empty()  &&  (*_timed_operations_queue.begin())->async_next_gmtime() < double_time_max )
    {
        double now = 0;
        
        do
        {
            _timed_operations_queue_modified = false;

            Async_operation* dont_repeat_this_operation = NULL;

            for( Operations_queue::iterator it = _timed_operations_queue.begin(); it != _timed_operations_queue.end(); )
            {
                Async_operation* op = *it;
                if( op == dont_repeat_this_operation )  break;

                double next_gmtime = op->async_next_gmtime();
                if( next_gmtime > now )
                {
                    if( next_gmtime == double_time_max )  break;
                    if( !now )  now = double_from_gmtime();   // Zeit erst holen, wenn sie gebraucht wird
                    if( next_gmtime > now )  break;
                }

                if( operation_is_ok == NULL  ||  (*operation_is_ok)( op ) )
                {
                    //op->set_async_next_gmtime( double_time_max );     Stört unseren Iterator
                    op->_next_gmtime = double_time_max;
                    _timed_operations_queue.push_back( op );       // Ans Ende hängen, ohne unseren Iterator it über _timed_operations zu stören
                    it = _timed_operations_queue.erase( it );

                    if( !dont_repeat_this_operation )  dont_repeat_this_operation = op;

                    bool my_something_done = op->async_continue( Async_operation::cont_next_gmtime_reached );
                    something_done |= my_something_done;
                    //if( my_something_done  &&  operation_count++ == max_operations )  break;

                    if( _timed_operations_queue_modified )  break;      // Iterator ist ungültig?
                }
                else
                    it++;
            }
        }
        while( _timed_operations_queue_modified );  // Hmm, terminiert diese Schleife immmer? Was, wenn einer immer (redundant) async_next_gmtime setzt?
    }

    return something_done;
}

//-----------------------------------------------------------------Async_manager::async_next_gmtime

double Async_manager::async_next_gmtime()
{
    Async_operation* operation = async_next_operation();
    
    return operation? operation->async_next_gmtime() : 0;
}

//--------------------------------------------------------------Async_manager::async_next_operation

Async_operation* Async_manager::async_next_operation()
{
    return //!_signaled_operations_queue.empty()? *_signaled_operations_queue.begin() :
           !_timed_operations_queue   .empty()? *_timed_operations_queue.begin()
                                              : NULL;
}

//----------------------------------------------------------------------------Async_manager_printer

struct Async_manager_printer
{
    String_list                _result;
    vector<bool>               _printed_set;
};

//-----------------------------------------------------------Async_manager::string_from_operations2

void Async_manager::string_from_operations2( Async_manager_printer* p, int indent, Async_operation* parent )
{
    int i = 0;
    Z_FOR_EACH( Operations_queue, _timed_operations_queue, o )
    {
        Async_operation* op = *o;
        if( op->async_parent() == parent )
        {
            if( !p->_result.is_empty() )  p->_result.append( "\n" );
            p->_result.append( string( indent * 2, ' ' ) );
            p->_result.append( (*o)->async_state_text() );

            p->_printed_set[ i ] = true;

            string_from_operations2( p, indent + 1, op );
        }

        i++;
    }
}

//------------------------------------------------------------Async_manager::string_from_operations

string Async_manager::string_from_operations( const string& separator )
{
    Async_manager_printer p;

    if( separator == "\n" )
    {
        p._printed_set.resize( _timed_operations_queue.size() );

        string_from_operations2( &p, 0, NULL );

        int i = 0;
        Z_FOR_EACH( Operations_queue, _timed_operations_queue, o )
        {
            if( !p._printed_set[ i ] )
            {
                //Z_DEBUG_ONLY( assert( !"Async_manager::string_from_operations  unknown Async_operation::_parent" ) );
                if( !p._result.is_empty() )  p._result.append( separator );
                p._result.append( "Unknown parent: " );
                p._result.append( (*o)->async_state_text() );
            }

            i++;
        }
    }
    else
    {
        Z_FOR_EACH( Operations_queue, _timed_operations_queue, o )
        {
            if( !p._result.is_empty() )  p._result.append( separator );
            p._result.append( (*o)->async_state_text() );
        }
    }

    return p._result;
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
