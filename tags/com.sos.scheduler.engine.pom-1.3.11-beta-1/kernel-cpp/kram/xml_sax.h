// $Id$   

#ifndef __SOS_XML_SAX_H
#define __SOS_XML_SAX_H

#ifdef SYSTEM_WIN
//#   import <msxml3.dll> raw_interfaces_only 
//    typedef MSXML2::ISAXAttributes Xml_sax_attributes;
    namespace MSXML2 
    {
        struct ISAXAttributes;
        struct ISAXXMLReader;
    }

    typedef MSXML2::ISAXAttributes Xml_sax_attributes;
    typedef MSXML2::ISAXXMLReader  Xml_sax_reader;

#endif

namespace sos {
namespace xml {
namespace sax {

#ifdef SYSTEM_WIN
    struct Content_handler_for_com;
#endif

//----------------------------------------------------------------------------------------Attributes

struct Attributes
{
                                Attributes              ( Xml_sax_attributes* a )         : _delegated(a) {}

    int                         length                  ();
    string                      local_name              ( int );
    string                      value                   ( int );

    Xml_sax_attributes*        _delegated;
};

//-----------------------------------------------------------------------------------Content_handler

struct Content_handler : Sos_self_deleting
{
    virtual                    ~Content_handler         () {}

    virtual void                start_element           ( const string& namespace_uri, const string& local_name, const string& name, const Attributes& ) = 0;
    virtual void                end_element             ( const string& namespace_uri, const string& local_name, const string& name ) = 0;
    virtual void                start_document          () = 0;
};

//--------------------------------------------------------------------------------------------Reader

struct Reader
{
                                Reader                  ();
    virtual                    ~Reader                  ();

    void                        set_content_handler     ( Content_handler* );
    void                        parse                   ( const string& document );

    Xml_sax_reader*            _delegated;
    Content_handler_for_com*   _content_handler_for_com;
    Sos_ptr<Content_handler>   _content_handler;
};


} //namespace sax
} //namespace xml
} //namespace sos

#endif