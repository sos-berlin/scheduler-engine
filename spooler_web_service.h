// $Id: spooler_process.h 3784 2005-07-01 17:41:48Z jz $

#ifndef __SPOOLER_WEB_SERVICE_H
#define __SPOOLER_WEB_SERVICE_H


namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

struct Web_service_transaction;

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
    ptr<Web_service_transaction> new_transaction            ( Http_processor* );
    xml::Document_ptr           transform_request           ( const xml::Document_ptr& request );
    xml::Document_ptr           transform_response          ( const xml::Document_ptr& command_answer );
    xml::Document_ptr           transform_forward           ( const xml::Document_ptr& order_or_task );
    void                        forward_order               ( const Order&, Job* last_job );
    void                        forward_task                ( const Task& );
    void                        forward                     ( const xml::Document_ptr& payload );
    
  private:
    friend struct               Web_service_transaction;
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
    int                        _next_transaction_number;
    bool                       _debug;
    bool                       _log_xml;
};

//-------------------------------------------------------------------------------------------------

struct Web_service_transaction : zschimmer::Object,
                                 Scheduler_object
{
                                Web_service_transaction     ( Web_service*, Http_processor*, int transaction_number );


    // Scheduler_object
    Prefix_log*                 log                         ()                                      { return _log; }
    string                      obj_name                    () const;


    ptr<Http_response>          process_http                ( Http_processor* );
    string                      process_request             ( const string& request_data );

  private:
    Fill_zero                  _zero_;
    ptr<Web_service>           _web_service;
    int                        _transaction_number;
    ptr<Http_processor>        _http_processor;
    string                     _log_filename_prefix;
    ptr<Prefix_log>            _log;
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

    int                        _next_transaction_number;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
