// $Id: pipe_collector.cxx 13691 2008-09-30 20:42:20Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "zschimmer.h"
#include "pipe_collector.h"
#include "log.h"
#include "z_io.h"
#include "async.h"
#include "async_socket.h"

#ifdef Z_WINDOWS

namespace zschimmer
{

//-----------------------------------------------Pipe_collector::Collector_thread::Collector_thread
    
Pipe_collector::Collect_operation::Collect_operation( Pipe_collector* pipe_collector, SOCKET socket, Handler* handler )
:
    _zero_(this+1),
    _handler(handler),
    _buffered_socket_operation( Z_NEW( Buffered_socket_operation( socket ) ) ),
    _pipe_collector(pipe_collector)
{
    _buffered_socket_operation->set_close_socket_at_end( true );
    _buffered_socket_operation->call_ioctl( FIONBIO, 1 );   // Non blocking
    _buffered_socket_operation->set_async_parent( this );
}

//---------------------------------------------ipe_collector::Collect_operation::~Collect_operation
    
Pipe_collector::Collect_operation::~Collect_operation()
{
    if( _buffered_socket_operation )  
    {
        _buffered_socket_operation->set_async_parent( NULL );
        _buffered_socket_operation->set_async_manager( NULL );
    }
}

//-----------------------------------------------Pipe_collector::Collect_operation::async_continue_
    
bool Pipe_collector::Collect_operation::async_continue_( Continue_flags )
{
    bool something_done = false;

    switch( _state )
    {
        case s_initial:
        {
            _buffered_socket_operation->add_to_socket_manager( _pipe_collector->_socket_manager );
            _last_time = ::time(NULL);    
            _state = s_receiving;
            //break;
        }

        case s_receiving:
        {
            time_t now = ::time(NULL);
            char buffer [ 100000 ];

            int length = _buffered_socket_operation->call_recv( buffer, sizeof buffer );
            
            if( _buffered_socket_operation->eof() )
            {
                _buffered_socket_operation->close();
                _state = s_eof;
                something_done = true;
            }
            else
            {
                _string_list.append( io::Char_sequence( buffer, length ) );

                const char* last_nl = (const char*)memrchr( buffer, '\n', length );

                if( _string_list.length() >= _pipe_collector->_buffer_size 
                 || last_nl
                 || _last_time + _pipe_collector->_timeout <= now )
                {
                    size_t length = last_nl? last_nl - buffer + 1 : _string_list.length();
                    _handler->on_thread_has_received_data( _string_list.string_eat( length ) );
                }

                if( length > 0 )  _last_time = now;

                something_done = true;
            }

            break;
        }

        case s_eof:
        {
            break;
        }
    }

    return something_done;
}

//---------------------------------------------Pipe_collector::Collect_operation::async_state_text_

string Pipe_collector::Collect_operation::async_state_text_() const
{
    return "Pipe_collector::Collect_operation";
}

//-----------------------------------------------Pipe_collector::Collector_thread::Collector_thread

Pipe_collector::Collector_thread::Collector_thread( Pipe_collector* p )
: 
    _zero_(this+1), 
    _pipe_collector(p) 
{
}

//----------------------------------------------------Pipe_collector::Collector_thread::thread_main
    
int Pipe_collector::Collector_thread::thread_main()
{
    Z_LOG2( "zschimmer", Z_FUNCTION << "\n" );

    while( !_pipe_collector->_socket_manager->is_empty() )
    {
        double wait_time = _pipe_collector->_socket_manager->async_next_gmtime() - double_from_gmtime();
        Z_LOG2( "zschimmer", Z_FUNCTION << "  wait " << wait_time << "s\n" );
        //_pipe_collector->_socket_manager->wait( wait_time );
        _pipe_collector->_socket_manager->async_continue_selected( NULL, wait_time );
    }

    return 0;
}

//-------------------------------------------------------------------Pipe_collector::Pipe_collector
    
Pipe_collector::Pipe_collector()
:
    _zero_(this+1)
{
    _buffer_size = 20000;
    _timeout = 1;
    _socket_manager = Z_NEW( Socket_manager );
}

//------------------------------------------------------------------Pipe_collector::~Pipe_collector
    
Pipe_collector::~Pipe_collector()
{
    stop();
}

//----------------------------------------------------------------------------Pipe_collector::start
    
void Pipe_collector::start()
{
    _thread = Z_NEW( Collector_thread( this ) );
    _thread->thread_start();
}

//-----------------------------------------------------------------------------Pipe_collector::stop

void Pipe_collector::stop()
{
    _collect_operation_map.clear();

    if( _thread )
    {
        //_thread->_end = true;
        _thread->thread_wait_for_termination();
    }
}

//-------------------------------------------------------------------------Pipe_collector::add_pipe

SOCKET Pipe_collector::add_pipe( Handler* handler )
{
    if( _thread )  throw_xc( Z_FUNCTION );


    SOCKET socket_pair[ 2 ] = { -1, -1 };
    
    int error = z_socketpair( PF_UNIX, SOCK_STREAM, 0, socket_pair );
    if( error )  throw_errno( errno, "z_socketpair" );

    ptr<Collect_operation> collect_operation = Z_NEW( Collect_operation( this, socket_pair[1], handler ) );

    _collect_operation_map[ socket_pair[ 0 ] ] = collect_operation;
    collect_operation->async_wake();        // Starten
    collect_operation->set_async_manager( _socket_manager );

    return socket_pair[ 0 ];
}

//-------------------------------------------------Stdout_stderr_collector::Stdout_stderr_collector

Stdout_stderr_collector::Stdout_stderr_collector()
:
    _zero_(this+1),
    _stdout_socket( SOCKET_ERROR ),
    _stderr_socket( SOCKET_ERROR ),
    _previous_stdout( SOCKET_ERROR ),
    _previous_stderr( SOCKET_ERROR )
{
}

//------------------------------------------------Stdout_stderr_collector::~Stdout_stderr_collector

Stdout_stderr_collector::~Stdout_stderr_collector()
{
    if( _stdout_socket != SOCKET_ERROR ) 
    {
        closesocket( _stdout_socket );
        SetStdHandle( STD_OUTPUT_HANDLE, (HANDLE)_previous_stdout );
    }

    if( _stderr_socket != SOCKET_ERROR )  
    {
        closesocket( _stderr_socket );
        SetStdHandle( STD_ERROR_HANDLE, (HANDLE)_previous_stderr );
    }
}

//------------------------------------------------------Stdout_stderr_collector::set_stdout_handler

void Stdout_stderr_collector::set_stdout_handler( Handler* handler )
{
    _stdout_socket = add_pipe( handler );

#   ifdef Z_WINDOWS
        BOOL ok;

        _previous_stdout = (SOCKET)GetStdHandle( STD_OUTPUT_HANDLE );

        ok = SetStdHandle( STD_OUTPUT_HANDLE, (HANDLE)_stdout_socket );
        if( !ok )  throw_mswin( "SetStdHandle", "stdout" );

        ok = SetHandleInformation( (HANDLE)_stdout_socket, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT );
        if( !ok )  throw_mswin( "SetHandleInformation" );

#    else

        dup2(...);

#   endif
}

//------------------------------------------------------Stdout_stderr_collector::set_stderr_handler

void Stdout_stderr_collector::set_stderr_handler( Handler* handler )
{
    _stderr_socket = add_pipe( handler );

#   ifdef Z_WINDOWS
        BOOL ok;

        _previous_stderr = (SOCKET)GetStdHandle( STD_ERROR_HANDLE );

        ok = SetStdHandle( STD_ERROR_HANDLE, (HANDLE)_stderr_socket );
        if( !ok )  throw_mswin( "SetStdHandle", "stderr");

        ok = SetHandleInformation( (HANDLE)_stderr_socket, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT );
        if( !ok )  throw_mswin( "SetHandleInformation" );

#    else

        dup2(...);

#   endif
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
