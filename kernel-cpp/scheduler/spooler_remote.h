// $Id: spooler_remote.h 13198 2007-12-06 14:13:38Z jz $

namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------------------

//struct Object_server_processor_channel;

//-------------------------------------------------------------------------Remote_client_connection

struct Remote_client_connection : Async_operation, Subsystem
{
    enum State
    {
        s_not_connected,
        s_connecting,
        s_registering,
        s_registered
    };

    static string               state_name                  ( State );


                                Remote_client_connection    ( Spooler*, const Host_and_port& );
                               ~Remote_client_connection    ();

    void                        close                       ();
    virtual string              obj_name                    () const;

    State                       state                       () const                                { return _state; }
    void                        connect                     ();


  protected:
    string                      async_state_text_           () const                                { return obj_name(); }
    bool                        async_continue_             ( Continue_flags );
    bool                        async_finished_             () const                                { return _state == s_not_connected  
                                                                                                          || _state == s_registered; }
  //bool                        async_signaled_             ()                                      { return _socket_operation && _socket_operation->async_signaled(); }

  private:
    Fill_zero                  _zero_;
    State                      _state;
    Host_and_port              _host_and_port;
    ptr<Xml_client_connection> _xml_client_connection;
};

//------------------------------------------------------------------------Main_scheduler_connection
// Verbindung zum Main Scheduler
/*
struct Main_scheduler_connection : Async_operation
{
    enum State
    {
        s_initial,
        s_connecting,
        s_stand_by,
                                Main_scheduler_connection( Spooler*, const Host_and_port& );


  protected:
    string                      async_state_text_           ();
    bool                        async_continue_             ( Continue_flags );
    bool                        async_finished_             ()                                      { return _state == s_initial  

  private:
    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Remote_client_connection      _xml_client_connection;
};
*/
//-----------------------------------------------------------------------------Xml_client_operation
/*
struct Xml_client_operation : Operation
{
    Xml_client_operation( Remote_client_connection* conn ) : _connection( conn ) {}

    ptr<Remote_client_connection> _connection;
};
*/
//-------------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------Object_server_processor
/*
struct Object_server_processor : Communication::Operation
{
                                Object_server_processor     ( Object_server_processor_channel* );


    void                        put_request_part            ( const char* data, int length );
    bool                        request_is_complete         ();

    void                        process                     ();

    bool                        response_is_complete        ();
    string                      get_response_part           ();
    bool                        should_close_connection     ();


    Fill_zero                          _zero_;
    Object_server_processor_channel*      _operation_channel;
    object_server::Input_message          _input_message;
    object_server::Input_message::Builder _input_message_builder;
    object_server::Output_message         _output_message;
};

//--------------------------------------------------------------------Object_server_processor_channel

struct Object_server_processor_channel : Communication::Operation_channel
{
                                Object_server_processor_channel( Communication::Channel* );

    ptr<Communication::Operation> processor                 ()                                      { ptr<Object_server_processor> result = Z_NEW( Object_server_processor( this ) ); 
                                                                                                      return +result; }

    ptr<object_server::Session> _session;
};
*/
//-----------------------------------------------------------------------------------Remote_scheduler

struct Remote_scheduler : zschimmer::Object
{
                                Remote_scheduler            ()                                      : _zero_(this+1){}

    void                        connection_lost_event       ( const exception* );
    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr& document, const Show_what& show );


    Fill_zero                  _zero_;
    Host_and_port              _host_and_port;
    string                     _scheduler_id;
    string                     _version;
    Time                       _connected_at;
    Time                       _disconnected_at;
    bool                       _logged_on;
    bool                       _is_connected;
    Xc_copy                    _error;

  //ptr<object_server::Proxy>  _scheduler_proxy;
};

//-------------------------------------------------------------------------------------------------

struct Remote_scheduler_register
{
                                Remote_scheduler_register   ()                                      : _zero_(this+1){}


    void                        add                         ( Remote_scheduler* );
    Remote_scheduler*           get_or_null                 ( const Host_and_port& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr& document, const Show_what& show );


    Fill_zero                  _zero_;
    typedef map< Host_and_port, ptr<Remote_scheduler> >   Map;
    Map                                                  _map;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
