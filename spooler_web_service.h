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

    string                      url_path                    () const                                { return _url_path; }

    void                    set_dom                         ( const xml::Element_ptr&, const Time& xml_mod_time = Time() );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& = Show_what() );

    string                      name                        () const                                { return _name; }
    ptr<Web_service_transaction> new_transaction            ( Http_processor* );
    xml::Document_ptr           transform_request           ( const xml::Document_ptr& request );
    xml::Document_ptr           transform_response          ( const xml::Document_ptr& command_answer );
    void                        forward_order               ( Order* );
    void                        forward_task                ( Task* );
    
  private:
    friend struct               Web_service_transaction;
  //friend Web_service*         Spooler::web_service_by_url_path_or_null( const string& url_path );

    Fill_zero                  _zero_;
    string                     _name;
    string                     _url_path;
    ptr<Prefix_log>            _log;
    string                     _request_xslt_stylesheet_path;
    Xslt_stylesheet            _request_xslt_stylesheet;
    string                     _response_xslt_stylesheet_path;
    Xslt_stylesheet            _response_xslt_stylesheet;
    string                     _forward_xslt_stylesheet_path;
    Xslt_stylesheet            _forward_xslt_stylesheet;
    bool                       _debug;
};

//----------------------------------------------------------------------------------Web_service_map

typedef map< string, ptr<Web_service> >    Web_service_map;

//-------------------------------------------------------------------------------------------------

struct Web_service_transaction : zschimmer::Object,
                                 Scheduler_object
{
                                Web_service_transaction     ( Web_service*, Http_processor* );


    // Scheduler_object
    Prefix_log*                 log                         ()                                      { return _log; }


    ptr<Http_response>          process_http                ( Http_request* );
    string                      process_request             ( const string& request_data );

  private:
    Fill_zero                  _zero_;
    ptr<Prefix_log>            _log;
    ptr<Web_service>           _web_service;
    ptr<Http_processor>        _http_processor;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
