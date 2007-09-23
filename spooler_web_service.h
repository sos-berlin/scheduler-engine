// $Id$

#ifndef __SPOOLER_WEB_SERVICE_H
#define __SPOOLER_WEB_SERVICE_H


namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------------------

struct Web_service_operation;

//struct Web_service_base : Scheduler_object
//{
//                                Web_service_base            ( Scheduler* scheduler, IUnknown* iunknown, Type_code type ) : Scheduler_object( scheduler, iunknown, type ) {}
//    virtual void                activate                    ()                                      = 0;
//};
//
//------------------------------------------------------------------------------Http_file_directory

struct Http_file_directory : Object, Scheduler_object
{
                                Http_file_directory         ( Scheduler* scheduler, const string& url_path, const File_path& directory ) : Scheduler_object( scheduler, this, type_http_file_directory ),
                                                                                                                                            _url_path(url_path), _directory(directory) {}

    string                      url_path                    () const                                { return _url_path; }
    File_path                   directory                   () const                                { return _url_path; }
    File_path                   file_path_from_url_path     ( const string& url_path );
  //void                        execute_request             ();

  private:
    File_path                  _directory;
    string                     _url_path;
};

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
    STDMETHODIMP            get_Params                      ( spooler_com::Ivariable_set** result ) { *result = _parameters.copy();  return S_OK; }



    // Scheduler_object
    string                      obj_name                    () const                                { return "Web_service " + _name; }

    void                        activate                    ();
    void                        check                       ();
    void                    set_url_path                    ( const string& url_path )              { _url_path = url_path; }
    string                      url_path                    () const                                { return _url_path; }
    void                    set_dom                         ( const xml::Element_ptr& );
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
    Fill_zero                  _zero_;
    friend struct               Web_service_operation;
    void                        load_xslt_stylesheet        ( Xslt_stylesheet*, const string& path );

    string                     _name;
    string                     _url_path;
    int                        _next_operation_id;
    int                        _timeout;

    string                     _log_filename_prefix;
    bool                       _log_xml;
    bool                       _debug;

    Absolute_path              _job_chain_path;

    string                     _request_xslt_stylesheet_path;
    Xslt_stylesheet            _request_xslt_stylesheet;
    string                     _response_xslt_stylesheet_path;
    Xslt_stylesheet            _response_xslt_stylesheet;
    string                     _forward_xslt_stylesheet_path;
    Xslt_stylesheet            _forward_xslt_stylesheet;
    ptr<Com_variable_set>      _parameters;
};

//---------------------------------------------------------------------------Web_services_interface

struct Web_services_interface: Object, Subsystem
{
                                Web_services_interface      ( Scheduler* s, Type_code type )       : Subsystem( s, this, type ) {}

    virtual void                set_dom                     ( const xml::Element_ptr& )             = 0;
    virtual xml::Element_ptr    dom_element                 ( const xml::Document_ptr&, const Show_what& ) const = 0;

    virtual Web_service*        web_service_by_url_path_or_null( const string& )                    = 0;
    virtual Web_service*        web_service_by_name         ( const string& )                       = 0;
    virtual Web_service*        web_service_by_name_or_null ( const string& )                       = 0;
    virtual Http_file_directory* http_file_directory_by_url_path_or_null( const string& )           = 0;
    virtual bool                need_authorization         ()                                      = 0;
    virtual bool                is_request_authorized       ( http::Request* )                      = 0;
};


ptr<Web_services_interface>     new_web_services            ( Scheduler* );

//---------------------------------------------------------------------------Web_service_operation 

struct Web_service_operation : idispatch_implementation< Web_service_operation, spooler_com::Iweb_service_operation >,
                               Scheduler_object
{
    static Class_descriptor     class_descriptor;
    static const Com_method     _methods[];


                                Web_service_operation       ( Web_service*, http::Operation*, int operation_id );
                               ~Web_service_operation       ();


    // Scheduler_object
    string                      obj_name                    () const;
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& ) const;
    bool                        closed                      () const                                { return _http_operation == NULL; }
    void                        assert_usable               ();

    // interface Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Web_service_operation"; }


    // Iweb_service_operation
    STDMETHODIMP            get_Web_service                 ( spooler_com::Iweb_service** result )  { *result = _web_service.copy();  return S_OK; }
    STDMETHODIMP            get_Request                     ( spooler_com::Iweb_service_request** );
    STDMETHODIMP            get_Response                    ( spooler_com::Iweb_service_response** );
    STDMETHODIMP                Assert_is_usable            ();
    STDMETHODIMP            get_Peer_ip                     ( BSTR* );
    STDMETHODIMP            get_Peer_hostname               ( BSTR* );

    virtual void                close                       ();
    virtual void                begin                       ();
    void                    set_host                        ( Host* );
    void                        execute_stylesheets         ();                                     // Führt _web_service->_request_xslt_stylesheet usw. aus

    int                         id                          () const                                { return _id; }

    Web_service*                web_service                 () const                                { return _web_service; }
    http::Operation*            http_operation              () const                                { return _http_operation; }
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
    Time                       _timeout_at;
    string                     _log_filename_prefix;
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
    void                        assert_usable               ();

    // interface Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Web_service_request"; }

    // Iweb_service_request
    STDMETHODIMP            get_Url                         ( BSTR* );
    STDMETHODIMP            get_Header                      ( BSTR name, BSTR* );
    STDMETHODIMP            get_Charset_name                ( BSTR* );
    STDMETHODIMP            get_Content_type                ( BSTR* );
    STDMETHODIMP            get_String_content              ( BSTR* );
    STDMETHODIMP            get_Binary_content              ( SAFEARRAY** );

    STDMETHODIMP                Assert_is_usable            ();

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
    void                        assert_usable               ();

    // interface Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Web_service_response"; }

    // Iweb_service_response
    STDMETHODIMP            put_Status_code                 ( int code );
    STDMETHODIMP            put_Header                      ( BSTR name, BSTR value );
    STDMETHODIMP            get_Header                      ( BSTR name, BSTR* result );
    STDMETHODIMP            put_Charset_name                ( BSTR );
    STDMETHODIMP            get_Charset_name                ( BSTR* );
    STDMETHODIMP            put_Content_type                ( BSTR );
    STDMETHODIMP            get_Content_type                ( BSTR* );
    STDMETHODIMP            put_String_content              ( BSTR );
    STDMETHODIMP            put_Binary_content              ( SAFEARRAY* );
    STDMETHODIMP                Send                        ();
  //STDMETHODIMP                Send                        ( VARIANT*, BSTR );

    STDMETHODIMP                Assert_is_usable            ();

    http::Response*             http_response               () const                                { return _web_service_operation->http_response(); }


  private:
    friend struct               Web_service_operation;

    Fill_zero                  _zero_;
    Web_service_operation*     _web_service_operation;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
