// $Id: spooler_xml.cxx,v 1.2 2001/01/02 12:50:25 jz Exp $

//#include <precomp.h>


#include "../kram/sos.h"
#include "../kram/olestd.h"
#include "../file/anyfile.h"
#include "spooler.h"


namespace sos {
namespace spooler {


//-------------------------------------------------------------------------------------------------

typedef _bstr_t Dom_string;

inline Dom_string               as_dom_string           ( const string& str )                       { return as_bstr_t( str ); }
inline Dom_string               as_dom_string           ( const char* str )                         { return as_bstr_t( str ); }

//----------------------------------------------------------------------------------single_element

xml::Element_ptr single_element( xml::Element_ptr element, const string& name )
{
    xml::NodeList_ptr list = element->getElementsByTagName( as_dom_string( name ) );

    int len = list->length;
    if( len == 0 )  throw_xc( "SOS-1422", name );
    if( len > 1 )  throw_xc( "SOS-1423", name );

    return list->Getitem(0);
}

//---------------------------------------------------------------Object_set_class::Object_set_class

Object_set_class::Object_set_class( xml::Element_ptr element )
{
    _name = as_string( element->getAttribute( "name" ) );

    xml::Element_ptr script_element = single_element( element, "script" );

    _script = script_element->text;
    _script_language = as_string( script_element->getAttribute( "language" ) );

    xml::NodeList_ptr levels_nodelist = element->getElementsByTagName( "level_decls" );
    if( levels_nodelist->length >= 1 ) 
    {
        xml::NodeList_ptr node_list = levels_nodelist->Getitem(0)->childNodes;

        for( int i = 0; i < node_list->length; i++ )
        {
            xml::Node_ptr n = node_list->Getitem(i);

            if( n->nodeName == "level_decl" ) 
            {
                int level = as_int( single_element( n, "level" )->text );
                string name = single_element( n, "level_description" )->text;

                _level_map[ level ] = name;
            }
        }
    }
}

//-------------------------------------------------------------------Level_interval::Level_interval

Level_interval::Level_interval( xml::Element_ptr element )
:
    _low_level ( as_int( single_element( element, "low_level" )->text ) ),
    _high_level( as_int( single_element( element, "high_level" )->text ) )
{
}

//----------------------------------------------------------------Object_set_descr::Object_set_descr

Object_set_descr::Object_set_descr( xml::Element_ptr element )
: 
    _class_name( single_element( element, "object_set_class.name" )->text ),
    _level_interval( single_element( element, "levels" ) )
{
}

//-----------------------------------------------------------------------------------------Job::Job

Job::Job( xml::Element_ptr element )
: 
    _zero_(this+1)
{
    _object_set_descr = single_element( element, "object_set" );
    _output_level = as_int( single_element( element, "output_level" )->text );
}

//--------------------------------------------------------Spooler::load_object_set_classes_from_xml

void Spooler::load_object_set_classes_from_xml( Object_set_class_list* list, xml::Element_ptr element )
{
    xml::NodeList_ptr node_list = element->childNodes;

    for( int i = 0; i < node_list->length; i++ )
    {
        xml::Node_ptr n = node_list->Getitem(i);

        if( n->nodeName == "object_set_class" )  list->push_back( SOS_NEW( Object_set_class( n ) ) );
    }
}

//----------------------------------------------------------------------Spooler::load_jobs_from_xml

void Spooler::load_jobs_from_xml( Job_list* list, xml::Element_ptr element )
{
    xml::NodeList_ptr node_list = element->childNodes;

    for( int i = 0; i < node_list->length; i++ )
    {
        xml::Node_ptr n = node_list->Getitem(i);

        if( n->nodeName == "job" ) 
        {
            try
            {
                Sos_ptr<Job> job = SOS_NEW( Job( n ) );

                for( Object_set_class_list::iterator it = _object_set_class_list.begin(); it != _object_set_class_list.end(); it++ )
                {
                    if( (*it)->_name == job->_object_set_descr._class_name )  break;
                }
                if( it == _object_set_class_list.end() )  throw_xc( "SPOOLER-101", job->_object_set_descr._class_name );

                job->_object_set_descr._class = *it;

                list->push_back( job );
            }
            catch( const Xc& )
            {
                throw; // Fehler abfangen, melden und dann fortfahren
            }
        }
    }
}

//--------------------------------------------------------------------------------Spooler::load_xml

void Spooler::load_xml()
{
    try 
    {
        xml::Document_ptr document ( __uuidof(xml::DOMDocument30), NULL );

        int ok = document->loadXML( as_dom_string( file_as_string( "spooler_config.xml" ) ) );
        if( !ok ) 
        {
            xml::IXMLDOMParseErrorPtr error = document->GetparseError();

            string text = w_as_string( error->reason );
            text += ", code="   + as_string( error->errorCode );
            text += ", line="   + as_string( error->line );
            text += ", column=" + as_string( error->linepos );

            throw_xc( "XML-ERROR", text );
        }

        xml::Element_ptr  spooler_config = document->documentElement;
        xml::NodeList_ptr node_list      = spooler_config->childNodes;

        for( int i = 0; i < node_list->length; i++ )
        {
            xml::Node_ptr node = node_list->Getitem(i);

            if( node->nodeName == "object_set_classes" )
            {
                load_object_set_classes_from_xml( &_object_set_class_list, node );
            }
            else
            if( node->nodeName == "jobs" ) 
            {
                load_jobs_from_xml( &_job_list, node );
            }
        }
    }
    catch( const _com_error& com_error )  { throw_com_error( com_error ); }
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
