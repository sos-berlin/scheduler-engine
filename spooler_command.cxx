// $Id: spooler_command.cxx,v 1.29 2001/02/16 18:23:12 jz Exp $
/*
    Hier ist implementiert

    Command_processor
*/


#include "../kram/sos.h"
#include "spooler.h"

#include "../kram/sleep.h"
#include "../file/anyfile.h"



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
//------------------------------------------------------------------------------------dom_append_nl

void dom_append_nl( const xml::Element_ptr& element )
{
    xml::Document_ptr doc       = element->ownerDocument;
    xml::Node_ptr     text_node = doc->createTextNode( as_dom_string( "\n" ) );

    element->appendChild( text_node );
}

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

void append_error_element( const xml::Element_ptr& element, const Xc_copy& x )
{
    element->appendChild( create_error_element( element->ownerDocument, x ) );
}

//----------------------------------------------------------Command_processor::execute_show_threads

xml::Element_ptr Command_processor::execute_show_threads()
{
    if( _security_level < Security::seclev_info )  throw_xc( "SPOOLER-121" );

    xml::Element_ptr threads = _answer->createElement( "threads" );

    dom_append_nl( threads );

    FOR_EACH( Thread_list, _spooler->_thread_list, it )
    {
        threads->appendChild( (*it)->xml( _answer ) );
        dom_append_nl( threads );
    }

    return threads;
}

//------------------------------------------------------------Command_processor::execute_show_state

xml::Element_ptr Command_processor::execute_show_state()
{
    if( _security_level < Security::seclev_info )  throw_xc( "SPOOLER-121" );

    xml::Element_ptr state_element = _answer->createElement( "state" );
 
    state_element->setAttribute( "time"                 , as_dom_string( Sos_optional_date_time::now().as_string() ) );
    state_element->setAttribute( "spooler_running_since", as_dom_string( Sos_optional_date_time( _spooler->_spooler_start_time ).as_string() ) );

    state_element->setAttribute( "log_file"             , as_dom_string( _spooler->_log.filename() ) );

    double cpu_time = get_cpu_time();
    char buffer [30];
    sprintf( buffer, "%-0.3lf", cpu_time ); 
    state_element->setAttribute( "cpu_time"             , as_dom_string( buffer ) );

    state_element->appendChild( execute_show_threads() );

    return state_element;
}

//-----------------------------------------------------------Command_processor::execute_show_config
/*
xml::Element_ptr Command_processor::execute_show_config()
{
    if( _security_level < Security::seclev_info )  throw_xc( "SPOOLER-121" );

    xml::Element_ptr config_element = _answer->createElement( "config" );
    xml::Element_ptr run_
 
    return config_element;
}
*/
//--------------------------------------------------------Command_processor::execute_modify_spooler

xml::Element_ptr Command_processor::execute_modify_spooler( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_all )  throw_xc( "SPOOLER-121" );

    string cmd = as_string( element->getAttribute( "cmd" ) );
  //if( !cmd.empty() )
    {
        if( cmd == "pause"                 )  _spooler->cmd_pause();
        else
        if( cmd == "continue"              )  _spooler->cmd_continue();
        else
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
        if( cmd == "abort_immediately_and_restart" )  { try{ spooler_restart( _spooler->_is_service ); }catch(...){}; _exit(1); }
        else
      //if( cmd == "new_log"               )  _spooler->cmd_new_log();
      //else
            throw_xc( "SPOOLER-105", cmd );
    }
    
    return _answer->createElement( "ok" );
}

//------------------------------------------------------------Command_processor::execute_modify_job

xml::Element_ptr Command_processor::execute_modify_job( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_all )  throw_xc( "SPOOLER-121" );

    string job_name = as_string( element->getAttribute( "job" ) );
    string cmd_name = as_string( element->getAttribute( "cmd" ) );

    Job::State_cmd cmd = cmd_name.empty()? Job::sc_none 
                                         : Job::as_state_cmd( cmd_name );

    xml::Element_ptr jobs_element = _answer->createElement( "jobs" );
    bool found = false;

    Job* job = _spooler->get_job( job_name );

    if( cmd )  job->set_state_cmd( cmd );
    
    return jobs_element;
}

//---------------------------------------------------------Command_processor::execute_signal_object

xml::Element_ptr Command_processor::execute_signal_object( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_signal )  throw_xc( "SPOOLER-121" );

    string class_name = as_string( element->getAttribute( "class" ) );
    Level  level      = as_int( element->getAttribute( "level" ) );

    xml::Element_ptr jobs_element = _answer->createElement( "tasks" );

    _spooler->signal_object( class_name, level );
    
    return _answer->createElement( "ok" );
}

//----------------------------------------------------------------Command_processor::execute_config

xml::Element_ptr Command_processor::execute_config( const xml::Element_ptr& config_element )
{
    if( _security_level < Security::seclev_all )  throw_xc( "SPOOLER-121" );

    if( config_element->tagName != "config" )  throw_xc( "SPOOLER-113", as_string( config_element->tagName ) );

    string spooler_id = as_string( config_element->getAttribute( "spooler_id" ) );
    if( spooler_id.empty()  ||  spooler_id == _spooler->_spooler_id )
    {
        _spooler->cmd_load_config( config_element );
    }

    return _answer->createElement( "ok" );
}

//---------------------------------------------------------------Command_processor::execute_command

xml::Element_ptr Command_processor::execute_command( const xml::Element_ptr& element )
{
    if( element->tagName == "show_state"        )  return execute_show_state();
    else
    if( element->tagName == "modify_spooler"    )  return execute_modify_spooler( element );
    else
    if( element->tagName == "modify_job"        )  return execute_modify_job( element );
    else
    if( element->tagName == "signal_object"     )  return execute_signal_object( element );
    else
    if( element->tagName == "config"            )  return execute_config( element );
    else
    {
        throw_xc( "SPOOLER-105", as_string( element->tagName ) ); return NULL;
    }
}

//------------------------------------------------------------------------Command_processor::execute

string Command_processor::execute( const string& xml_text )
{
    try 
    {
        _error = NULL;
        execute_2( xml_text );
    }
    catch( const Xc& x )
    {
        _error = x;
        append_error_element( _answer->documentElement->firstChild, x );
    }

    return _answer->xml;

/*  Bei save wird die encoding belassen. Eigenschaft xml verwendet stets unicode, was wir nicht wollen.
    _answer->save( "c:/tmp/~spooler.xml" );
    _answer = NULL;
    return file_as_string( "c:/tmp/~spooler.xml" );
*/
}

//----------------------------------------------------------------------Command_processor::execute_2

void Command_processor::execute_2( const string& xml_text )
{
    try 
    {
        _answer = xml::Document_ptr( __uuidof(xml::DOMDocument30), NULL );
        _answer->appendChild( _answer->createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"iso-8859-1\"" ) );
        _answer->appendChild( _answer->createElement( "spooler" ) );

        xml::Element_ptr answer_element = _answer->documentElement->appendChild( _answer->createElement( "answer" ) );

        _security_level = _host? _spooler->_security.level( *_host ) 
                               : Security::seclev_all;

        xml::Document_ptr command_doc ( __uuidof(xml::DOMDocument30), NULL );

        int ok = command_doc->loadXML( as_dom_string( xml_text ) );
        if( !ok )
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

        if( e->tagName == "spooler" )  e = e->firstChild;

        if( e )
        {
            if( e->tagName == "command" )
            {
                xml::NodeList_ptr node_list = e->childNodes;

                for( int i = 0; i < node_list->length; i++ )
                {
                    xml::Node_ptr node = node_list->Getitem(i);

                    answer_element->appendChild( execute_command( node ) );
                }
            }
            else
            {
                answer_element->appendChild( execute_command( e ) );
            }
        }
    }
    catch( const _com_error& com_error ) { throw_com_error(com_error);  }
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
