// $Id: spooler_command.cxx,v 1.11 2001/01/11 21:39:42 jz Exp $

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

    e->setAttribute( "time", as_dom_string( Sos_optional_date_time( x.time() - _timezone ).as_string() ) );

    if( !empty( x->name() )          )  e->setAttribute( "class" , as_dom_string( x->name()          ) );

    e->setAttribute( "code", as_dom_string( x->code() ) );
    e->setAttribute( "text", as_dom_string( x->what() ) );
    
    if( !empty( x->_pos.filename() ) )  e->setAttribute( "source", as_dom_string( x->_pos.filename() ) );
    if( x->_pos._line >= 0           )  e->setAttribute( "line"  , as_dom_string( x->_pos._line + 1  ) );
    if( x->_pos._col  >= 0           )  e->setAttribute( "col"   , as_dom_string( x->_pos._col + 1   ) );

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
    task_element->setAttribute( "state", as_dom_string( task->state_name() ) );

    if( task->_state_cmd )
        task_element->setAttribute( "cmd", as_dom_string( task->state_cmd_name() ) );

    if( task->_state == Task::s_pending )
        task_element->setAttribute( "next_start_time", as_dom_string( Sos_optional_date_time( task->_next_start_time ).as_string() ) );

    if( task->_state == Task::s_running )
        task_element->setAttribute( "running_since", as_dom_string( Sos_optional_date_time( task->_running_since ).as_string() ) );

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
 
    state_element->setAttribute( "time"                 , as_dom_string( Sos_optional_date_time::now().as_string() ) );
    state_element->setAttribute( "spooler_running_since", as_dom_string( Sos_optional_date_time( _spooler->_spooler_start_time ).as_string() ) );
    state_element->setAttribute( "sleeping_until"       , as_dom_string( Sos_optional_date_time( _spooler->_next_start_time ).as_string() ) );
    state_element->setAttribute( "tasks"                , as_dom_string( _spooler->_task_count ) );
    state_element->setAttribute( "steps"                , as_dom_string( _spooler->_step_count ) );
    state_element->setAttribute( "log_file"             , as_dom_string( _spooler->_log.filename() ) );

#   ifdef SYSTEM_WIN    
        int64 CreationTime; // process creation time
        int64 ExitTime;     // process exit time
        int64 KernelTime;   // process kernel-mode time
        int64 UserTime;     // process user-mode time

        BOOL ok = GetProcessTimes( GetCurrentProcess(), (LPFILETIME)&CreationTime, (LPFILETIME)&ExitTime, 
                                                        (LPFILETIME)&KernelTime, (LPFILETIME)&UserTime );
        if( ok )
        {
            double cpu_time = ( KernelTime + UserTime ) / 1e7;
            char   buffer [30];
            sprintf( buffer, "%-0.3lf", cpu_time ); 
            state_element->setAttribute( "cpu_time"     , as_dom_string( buffer ) );
        }
#   endif

    state_element->appendChild( execute_show_tasks() );

    return state_element;
}

//--------------------------------------------------------Command_processor::execute_modify_spooler

xml::Element_ptr Command_processor::execute_modify_spooler( const xml::Element_ptr& element )
{
    string cmd = as_string( element->getAttribute( "cmd" ) );
  //if( !cmd.empty() )
    {
        if( cmd == "stop"                  )  _spooler->cmd_stop();
        else
        if( cmd == "reload"                )  _spooler->cmd_reload();
        else
        if( cmd == "terminate"             )  _spooler->cmd_terminate();
        else
        if( cmd == "terminate_and_restart" )  _spooler->cmd_terminate_and_restart();
        else
        if( cmd == "abort_immediately"     )  _exit(1);
        else
      //if( cmd == "new_log"               )  _spooler->cmd_new_log();
      //else
            throw_xc( "SPOOLER-106", cmd );
    }
    
    return _answer->createElement( "ok" );
}

//------------------------------------------------------------Command_processor::execute_modify_job

xml::Element_ptr Command_processor::execute_modify_job( const xml::Element_ptr& element )
{
    string job_name     = as_string( element->getAttribute( "job" ) );
    string action_name  = as_string( element->getAttribute( "cmd" ) );
    string state_name   = as_string( element->getAttribute( "state" ) );

    Task::State_cmd cmd = action_name.empty()? Task::sc_none 
                                             : Task::as_state_cmd( action_name );

    Task::State state = state_name.empty()? Task::s_none 
                                          : Task::as_state( state_name );

    xml::Element_ptr tasks_element = _answer->createElement( "tasks" );
    bool found = false;

    FOR_EACH( Task_list, _spooler->_task_list, it )
    {
        Task* task = *it;
        if( task->_job->_name == job_name ) 
        {
            found = true;

            if( cmd )  task->set_state_cmd( cmd );
            else
            if( state )  task->set_state( state );      // experimental

            tasks_element->appendChild( execute_show_task( *it ) );
        }
    }
    
    if( !found )  throw_xc( "SPOOLER-108", job_name );

    return tasks_element;
}

//---------------------------------------------------------Command_processor::execute_signal_object

xml::Element_ptr Command_processor::execute_signal_object( const xml::Element_ptr& element )
{
    string class_name = as_string( element->getAttribute( "class" ) );
    Level  level      = as_int( element->getAttribute( "level" ) );

    xml::Element_ptr tasks_element = _answer->createElement( "tasks" );

    FOR_EACH( Task_list, _spooler->_task_list, it )
    {
        Task* task = *it;
        
        if( task->_state == Task::s_pending
         && task->_job->_object_set_descr
         && task->_job->_object_set_descr->_class->_name == class_name 
         && task->_job->_object_set_descr->_level_interval.is_in_interval( level ) )
        {
            task->set_state_cmd( Task::sc_start );
            tasks_element->appendChild( execute_show_task( *it ) );
        }
    }

    return tasks_element;
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
    if( element->tagName == "signal_object"     )  return execute_signal_object( element );
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

    string result = _answer->xml;
    _answer = NULL;
    return result;

/*  Bei save wird die encoding belassen. Eigenschaft xml verwendet stets unicode, was wir nicht wollen.
    _answer->save( "c:/tmp/~spooler.xml" );
    _answer = NULL;
    return file_as_string( "c:/tmp/~spooler.xml" );
*/
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
