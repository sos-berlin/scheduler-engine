// $Id$

namespace sos {
namespace spooler {

//---------------------------------------------------------------------------------Object_processor

struct Object_processor : Communication::Processor
{
                                Object_processor            ( Object_processor_channel* );


    void                        put_request_part            ( const char* data, int length );
    bool                        request_is_complete         ();

    void                        process                     ();

    bool                        response_is_complete        ();
    string                      get_response_part           ();
    bool                        should_close_connection     ();


    Fill_zero                  _zero_;
    Object_processor_channel*  _processor_channel;
    string                     _buffer;
};

//---------------------------------------------------------------------------Object_processor_channel

struct Object_processor_channel : Communication::Processor_channel
{
                                Object_processor_channel    ( Communication::Channel* ch )          : Communication::Processor_channel( ch ) {}

    ptr<Communication::Processor> processor                 ()                                      { ptr<Object_processor> result = Z_NEW( Object_processor( this ) ); 
                                                                                                      return +result; }
};

//-----------------------------------------------------------------------------------Remote_scheduler

struct Remote_scheduler
{
    Host                       _host;
    int                        _tcp_port;
    string                     _scheduler_id;
    string                     _version;
    Time                       _connected_at;
    Time                       _disconnected_at;
    bool                       _logged_run;
    bool                       _connection_lost;

    ptr<object_server::Proxy>  _scheduler_proxy;
};

//-------------------------------------------------------------------------------------------------

struct Remote_scheduler_register : Async_operation
{

    virtual bool                async_continue_         ( bool wait );
    virtual bool                async_finished_         ()                                          { return false; }
    virtual string              async_state_text_       ();


    ptr<object_server::Server> _server;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
