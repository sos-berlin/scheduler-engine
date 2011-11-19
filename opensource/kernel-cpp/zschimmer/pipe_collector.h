// $Id: pipe_collector.h 13199 2007-12-06 14:15:42Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __ZSCHIMMER_PIPE_COLLECTOR_
#define __ZSCHIMMER_PIPE_COLLECTOR_

#include "async_socket.h"
#include "threads.h"

namespace zschimmer
{

//-------------------------------------------------------------------------------------Pipe_collector

struct Pipe_collector : Object
{
    struct Collector_thread;


    struct Handler
    {
        virtual                ~Handler                     ()                                      {}
        virtual void            on_thread_has_received_data ( const string& )                       = 0;
    };


    struct Collect_operation : Async_operation
    {
        enum State { s_initial, s_receiving, s_eof };


                                Collect_operation           ( Pipe_collector*, SOCKET, Handler* );
                               ~Collect_operation           ();


        bool                    async_continue_             ( Continue_flags );
        bool                    async_finished_             () const                                { return _state == s_eof; }
        string                  async_state_text_           () const;

        Fill_zero              _zero_;
        State                  _state;
        String_list            _string_list;
        time_t                 _last_time;
        ptr<Buffered_socket_operation> _buffered_socket_operation;
        Handler*               _handler;
        Pipe_collector*        _pipe_collector;
    };


    struct Collector_thread : Thread
    {
                                Collector_thread              ( Pipe_collector* );

        int                     thread_main                 ();


        Fill_zero              _zero_;
        bool                   _end;
        std::list<Buffered_socket_operation> _socket_list;
        Pipe_collector*          _pipe_collector;
    };


                                Pipe_collector              ();
                               ~Pipe_collector              ();


    SOCKET                      add_pipe                    ( Handler* );

    void                        start                       ();
    void                        stop                        ();


  private:
    Fill_zero                  _zero_;
    size_t                     _buffer_size;
    int                        _timeout;
    ptr<Collector_thread>      _thread;
    ptr<Socket_manager>        _socket_manager;

    typedef stdext::hash_map< SOCKET, ptr<Collect_operation> >  Collect_operation_map;
    Collect_operation_map      _collect_operation_map;
};

//--------------------------------------------------------------------------Stdout_stderr_collector

struct Stdout_stderr_collector : Pipe_collector
{
                                Stdout_stderr_collector     ();
                               ~Stdout_stderr_collector     ();

    void                        set_stdout_handler          ( Handler* );
    void                        set_stderr_handler          ( Handler* );

  private:
    Fill_zero                  _zero_;
    SOCKET                     _stdout_socket;
    SOCKET                     _stderr_socket;
    SOCKET                     _previous_stdout;
    SOCKET                     _previous_stderr;                     
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
