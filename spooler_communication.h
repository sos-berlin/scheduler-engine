// $Id$

#ifndef __SPOOLER_COMMUNICATION_H
#define __SPOOLER_COMMUNICATION_H

#include "../zschimmer/z_sockets.h"
#include "../zschimmer/xml_end_finder.h"

/*
#ifdef __GNUC__
#   include <errno.h>
#   include <sys/socket.h>

#   ifndef SYSTEM_HPUX
#       include <sys/select.h>
#   endif

#   include <netdb.h>
#   include <netinet/in.h>   // gethostbyname()
#   include <arpa/inet.h>    // inet_addr()
#   include <unistd.h>       // close()

    typedef int SOCKET;
    const SOCKET SOCKET_ERROR = (SOCKET)-1;
#endif
*/

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

struct Xml_processor_channel;

//inline bool operator < ( const in_addr& a, const in_addr& b )  { return a.s_addr < b.s_addr; }  // Für map<>

//------------------------------------------------------------------------------------Communication

struct Communication
{       
    struct Processor_channel;
    struct Processor;


    struct Channel : zschimmer::Buffered_socket_operation
    {
                                Channel                     ( Communication* );
                               ~Channel                     ();

        void                    remove_me                   ( const exception* = NULL );
        void                    terminate                   ();

        bool                    do_accept                   ( SOCKET listen_socket );
        void                    do_close                    ();
        bool                    do_recv                     ();
        bool                    do_send                     ();

        void                    recv_clear                  ();

        virtual bool            async_continue_             ( bool wait );
        virtual bool            async_finished_             ()                                      { return false; }
        virtual string          async_state_text_           ()                                      { return "Spooler::Communication::Channel()"; }  // \"" + _named_host + "\"


        Fill_zero              _zero_;
        Spooler*               _spooler;
        Communication*         _communication;
      //Host                   _host;

        bool                   _responding;
        bool                   _receive_at_start;

        int                    _socket_send_buffer_size;
      //string                 _send_data;
      //int                    _send_progress;
        bool                   _dont_receive;               // Bei terminate() ist Empfang gesperrt
        Prefix_log             _log;

        ptr<Processor_channel> _processor_channel;
        ptr<Processor>         _processor;
    };


    struct Listen_socket : Socket_operation
    {
                                Listen_socket               ( Communication* c )                    : _communication(c), _spooler(c->_spooler) {}

        virtual bool            async_continue_             ( bool wait );
        virtual bool            async_finished_             ()                                      { return false; }
        virtual string          async_state_text_           ()                                      { return "Spooler::Communication::Listen_socket()"; }

        Spooler*               _spooler;
        Communication*         _communication;
    };


    struct Udp_socket : Socket_operation
    {
                                Udp_socket                  ( Communication* c )                    : _communication(c), _spooler(c->_spooler) {}

        virtual bool            async_continue_             ( bool wait );
        virtual bool            async_finished_             ()                                      { return false; }
        virtual string          async_state_text_           ()                                      { return "Spooler::Communication::Udp_socket()"; }

        Spooler*               _spooler;
        Communication*         _communication;
    };


    struct Processor : Object
    {
                                Processor                   ( Processor_channel* pc )               : _zero_(this+1), _channel(pc->_channel), _spooler(pc->_spooler), _processor_channel(pc) {}


        void                    set_host                    ( Host* host )                          { _host = host; }

        virtual void            put_request_part            ( const char*, int length )             = 0;
        virtual bool            request_is_complete         ()                                      = 0;

        virtual void            process                     ()                                      = 0;

        virtual bool            response_is_complete        ()                                      = 0;
        virtual string          get_response_part           ()                                      = 0;
        virtual bool            should_close_connection     ()                                      { return false; }


        Fill_zero              _zero_;
        Spooler*               _spooler;
        Channel*               _channel;
        Processor_channel*     _processor_channel;
        Host*                  _host;
    };



    struct Processor_channel : Object
    {
                                Processor_channel           ( Channel* ch )                         : _spooler(ch->_spooler), _channel(ch) {}


        virtual ptr<Processor>  processor                   ()                                      = 0;
        virtual void            connection_lost_event       ( const exception* )                    {}



        Spooler*               _spooler;
        Channel*               _channel;
    };


    /*  
        Ablauf:


        accept();
        ptr<Processor_channel> processor_channel;

        while(1)
        {
            ptr<Processor> processor = processor_channel->processor();

            while(1)
            {
                recv( &data, length );
                processor->put_request_part( data, length );
                if( processor->request_is_complete() )  break;
            }

            processor->process();

            while( !processor->response_is_complete() )  send( processor->get_response_part() );

            if( processor->should_close_connection() )  break;
        }
    */



    typedef list< ptr<Channel> >  Channel_list;


                                Communication               ( Spooler* );
                               ~Communication               ();

    void                        init                        ();
    void                        start_or_rebind             ();
  //void                        start_thread                ();
    void                        close                       ( double wait_time = 0.0 );
    void                        bind                        ();
    void                        rebind                      ()                                      { bind(); }
  //int                         thread_main                 ();
    bool                        started                     ()                                      { return _started; }
  //bool                        main_thread_exists          ();
    void                        remove_channel              ( Channel* );

  private:
  //int                         run                         ();
  //bool                        handle_socket               ( Channel* );
    int                         bind_socket                 ( SOCKET, struct sockaddr_in* );
  //void                       _fd_set                      ( SOCKET, fd_set* );

  public:
    Fill_zero                  _zero_;
    Spooler*                   _spooler;

  private:
    Listen_socket              _listen_socket;
    Udp_socket                 _udp_socket;
    Channel_list               _channel_list;
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

//------------------------------------------------------------------------------------Xml_processor

struct Xml_processor : Communication::Processor
{
                                Xml_processor               ( Xml_processor_channel* );

    void                        put_request_part            ( const char*, int length );
    bool                        request_is_complete         ()                                      { return _request_is_complete; }

    void                        process                     ();

    bool                        response_is_complete        ()                                      { return true; }
    string                      get_response_part           ()                                      { string result = _response;  _response = "";  return result; }

    Fill_zero                  _zero_;
    Xml_processor_channel*     _processor_channel;
    bool                       _request_is_complete;
    Xml_end_finder             _xml_end_finder;
    string                     _request;
    string                     _response;
};

//------------------------------------------------------------------------------Xml_processor_channel

struct Xml_processor_channel : Communication::Processor_channel
{
                                Xml_processor_channel       ( Communication::Channel* );


    ptr<Communication::Processor> processor                 ()                                      { ptr<Xml_processor> result = Z_NEW( Xml_processor( this ) ); 
                                                                                                      return +result; }

    virtual void                connection_lost_event       ( const exception* );


    bool                       _indent;
    ptr<Remote_scheduler>      _remote_scheduler;
};

//-------------------------------------------------------------------------------------------------

inline bool is_communication_operation( Async_operation* op )
{
    // Diese Socket_operations werden in spooler_history.cxx fortgesetzt, wenn auf die DB gewartet wird.

    return dynamic_cast<Communication::Listen_socket*>( op ) ||
           dynamic_cast<Communication::Udp_socket*   >( op ) ||
           dynamic_cast<Communication::Channel*      >( op );
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
