// $Id: spooler_process.h 3784 2005-07-01 17:41:48Z jz $

#ifndef __SPOOLER_WEB_SERVICE_H
#define __SPOOLER_WEB_SERVICE_H


namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

struct Web_service_operation;

//--------------------------------------------------------------------------------------Web_service

struct Web_service: idispatch_implementation< Web_service, spooler_com::Iweb_service >,
                    Scheduler_object
{
    static Class_descriptor     class_descriptor;
    static const Com_method     _methods[];

    static const string         forwarding_job_chain_name;
    static const string         forwarding_job_chain_forward_state;
    static const string         forwarding_job_chain_finished_state;


                                Web_service                 ( Spooler* );
    Z_GNU_ONLY(                 Web_service                 (); )
                               ~Web_service                 ();


    // interface Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Web_service"; }


    // interface Iweb_service
    STDMETHODIMP            get_Name                        ( BSTR* result )                        { return String_to_bstr( _name, result ); }
    STDMETHODIMP            get_Request_xslt_stylesheet_path( BSTR* result )                        { return String_to_bstr( _request_xslt_stylesheet_path, result ); }
    STDMETHODIMP            get_Response_xslt_stylesheet_path( BSTR* result )                       { return String_to_bstr( _response_xslt_stylesheet_path, result ); }
    STDMETHODIMP            get_Forward_xslt_stylesheet_path( BSTR* result )                        { return String_to_bstr( _forward_xslt_stylesheet_path, result ); }


    // Scheduler_object
    Prefix_log*                 log                         ()                                      { return _log; }
    string                      obj_name                    () const                                { return "Web_service " + _name; }

    void                        load                        ();
    void                        check                       ();
    void                    set_url_path                    ( const string& url_path )              { _url_path = url_path; }
    string                      url_path                    () const                                { return _url_path; }
    void                    set_dom                         ( const xml::Element_ptr&, const Time& xml_mod_time = Time() );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& = Show_what() );
    void                    set_name                        ( const string& name )                  { _name = name; }
    string                      name                        () const                                { return _name; }
    ptr<Web_service_operation>  new_operation               ( http::Operation* );

    xml::Document_ptr           transform_request           ( const xml::Document_ptr& request );
    xml::Document_ptr           transform_response          ( const xml::Document_ptr& command_answer );
    xml::Document_ptr           transform_forward           ( const xml::Document_ptr& order_or_task );
    void                        forward_order               ( const Order&, Job* last_job );
    void                        forward_task                ( const Task& );
    void                        forward                     ( const xml::Document_ptr& payload );
    
  private:
    friend struct               Web_service_operation;

    Fill_zero                  _zero_;
    string                     _name;
    string                     _url_path;
    int                        _next_operation_id;
    int                        _timeout;

    string                     _log_filename_prefix;
    ptr<Prefix_log>            _log;
    bool                       _log_xml;
    bool                       _debug;

    string                     _job_chain_name;

    string                     _request_xslt_stylesheet_path;
    Xslt_stylesheet            _request_xslt_stylesheet;
    string                     _response_xslt_stylesheet_path;
    Xslt_stylesheet            _response_xslt_stylesheet;
    string                     _forward_xslt_stylesheet_path;
    Xslt_stylesheet            _forward_xslt_stylesheet;
};

//-------------------------------------------------------------------------------------Web_services

struct Web_services
{
    Fill_zero _zero_;

                                Web_services                ( Spooler* sp )                         : _spooler(sp), _zero_(this+1) {}

    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& ) const;

    void                        add_web_services            ( const xml::Element_ptr& web_services_element );
    void                        add_web_service             ( Web_service* );
    void                        init                        ();

    Web_service*                web_service_by_url_path_or_null( const string& url_path );
    Web_service*                web_service_by_name         ( const string& name );
    Web_service*                web_service_by_name_or_null ( const string& name );

  private:
    Spooler*                   _spooler;

    typedef stdext::hash_map< string, ptr<Web_service> >    Name_web_service_map;
    Name_web_service_map       _name_web_service_map;

    typedef stdext::hash_map< string, ptr<Web_service> >    Url_web_service_map;
    Url_web_service_map        _url_web_service_map;
};

//---------------------------------------------------------------------------Web_service_operation 

struct Web_service_operation : idispatch_implementation< Web_service_operation, spooler_com::Iweb_service_operation >,
                               Scheduler_object
{
    static Class_descriptor     class_descriptor;
    static const Com_method     _methods[];


                                Web_service_operation       ( Web_service*, http::Operation*, int operation_id );
                               ~Web_service_operation       ();


    // Scheduler_object
    Prefix_log*                 log                         ()                                      { return _log; }
    string                      obj_name                    () const;
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& ) const;
    bool                        closed                      () const                                { return _http_operation == NULL; }

    // interface Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Web_service_operation"; }


    // Iweb_service_operation
    STDMETHODIMP            get_Web_service                 ( spooler_com::Iweb_service** result )  { *result = _web_service.copy();  return S_OK; }
    STDMETHODIMP            get_Request                     ( spooler_com::Iweb_service_request** );
    STDMETHODIMP            get_Response                    ( spooler_com::Iweb_service_response** );
    STDMETHODIMP                Execute_stylesheets         ();

    virtual void                close                       ();
    virtual void                begin                       ();
  //virtual bool                async_continue              ( Async_operation::Continue_flags );
  //virtual bool                async_finished              ();
    void                    set_host                        ( Host* );
    void                        cancel                      ();
    void                        execute_stylesheets         ();                                     // Führt _web_service->_request_xslt_stylesheet usw. aus

    int                         id                          () const                                { return _id; }

    http::Request*              http_request                () const                                { return _http_operation->request(); }
    http::Response*             http_response               () const                                { return _http_operation->response(); }

  private:
    friend struct               Web_service_request;
    friend struct               Web_service_response;

    Fill_zero                  _zero_;
    int                        _id;
    ptr<Web_service>           _web_service;
    ptr<Web_service_request>   _request;
    ptr<Web_service_response>  _response;
    http::Operation*           _http_operation;
    ptr<Order>                 _order;
    Time                       _timeout_at;
    string                     _log_filename_prefix;
    ptr<Prefix_log>            _log;
};

//----------------------------------------------------------------Web_service_stylesheet_operation 
/*
struct Web_service_stylesheet_operation : Web_service_operation
{
                                Web_service_stylesheet_operation( Web_service* ws, http::Operation* op, int operation_id ) : Web_service_operation( ws, op, operation_id ) {}

    void                        begin                       ();
    bool                        async_continue              ()                                      { return true; }
    bool                        async_finished              ()                                      { return true; }
};
*/
//------------------------------------------------------------------------------Web_service_request

struct Web_service_request : idispatch_implementation< Web_service_request, spooler_com::Iweb_service_request >,
                             Scheduler_object
{
    static Class_descriptor     class_descriptor;
    static const Com_method     _methods[];


                                Web_service_request         ( Web_service_operation* );

    // Scheduler_object
    Prefix_log*                 log                         ()                                      { return _web_service_operation->log(); }
    string                      obj_name                    () const;
    bool                        closed                      () const                                { return _web_service_operation == NULL; }

    // interface Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Web_service_request"; }

    // Iweb_service_request
    STDMETHODIMP            get_Url                         ( BSTR* );
    STDMETHODIMP            get_Header                      ( BSTR name, BSTR* );
  //STDMETHODIMP            get_Character_encoding          ( BSTR* result )                        { return String_to_bstr( http_request()->character_encoding(), result ); }
  //STDMETHODIMP            get_Content_type                ( BSTR* result )                        { return String_to_bstr( http_request()->content_type(), result ); }
    STDMETHODIMP            get_String_content              ( BSTR* );
    STDMETHODIMP            get_Binary_content              ( SAFEARRAY** );

    http::Request*              http_request                () const                                { return _web_service_operation->http_request(); }


  private:
    friend struct               Web_service_operation;

    Fill_zero                  _zero_;
    Web_service_operation*     _web_service_operation;
};

//-----------------------------------------------------------------------------Web_service_response

struct Web_service_response : idispatch_implementation< Web_service_response, spooler_com::Iweb_service_response >,
                              Scheduler_object
{
    static Class_descriptor     class_descriptor;
    static const Com_method     _methods[];


                                Web_service_response        ( Web_service_operation* );

    // Scheduler_object
    Prefix_log*                 log                         ()                                      { return _web_service_operation->log(); }
    string                      obj_name                    () const;
    bool                        closed                      () const                                { return _web_service_operation == NULL; }

    // interface Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Web_service_response"; }

    // Iweb_service_response
    STDMETHODIMP            put_Status_code                 ( int code );
    STDMETHODIMP            put_Header                      ( BSTR name, BSTR value );
    STDMETHODIMP            get_Header                      ( BSTR name, BSTR* result );
  //STDMETHODIMP            put_Character_encoding          ( BSTR encoding );
  //STDMETHODIMP            get_Character_encoding          ( BSTR* result );
  //STDMETHODIMP            put_Content_type                ( BSTR content_type );
  //STDMETHODIMP            get_Content_type                ( BSTR* result );
    STDMETHODIMP            put_String_content              ( BSTR );
    STDMETHODIMP            put_Binary_content              ( SAFEARRAY* );
    STDMETHODIMP                Send                        ();
  //STDMETHODIMP                Send                        ( VARIANT*, BSTR );

    http::Response*             http_response               () const                                { return _web_service_operation->http_response(); }


  private:
    friend struct               Web_service_operation;

    Fill_zero                  _zero_;
    Web_service_operation*     _web_service_operation;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
