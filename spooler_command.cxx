// $Id: spooler_command.cxx,v 1.6 2001/01/07 16:35:19 jz Exp $

#include "../kram/sos.h"
#include "../kram/sleep.h"
#include "../file/anyfile.h"
#include "spooler.h"



namespace sos {
namespace spooler {

using namespace std;

//--------------------------------------------------------------------------dom_append_text_element
/*
void dom_append_text_element( const xml::Element_ptr& element, const char* element_name, const string& text )
{
    xml::Document_ptr doc       = element->ownerDocument;
    xml::Node_ptr     text_node = doc->createTextNode( as_dom_string( text ) );
    xml::Element_ptr  e         = element->appendChild( doc->createElement( element_name ) );

    e->appendChild( text_node );
}
*/

//-----------------------------------------------------------------------------create_error_element

xml::Element_ptr create_error_element( xml::Document* document, const Xc_copy& x )
{
    xml::Element_ptr e = document->createElement( "ERROR" );

    if( !empty( x->name() )          )  e->setAttribute( "class" , as_dom_string( x->name()          ) );

    e->setAttribute( "code", as_dom_string( x->code() ) );
    
    if( !empty( x->_pos.filename() ) )  e->setAttribute( "source", as_dom_string( x->_pos.filename() ) );
    if( x->_pos._line >= 0           )  e->setAttribute( "line"  , as_dom_string( x->_pos._line + 1  ) );
    if( x->_pos._col  >= 0           )  e->setAttribute( "col"   , as_dom_string( x->_pos._col + 1   ) );

    e->setAttribute( "text", as_dom_string( x->what() ) );
    e->setAttribute( "time", as_dom_string( Sos_optional_date_time( x.time() - _timezone ).as_string() ) );

    return e;
}

//-----------------------------------------------------------------------------append_error_element

void append_error_element( xml::Element* element, const Xc_copy& x )
{
    element->appendChild( create_error_element( element->ownerDocument, x ) );
}

//------------------------------------------------------------Command_processor::execute_show_tasks

xml::Element_ptr Command_processor::execute_show_task( Task* task )
{
    xml::Element_ptr task_element = _answer->createElement( "task" );

    task_element->setAttribute( "job", as_dom_string( task->_job->_name ) );
    task_element->setAttribute( "running", task->_running? "yes" : "no" );

    if( task->_running )
        task_element->setAttribute( "running_since", as_dom_string( Sos_optional_date_time( task->_running_since ).as_string() ) );

    if( !task->_stopped )
        task_element->setAttribute( "next_start_time", as_dom_string( Sos_optional_date_time( task->_next_start_time ).as_string() ) );

    task_element->setAttribute( "steps", as_dom_string( as_string( task->_step_count ) ) );

    if( task->_error )  append_error_element( task_element, task->_error );

    return task_element;
}

//------------------------------------------------------------Command_processor::execute_show_tasks

xml::Element_ptr Command_processor::execute_show_tasks()
{
    xml::Element_ptr tasks = _answer->createElement( "tasks" );

    FOR_EACH( Task_list, _spooler->_task_list, it )
    {
        tasks->appendChild( execute_show_task( *it ) );
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

//------------------------------------------------------------Command_processor::execute_modify_job

xml::Element_ptr Command_processor::execute_modify_job( const xml::Element_ptr& element )
{
    string job_name = as_string( element->getAttribute( "job" ) );
    string action   = as_string( element->getAttribute( "action" ) );
    xml::Element_ptr tasks_element = _answer->createElement( "tasks" );
    bool found = false;

    FOR_EACH( Task_list, _spooler->_task_list, it )
    {
        Task* task = *it;
        if( task->_job->_name == job_name ) 
        {
            found = true;

            if( action == "start" )  task->cmd_start();
            else
            if( action == "stop"  )  task->cmd_stop();
            else
                throw_xc( "SPOOLER-106", action );

            tasks_element->appendChild( execute_show_task( *it ) );
        }
    }
    
    if( !found )  throw_xc( "SPOOLER-108", job_name );

    return tasks_element;
}

//--------------------------------------------------------Command_processor::execute_modify_spooler

xml::Element_ptr Command_processor::execute_modify_spooler( const xml::Element_ptr& element )
{
    string action = as_string( element->getAttribute( "action" ) );
    if( !action.empty() )
    {
        if( action == "stop"      )  _spooler->cmd_stop();
        else
        if( action == "restart"   )  _spooler->cmd_restart();
        else
        if( action == "terminate" )  _spooler->cmd_terminate();
        else
            throw_xc( "SPOOLER-106", action );
    }
    
    string paused = as_string( element->getAttribute( "paused" ) );
    if( !paused.empty() )
    {
        if( paused == "yes" )  _spooler->cmd_pause();
                         else  _spooler->cmd_continue();
    }
    
    return _answer->createElement( "ok" );
}

//---------------------------------------------------------------Command_processor::execute_command

xml::Element_ptr Command_processor::execute_command( const xml::Element_ptr& element )
{
    if( element->tagName == "show_state"        )  return execute_show_state();
    else
/*
    if( element->tagName == "pause_spooler"     )  { _spooler->cmd_pause();     return _answer->createElement( "ok" ); }
    else
    if( element->tagName == "continue_spooler"  )  { _spooler->cmd_continue();  return _answer->createElement( "ok" ); }
    else
    if( element->tagName == "stop_spooler"      )  { _spooler->cmd_stop();      return _answer->createElement( "ok" ); }
    else
    if( element->tagName == "terminate_spooler" )  { _spooler->cmd_terminate(); return _answer->createElement( "ok" ); }
    else
    if( element->tagName == "restart_spooler"   )  { _spooler->cmd_restart();   return _answer->createElement( "ok" ); }
    else
*/
    if( element->tagName == "modify_spooler"    )  return execute_modify_spooler( element );
    else
    if( element->tagName == "modify_job"        )  return execute_modify_job( element );
    else
    {
        throw_xc( "SPOOLER-105", as_string( element->tagName ) ); return NULL;
    }
}

//------------------------------------------------------------------------Command_processor::execute

string Command_processor::execute( const string& xml_text )
{
    Thread_semaphore::Guard guard = &_spooler->_semaphore;

    _answer = xml::Document_ptr( __uuidof(xml::DOMDocument30), NULL );

    _answer->appendChild( _answer->createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"iso-8859-1\"" ) );

    xml::Element_ptr spooler_answer = _answer->appendChild( _answer->createElement( "spooler_answer" ) );

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

            if( e->tagName == "spooler_command" )
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
        append_error_element( spooler_answer, x );
    }

    _answer->save( "c:/tmp/~spooler.xml" );
    _answer = NULL;
    return file_as_string( "c:/tmp/~spooler.xml" );
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
