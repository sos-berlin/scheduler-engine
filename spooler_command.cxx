// $Id: spooler_command.cxx,v 1.2 2001/01/05 20:31:22 jz Exp $

#include "../kram/sos.h"
#include "../kram/sleep.h"
#include "../file/anyfile.h"
#include "spooler.h"



namespace sos {
namespace spooler {

using namespace std;

//--------------------------------------------------------------------------dom_append_text_element
/*
void dom_append_text_element( xml::Element_ptr element, const char* element_name, const string& text )
{
    xml::Document_ptr doc       = element->ownerDocument;
    xml::Node_ptr     text_node = doc->createTextNode( as_dom_string( text ) );
    xml::Element_ptr  e         = element->appendChild( doc->createElement( element_name ) );

    e->appendChild( text_node );
}
*/
//------------------------------------------------------------Command_processor::execute_show_tasks

xml::Element_ptr Command_processor::execute_show_tasks()
{
    xml::Element_ptr tasks = _answer->createElement( "tasks" );

    FOR_EACH( Task_list, _spooler->_task_list, it )
    {
        Task* task = *it;
        xml::Element_ptr task_element = _answer->createElement( "task" );

        task_element->setAttribute( "job", as_dom_string( task->_job->_name ) );

        if( task->_running_since )
            task_element->setAttribute( "task.running_since", as_dom_string( Sos_optional_date_time( task->_running_since ).as_string() ) );

        task_element->setAttribute( "next_start_time", as_dom_string( Sos_optional_date_time( task->_next_start_time ).as_string() ) );
        task_element->setAttribute( "steps", as_dom_string( as_string( task->_step_count ) ) );

        tasks->appendChild( task_element );
    }

    return tasks;
}

//------------------------------------------------------------Command_processor::execute_show_state

xml::Element_ptr Command_processor::execute_show_state()
{
    xml::Element_ptr state_element = _answer->createElement( "state" );
 
    state_element->setAttribute( "time", as_dom_string( Sos_optional_date_time::now().as_string() ) );
    state_element->setAttribute( "spooler_running_since", as_dom_string( Sos_optional_date_time( _spooler->_spooler_start_time ).as_string() ) );
    
    state_element->appendChild( execute_show_tasks() );

    return state_element;
}

//---------------------------------------------------------------Command_processor::execute_command

xml::Element_ptr Command_processor::execute_command( xml::Element_ptr element )
{
    if( element->tagName == "show_state" )  
    {
        return execute_show_state();
    }
    else
    {
        throw_xc( "SOS-1425", as_string( element->tagName ) ); return NULL;
    }
}

//------------------------------------------------------------------------Command_processor::execute

string Command_processor::execute( const string& xml_text )
{
    Thread_semaphore::Guard guard = &_spooler->_semaphore;

    _answer = xml::Document_ptr( __uuidof(xml::DOMDocument30), NULL );

    _answer->appendChild( _answer->createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"iso-8859-1\"" ) );

    xml::Element_ptr spooler_answer = _answer->appendChild( _answer->createElement( "spooler.answer" ) );

    try 
    {
        try 
        {
            xml::Document_ptr command_doc ( __uuidof(xml::DOMDocument30), NULL );

            int ok = command_doc->loadXML( as_dom_string( xml_text ) );
            if( !ok ) // DOPPELT DOPPELT DOPPELT DOPPELT
            {
                xml::IXMLDOMParseErrorPtr error = command_doc->GetparseError();

                string text = w_as_string( error->reason );
                if( text[ text.length()-1 ] == '\n' )  text = as_string( text.c_str(), text.length() - 1 );
                if( text[ text.length()-1 ] == '\r' )  text = as_string( text.c_str(), text.length() - 1 );

                text += ", code="   + as_hex_string( error->errorCode );
                text += ", line="   + as_string( error->line );
                text += ", column=" + as_string( error->linepos );

                throw_xc( "XML-ERROR", text );
            }


            xml::Element_ptr e = command_doc->documentElement;

            if( e->tagName == "spooler.command" )
            {
                xml::NodeList_ptr node_list = e->childNodes;

                for( int i = 0; i < node_list->length; i++ )
                {
                    xml::Node_ptr node = node_list->Getitem(i);

                    spooler_answer->appendChild( execute_command( node ) );
                }
            }
            else
            {
                spooler_answer->appendChild( execute_command( e ) );
            }
        }
        catch( const _com_error& com_error ) { throw_com_error(com_error); return NULL; }
    }
    catch( const Xc& x )
    {
        xml::Element_ptr e = _answer->createElement( "COMMAND_ERROR" );
        e->appendChild( _answer->createTextNode( as_dom_string( x.what() ) ) );
        spooler_answer->appendChild( e );
    }

    _answer->save( "c:/tmp/~spooler.xml" );
    _answer = NULL;
    return file_as_string( "c:/tmp/~spooler.xml" );
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
