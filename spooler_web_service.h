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

    static const string Web_service::forwarding_job_chain_name;
    static const string Web_service::forwarding_job_chain_forward_state;
    static const string Web_service::forwarding_job_chain_finished_state;


                                Web_service                 ( Spooler* );
    Z_GNU_ONLY(                 Web_service                 (); )
                               ~Web_service                 ();


    // interface Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Web_service"; }


    // interface Isubprocess
    STDMETHODIMP            get_Name                        ( BSTR* result )                        { return String_to_bstr( _name, result ); }
    STDMETHODIMP            get_Forward_xslt_stylesheet_path( BSTR* result )                        { return String_to_bstr( _forward_xslt_stylesheet_path, result ); }


    // Scheduler_object
    Prefix_log*                 log                         ()                                      { return _log; }
    string                      obj_name                    () const                                { return "Web_service " + _name; }


    void                        load                        ();
    string                      url_path                    () const                                { return _url_path; }

    void                    set_dom                         ( const xml::Element_ptr&, const Time& xml_mod_time = Time() );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& = Show_what() );

    string                      name                        () const                                { return _name; }
    ptr<Web_service_operation>  new_operation               ( Http_operation* );
    xml::Document_ptr           transform_request           ( const xml::Document_ptr& request );
    xml::Document_ptr           transform_response          ( const xml::Document_ptr& command_answer );
    xml::Document_ptr           transform_forward           ( const xml::Document_ptr& order_or_task );
    void                        forward_order               ( const Order&, Job* last_job );
    void                        forward_task                ( const Task& );
    void                        forward                     ( const xml::Document_ptr& payload );
    
  private:
    friend struct               Web_service_operation;
  //friend Web_service*         Spooler::web_service_by_url_path_or_null( const string& url_path );

    Fill_zero                  _zero_;
    string                     _name;
    string                     _url_path;
    string                     _log_filename_prefix;
    ptr<Prefix_log>            _log;
    string                     _request_xslt_stylesheet_path;
    Xslt_stylesheet            _request_xslt_stylesheet;
    string                     _response_xslt_stylesheet_path;
    Xslt_stylesheet            _response_xslt_stylesheet;
    string                     _forward_xslt_stylesheet_path;
    Xslt_stylesheet            _forward_xslt_stylesheet;
    string                     _job_chain_name;
    int                        _next_operation_id;
    bool                       _debug;
    bool                       _log_xml;
};

//-------------------------------------------------------------------------------------Web_services

struct Web_services
{
    Fill_zero _zero_;

                                Web_services                ( Spooler* sp )                         : _spooler(sp), _zero_(this+1) {}

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


                                Web_service_operation       ( Web_service*, Http_operation*, int operation_id );
                               ~Web_service_operation       ();


    // Scheduler_object
    Prefix_log*                 log                         ()                                      { return _log; }
    string                      obj_name                    () const;

    // interface Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Web_service_operation"; }


    // Iweb_service_operation
    STDMETHODIMP            get_Web_service                 ( spooler_com::Iweb_service** result )  { *result = _web_service.copy();  return S_OK; }
    STDMETHODIMP            get_Request                     ( spooler_com::Iweb_service_request** result );
    STDMETHODIMP            get_Response                    ( spooler_com::Iweb_service_response** result );

    void                        process_http__begin         ( Http_operation* );
    ptr<Http_response>          process_http__end           ();
    bool                        async_continue              ();
    bool                        async_finished              ();

    string                      process_request__begin      ( const string& request_data, const string& character_encoding );
    string                      process_request__old_style  ( const string& request_data, const string& character_encoding );      // AUßER BETRIEB
    int                         id                          () const                                { return _id; }

  private:
    Fill_zero                  _zero_;
    int                        _id;
    ptr<Web_service>           _web_service;
    ptr<Web_service_request>   _web_service_request;
    ptr<Web_service_response>  _web_service_response;
    ptr<Http_operation>        _http_operation;
    ptr<Order>                 _order;
    string                     _log_filename_prefix;
    ptr<Prefix_log>            _log;
};

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

    // interface Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Web_service_request"; }


    // Iweb_service_request
    STDMETHODIMP            get_Url                         ( BSTR* result )                        { return String_to_bstr( _http_request->url(), result ); }
    STDMETHODIMP            get_Header                      ( BSTR name, BSTR* result )             { return String_to_bstr( _http_request->header_field( string_from_bstr( name ) ), result ); }
    STDMETHODIMP            get_Character_encoding          ( BSTR* result )                        { return String_to_bstr( _http_request->character_encoding(), result ); }
    STDMETHODIMP            get_Content_type                ( BSTR* result )                        { return String_to_bstr( _http_request->content_type(), result ); }
    STDMETHODIMP            get_String_content              ( BSTR* );
    STDMETHODIMP            get_Binary_content              ( SAFEARRAY** );


  private:
    Fill_zero                  _zero_;
    ptr<Web_service_operation> _web_service_operation;
    ptr<Http_request>          _http_request;
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

    // interface Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Web_service_response"; }


    // Iweb_service_response
    STDMETHODIMP            put_Header                      ( BSTR name, BSTR value )                { _http_response->set_header_field( string_from_bstr( name ), string_from_bstr( value ) );  return S_OK; }
    STDMETHODIMP            put_Character_encoding          ( BSTR encoding )                        { _http_response->set_character_encoding( string_from_bstr( encoding ) );  return S_OK; }
    STDMETHODIMP            put_Content_type                ( BSTR content_type )                    { _http_response->set_content_type( string_from_bstr( content_type ) );  return S_OK; }
    STDMETHODIMP            put_String_content              ( BSTR );
    STDMETHODIMP            put_Binary_content              ( SAFEARRAY* );


  private:
    Fill_zero                  _zero_;
    ptr<Web_service_operation> _web_service_operation;
    ptr<Http_response>         _http_response;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
