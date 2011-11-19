// $Id: async_socket.h 14137 2010-11-25 15:10:51Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __Z_ASYNC_SOCKET_H
#define __Z_ASYNC_SOCKET_H

#include "async_event.h"
#include "z_sockets.h"
#include "z_io.h"
#include "async_io.h"

//-------------------------------------------------------------------------------------------------

namespace zschimmer { 

struct Socket_manager;

//--------------------------------------------------------------------------------------Socket_wait
#ifndef Z_WINDOWS

struct Socket_wait : Object
{
    enum Read_or_write
    { 
        readfd, 
        writefd,
        exceptfd
    };

                                Socket_wait                 ( Socket_manager* );
                              //Socket_wait                 ( Socket_manager*, int n, const fd_set& readfds, const fd_set& writefds, const fd_set& exceptfds );

  //void                        add                         ( int file_nr, Read_or_write, Has_set_signaled* );
    void                        add                         ( Event_base* );
    int                         wait                        ( double seconds );
    void                    set_polling_interval            ( double seconds )                      { _polling_interval = seconds; }

    Fill_zero                  _zero_;
    Socket_manager*            _socket_manager;
  //int                        _n;
  //fd_set                     _fds[3];
  //std::vector<Has_set_signaled*>  _signaled_array;

    typedef std::list<Event_base*>   Event_list;
    Event_list                 _event_list;
    double                     _polling_interval;
};

#endif

//---------------------------------------------------------------------------------Socket_operation

struct Socket_operation : Event_operation
{
    enum Signals
    {
        sig_none   = 0,
        sig_read   = 0x01,
        sig_write  = 0x02,
        sig_except = 0x04
    };


                                Socket_operation            ( SOCKET s = SOCKET_ERROR );// Socket_manager* = NULL );
                               ~Socket_operation            ();

    virtual void                close                       ();

    void                    set_close_socket_at_end         ( bool b )                              { _close_socket_at_end = b; }

    void                        add_to_socket_manager       ( Socket_manager* );
    void                        remove_from_socket_manager  ();
    void                    set_blocking                    ( bool b )                              { _blocking = b; }
  //void                        listen                      ();
    bool                        accept                      ( SOCKET listen_socket );

    void                        call_ioctl                  ( int what, unsigned long value );
    void                    set_keepalive                   ( bool );
    bool                try_set_keepalive                   ( bool );
    void                    set_linger                      ( bool, int seconds = 0 );

    virtual Socket_event*       async_event                 ()                                      { return &_socket_event; }
    void                    set_event_name                  ( const string& name )                  { _socket_event.set_name( name ); }
    void                        signal                      ( const string& name )                  { _socket_event.signal( name ); }

    void                        socket_expect_signals       ( Signals );

  //void                    set_socket_signals              ( Signals signals )                     { _socket_signals = signals; }
  //void                  clear_socket_signals              ()                                      { _socket_signals = sig_none; }

    bool                        async_signaled_             ();
    void                        async_clear_signaled_       ();
    string                      async_state_text_           () const;
/*
    bool                        socket_read_signaled        ()                                      { return ( _socket_signals & sig_read   ) != 0; }
    bool                        socket_write_signaled       ()                                      { return ( _socket_signals & sig_write  ) != 0; }
    bool                        socket_except_signaled      ()                                      { return ( _socket_signals & sig_except ) != 0; }
*/
  //string                      obj_name                    () const                                { return "TCP " + _peer_host_and_port.as_string(); }
    Host_and_port&              peer                        ()                                      { return _peer_host_and_port; }
    const Host_and_port&        peer                        () const                                { return _peer_host_and_port; }
    Host&                       peer_host                   ()                                      { return _peer_host_and_port.host(); }
    const Host&                 peer_host                   () const                                { return _peer_host_and_port.host(); }
    bool                        eof                         () const                                { return _eof; }

//protected:
 // friend struct Socket_manager;
    void                        set_socket_event_name       ( const string& state );

    Fill_zero _zero_;

    SOCKET                     _read_socket;
    SOCKET                     _write_socket;               // _read_socket != _write_socket, nur wenn _read_socket==stdin und _write_socket==stdout
    bool                       _close_socket_at_end;
    bool                       _eof;                        // recv(_read_socket) hat 0 geliefert, also die Verbindung geschlossen
  //Signals                    _socket_signals;
    Socket_manager*            _socket_manager;
    Socket_event               _socket_event;               // In Unix nur bei Http_response/Log_chunk_reader

  //struct sockaddr_in         _peer_addr;
    Host_and_port              _peer_host_and_port;
    bool                       _blocking;

#   ifdef Z_WINDOWS
        int                    _wsa_event_select_flags;
#   endif
};

inline Socket_operation::Signals  operator |  ( Socket_operation::Signals  a, Socket_operation::Signals b )  { return (Socket_operation::Signals)( (int)a | (int)b ); }
inline Socket_operation::Signals& operator |= ( Socket_operation::Signals& a, Socket_operation::Signals b )  { a = a | b; return a; }

//------------------------------------------------------------------------Buffered_socket_operation

struct Buffered_socket_operation : Socket_operation
{
    enum State 
    { 
        s_initial, 
        s_connecting,
        s_ready,                // connect(), recv() oder send() fertig
        s_sending,              // Daten sind zu senden
        s_receiving             // Daten sollen empfangen werden, recv() hat Z_EWOULDBLOCK geliefert
    };


                                Buffered_socket_operation   ( SOCKET s = SOCKET_ERROR );

    void                        close                       ();

    void                        connect__start              ( const Host_and_port& );
    bool                        connect__continue           ();

    void                        set_buffer_size             ();
    bool                        send__start                 ( const string& data );
    bool                        send__continue              ();

    bool                        is_received_data_ready      ();

    int                         call_recv                   ( Byte* buffer, int size )              { return call_recv( (char*)buffer, size ); }
    int                         call_recv                   ( char* buffer, int size );
  //void                        end_receiving               ();
    bool                        recv__start                 ();
    bool                        recv__continue              ();
    const string&               recv_data                   ()                                      { return _recv_data; }
    void                        recv_clear                  ()                                      { _recv_data = ""; }
    void                        assert_no_recv_data         ();
    bool                        check_for_eof               ();

    int64                       recv_total_byte_count       () const                                { return _recv_total_byte_count; }
    int64                       send_total_byte_count       () const                                { return _send_total_byte_count; }
    int64                       response_count              () const                                { return _response_count; }

    void                    set_state                       ( State state )                         { _state = state; }
    State                       state                       () const                                { return _state; }
    string                      state_name                  () const                                { return state_name( _state ); }
    static string               state_name                  ( State );

  protected:
    bool                        async_continue_             ( Continue_flags );
    bool                        async_has_error_            () const                                { return false; }
    bool                        async_finished_             () const                                { return _state == s_initial  
                                                                                                          || _state == s_ready; }
    string                      async_state_text_           () const;


  private:
    Fill_zero                  _zero_;
    State                      _state;
    int                        _socket_send_buffer_size;
    string                     _send_data;
    size_t                     _send_progress;
    string                     _recv_data;
    int64                      _recv_total_byte_count;
    int64                      _send_total_byte_count;
    int64                      _response_count;
};

//-----------------------------------------------------------------------------------Socket_manager

struct Socket_manager : Event_manager
{
    enum Read_or_write
    { 
        read_fd, 
        write_fd,
        except_fd
    };


    struct Used_ports
    {
        int                     bind_free_port              ( SOCKET, sockaddr_in* );
        void                    register_port               ( int port )                            { _used_ports.insert( port ); }
        void                    unregister_port             ( int port )                            { _used_ports.erase( port ); }

      private:
        stdext::hash_set<int>  _used_ports;
    };



                                Socket_manager              ();
                               ~Socket_manager              ();


    //void                        get_events                  ( std::vector<Socket_event*>* );


    // Event_manager
    string                      string_from_operations  ( const string& separator = ", " );


#ifdef Z_WINDOWS
    void                        set_fd                      ( Read_or_write, SOCKET )               {}
    void                        clear_fd                    ( Read_or_write, SOCKET )               {}
    void                        clear_socket_signaled       ( SOCKET )                              {}
#else
    void                        set_fd                      ( Read_or_write, SOCKET );
    void                        clear_fd                    ( Read_or_write, SOCKET );
    ptr<Socket_wait>            create_wait                 ();
    bool                        socket_signaled             ( SOCKET );
    void                        clear_socket_signaled       ( SOCKET );
#endif

    int                         wait                        ( double timeout_seconds );

    void                        set_read_fd                 ( SOCKET s )                            { set_fd( read_fd, s ); }
    void                        set_write_fd                ( SOCKET s )                            { set_fd( write_fd, s ); }
    void                        set_except_fd               ( SOCKET s )                            { set_fd( except_fd, s ); }

    //bool                        async_continue              ()                                      { return async_continue_selected( NULL ); }
    //bool                        async_continue_selected     ( Operation_is_ok f, double sleep = 0.0 );


  protected:
    friend struct               Socket_operation;

    // Benutze Socket_operation::add_to_socket_manager()!
    void                        add_socket_operation        ( Socket_operation* );
    void                        remove_socket_operation     ( Socket_operation* );

    //bool                        async_continue_selected_    ( Operation_is_ok* );


    Fill_zero                  _zero_;

    //typedef std::list< Socket_operation* >  Socket_operation_list;
    //Socket_operation_list      _socket_operation_list;
    //bool                       _socket_operation_list_modified;

#   ifndef Z_WINDOWS
        SOCKET                 _n;
        fd_set                 _fds[3];
        
        SOCKET                 _socket_signaled_count;
        fd_set                 _signaled_fds[3];
#   else
        bool                   _wsastartup_called;
#   endif

  public:
    Used_ports                 _used_tcp_ports;
};

//------------------------------------------------------------------------------------Socket_stream

struct Socket_stream : Buffered_socket_operation, io::Input_stream, io::Output_stream
{
    static const size_t         read_bytes_maximum;


    STDMETHODIMP_( ULONG )      AddRef                      ()                                      { return Buffered_socket_operation::AddRef(); }
    STDMETHODIMP_( ULONG )      Release                     ()                                      { return Buffered_socket_operation::Release(); }
    STDMETHODIMP                QueryInterface              ( const IID& iid, void** result )       { return Buffered_socket_operation::QueryInterface( iid, result ); }
    void                        close                       ()                                      { Buffered_socket_operation::close(); }

    void                        connect_tcp                 ( const string& address );
    void                        write_bytes                 ( const io::Byte_sequence& );
    string                      read_bytes                  ( size_t maximum = read_bytes_maximum );
    void                        flush                       ();
};

//-----------------------------------------------------------------------Socket_async_output_stream

struct Socket_async_output_stream : Async_operation, io::Async_output_stream
{
                                Socket_async_output_stream  ( Buffered_socket_operation* b )        : _buffered_socket_operation( b ) {}

    int                         try_write_bytes             ( const io::Byte_sequence& );
    bool                        try_flush                   ()                                      { return true; }
    void                        request_notification        ();
    void                        close                       ();

  private:
    ptr<Buffered_socket_operation> _buffered_socket_operation;
};

//------------------------------------------------------------------------Socket_async_input_stream

struct Socket_async_input_stream : Async_operation, io::Async_input_stream
{
    string                      try_read_bytes              ( int count );
    bool                        eof                         ();
    void                        close                       ()                                      {}

  private:
    ptr<Buffered_socket_operation> _buffered_socket_operation;
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
