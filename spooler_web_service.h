// $Id: spooler_process.h 3784 2005-07-01 17:41:48Z jz $

#ifndef __SPOOLER_WEB_SERVICE_H
#define __SPOOLER_WEB_SERVICE_H


namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

struct Web_service_transaction;

//--------------------------------------------------------------------------------------Web_service

struct Web_service : Object
{
                                Web_service                 ( Spooler* sp )                         : _spooler(sp), _zero_(this+1) {}
                                Web_service                 ( Spooler* sp, const xml::Element_ptr& e ) : _spooler(sp), _zero_(this+1) { set_dom( e ); }
    Z_GNU_ONLY(                 Web_service                 (); )
                               ~Web_service                 ();

    string                      url_path                    () const                                { return _url_path; }

    void                    set_dom                         ( const xml::Element_ptr&, const Time& xml_mod_time = Time() );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& = Show_what() );

    ptr<Web_service_transaction> new_transaction            ( Http_processor* );
    xml::Document_ptr           transform_request           ( const xml::Document_ptr& request );
    xml::Document_ptr           transform_response          ( const xml::Document_ptr& command_answer );
  //string                      transform_forward           ( const string& request_xml );
    void                        forward_order               ( Order* );
    void                        forward_task                ( Task* );
    
  private:
    friend struct               Web_service_transaction;
  //friend Web_service*         Spooler::web_service_by_url_path_or_null( const string& url_path );

    Fill_zero                  _zero_;
    Spooler* const             _spooler;
    string                     _name;
    string                     _url_path;
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

struct Web_service_transaction : zschimmer::Object
{
                                Web_service_transaction     ( Web_service* ws, Http_processor* ht ) : _zero_(this+1), _spooler(ws->_spooler), _web_service(ws), _http_processor(ht) {}

    string                      process_request             ( const string& request_data );

  private:
    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    ptr<Web_service>           _web_service;
    ptr<Http_processor>        _http_processor;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
