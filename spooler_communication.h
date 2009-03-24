// $Id$

#ifndef __SPOOLER_COMMUNICATION_H
#define __SPOOLER_COMMUNICATION_H

#include "../zschimmer/z_sockets.h"
#include "../zschimmer/xml_end_finder.h"


namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------------------

struct Xml_operation_connection;

//inline bool operator < ( const in_addr& a, const in_addr& b )  { return a.s_addr < b.s_addr; }  // Für map<>

//------------------------------------------------------------------------------------Communication

struct Communication
{       
    struct Operation_connection;
    struct Operation;


    struct Connection : zschimmer::Buffered_socket_operation
    {
        enum Connection_state
        {
            s_none,
            s_ready,
            s_receiving,
            s_processing,
            s_responding,
            s_closing
        };


                                Connection                  ( Communication* );
                               ~Connection                  ();

        xml::Element_ptr        dom_element                 ( const xml::Document_ptr&, const Show_what& ) const;

        Connection_state        connection_state            () const                                { return _connection_state; }
        string                  connection_state_name       () const                                { return connection_state_name( _connection_state ); }
        static string           connection_state_name       ( Connection_state );

        void                    remove_me                   ( const exception* = NULL );
        void                    terminate                   ( double wait_time );
        void                    remove_operation            ();

        bool                    do_accept                   ( SOCKET listen_socket );
        void                    do_close                    ();
        bool                    do_recv                     ();
        bool                    do_send                     ();

        virtual bool            async_continue_             ( Continue_flags );
        virtual bool            async_finished_             () const                                { return false; }
        virtual string          async_state_text_           () const                                { return "connection " + zschimmer::Buffered_socket_operation::async_state_text_(); }


        Fill_zero              _zero_;
        Connection_state       _connection_state;
        Spooler*               _spooler;
        Communication*         _communication;
        Security::Level        _security_level;

        int                    _socket_send_buffer_size;
        Prefix_log             _log;

        ptr<Operation_connection> _operation_connection;
        ptr<Operation>         _operation;
    };


    struct Listen_socket : Old_socket_operation
    {
                                Listen_socket               ( Communication* c )                    : _communication(c), _spooler(c->_spooler) {}

        virtual bool            async_continue_             ( Continue_flags );
        virtual bool            async_finished_             () const                                { return false; }
        virtual string          async_state_text_           () const                                { return "TCP listen " + Old_socket_operation::async_state_text_();  }

        Spooler*               _spooler;
        Communication*         _communication;
    };


    struct Udp_socket : Old_socket_operation
    {
                                Udp_socket                  ( Communication* c )                    : _communication(c), _spooler(c->_spooler) {}

        virtual bool            async_continue_             ( Continue_flags );
        virtual bool            async_finished_             () const                                { return false; }
        virtual string          async_state_text_           () const                                { return "UDP " + Old_socket_operation::async_state_text_();  }

        Spooler*               _spooler;
        Communication*         _communication;
    };


    
    struct Operation : Async_operation
    {
                                Operation                   ( Operation_connection* pc )            : _zero_(this+1), _connection(pc->_connection), _spooler(pc->_spooler), _operation_connection(pc) {}


      //void                    set_host                    ( Host* host )                          { _host = host; }
        void                set_gmtimeout                   ( double gmtime )                       { set_async_next_gmtime( gmtime ); }
        double                  gmtimeout                   () const                                { return async_next_gmtime(); }

        virtual void            close                       ()                                      { _connection = NULL, _operation_connection = NULL; }
        virtual void            put_request_part            ( const char*, int length )             = 0;
        virtual bool            request_is_complete         ()                                      = 0;

      //virtual void            process                     ()                                      = 0;
        virtual void            begin                       ()                                      = 0;

        virtual bool            response_is_complete        ()                                      = 0;
        virtual string          get_response_part           ()                                      = 0;
        virtual bool            should_close_connection     ()                                      { return false; }

        virtual bool            async_continue_             ( Continue_flags )                      = 0;
        virtual bool            async_finished_             () const                                { return true; }
        virtual string          async_state_text_           () const;

        virtual xml::Element_ptr dom_element                ( const xml::Document_ptr&, const Show_what& ) const = 0;

        Connection*             connection                  () const                                { return _connection; }
        Operation_connection*   operation_connection        () const                                { return _operation_connection; }


        Fill_zero              _zero_;
        Spooler*               _spooler;
        Connection*            _connection;
        Operation_connection*  _operation_connection;
    };



    struct Operation_connection : Object
    {
                                Operation_connection        ( Connection* ch )                      : _spooler(ch->_spooler), _connection(ch) {}


        virtual ptr<Operation>  new_operation               ()                                      = 0;
        virtual void            connection_lost_event       ( const exception* )                    {}
        virtual string          connection_type             () const                                = 0;

        void                    close                       ();
        void                    register_task_process       ( Process* );
        void                    unregister_task_process     ( Process_id process_id );
        Process*                get_task_process            ( Process_id process_id );




        Spooler*               _spooler;
        Connection*            _connection;

        typedef map< Process_id, ptr<Process> >  Task_process_register;
        Task_process_register      _task_process_register;
    };


    /*  
        Ablauf:


        accept();
        ptr<Operation_connection> processor_connection;

        while(1)
        {
            ptr<Operation> operation = processor_connection->operation();

            while(1)
            {
                recv( &data, length );
                operation->put_request_part( data, length );
                if( operation->request_is_complete() )  break;
            }

            operation->process();

            while( !operation->response_is_complete() )  send( operation->get_response_part() );

            if( operation->should_close_connection() )  break;
        }
    */



    typedef list< ptr<Connection> >  Connection_list;


                                Communication               ( Spooler* );
                               ~Communication               ();

    void                        init                        ();
    void                        start_or_rebind             ();
  //void                        start_thread                ();
    void                        close                       ( double wait_time );
    void                        finish_responses            ( double wait_time );                   // Antworten versenden (synchron)
    void                        bind                        ();
    void                        rebind                      ()                                      { bind(); }
  //int                         thread_main                 ();
    bool                        started                     ()                                      { return _started; }
  //bool                        main_thread_exists          ();
    void                        remove_connection           ( Connection* );

    xml::Element_ptr            dom_element                 ( const xml::Document_ptr& document, const Show_what& ) const;

  private:
  //int                         run                         ();
  //bool                        handle_socket               ( Connection* );
    int                         bind_socket                 ( SOCKET, struct sockaddr_in*, const string& tcp_or_udp );
  //void                       _fd_set                      ( SOCKET, fd_set* );

  public:
    Fill_zero                  _zero_;
    Spooler*                   _spooler;

  private:
    Listen_socket              _listen_socket;
    Udp_socket                 _udp_socket;
    Connection_list            _connection_list;
  //int                        _nfds;
  //fd_set                     _read_fds;
  //fd_set                     _write_fds;
  //Thread_semaphore           _semaphore;
    bool                       _terminate;
    int                        _tcp_port;
    int                        _udp_port;
    bool                       _rebound;
    int                        _initialized;
    bool                       _started;
};

//-------------------------------------------------------------------------------------------------

struct Xml_response : Async_operation, io::Writer
{
                                Xml_response                ()                                      : _zero_(this+1), _xml_writer(this) {}  // close() aufrufen!

    // Writer : IUnknown
    STDMETHODIMP                QueryInterface              ( const IID& iid, void** o )            { return Async_operation::QueryInterface( iid, o ); }
    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Async_operation::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Async_operation::Release(); }

    virtual void                close                       ();                                     // Unbedingt rufen, um Zirkel aufzulösen!
    virtual void                write                       ( const io::Char_sequence& )            = 0;
    virtual string              get_part                    ()                                      = 0;

    void                    set_connection                  ( Communication::Connection* c )        { _connection = c; }
    void                        signal_new_data             ();


  protected:
    Fill_zero                  _zero_;
    xml::Xml_writer            _xml_writer;

  private:
    Communication::Connection* _connection;
};
    
//------------------------------------------------------------------------------------Xml_operation

struct Xml_operation : Communication::Operation
{
                                Xml_operation               ( Xml_operation_connection* );
                               ~Xml_operation               ();


    // Async_operation
    virtual bool                async_continue_             ( Continue_flags )                      { return _response? _response->async_continue() : false; }


    virtual void                close                       ();
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& ) const;

    void                        put_request_part            ( const char*, int length );
    bool                        request_is_complete         ()                                      { return _request_is_complete; }

    void                        begin                       ();

    bool                        response_is_complete        ()                                      { return _response->async_finished(); }
    string                      get_response_part           ()                                      { return _response->get_part(); }

    virtual string              async_state_text_           () const;

    Fill_zero                  _zero_;
    Xml_operation_connection*  _operation_connection;
    bool                       _request_is_complete;
    Xml_end_finder             _xml_end_finder;
    string                     _request;
    ptr<Xml_response>          _response;
};

//-------------------------------------------------------------------------Xml_operation_connection

struct Xml_operation_connection : Communication::Operation_connection
{
                                Xml_operation_connection    ( Communication::Connection* );


    ptr<Communication::Operation> new_operation             ()                                      { ptr<Xml_operation> result = Z_NEW( Xml_operation( this ) ); 
                                                                                                      return +result; }

    virtual void                connection_lost_event       ( const exception* );
    string                      connection_type             () const                                { return "TCP"; }


    Fill_zero                  _zero_;
    string                     _indent_string;
    ptr<supervisor::Remote_scheduler_interface>  _remote_scheduler;
};

//-------------------------------------------------------------------------------------------------

inline bool is_communication_operation( Async_operation* op )
{
    // Diese Socket_operations werden in database.cxx fortgesetzt, wenn auf die DB gewartet wird.

    return dynamic_cast<Communication::Listen_socket*>( op ) ||
           dynamic_cast<Communication::Udp_socket*   >( op ) ||
           dynamic_cast<Communication::Connection*   >( op );
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
