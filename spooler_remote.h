// $Id$

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

struct Object_server_processor_channel;

//--------------------------------------------------------------------------Object_server_processor

struct Object_server_processor : Communication::Processor
{
                                Object_server_processor     ( Object_server_processor_channel* );


    void                        put_request_part            ( const char* data, int length );
    bool                        request_is_complete         ();

    void                        process                     ();

    bool                        response_is_complete        ();
    string                      get_response_part           ();
    bool                        should_close_connection     ();


    Fill_zero                          _zero_;
    Object_server_processor_channel*   _processor_channel;
    object_server::Input_message          _input_message;
    object_server::Input_message::Builder _input_message_builder;
    object_server::Output_message         _output_message;
};

//--------------------------------------------------------------------Object_server_processor_channel

struct Object_server_processor_channel : Communication::Processor_channel
{
                                Object_server_processor_channel( Communication::Channel* );

    ptr<Communication::Processor> processor                 ()                                      { ptr<Object_server_processor> result = Z_NEW( Object_server_processor( this ) ); 
                                                                                                      return +result; }

    ptr<object_server::Session> _session;
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
