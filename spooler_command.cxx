// $Id: spooler_command.cxx,v 1.50 2002/04/11 13:35:03 jz Exp $
/*
    Hier ist implementiert

    Command_processor
*/


#include "../kram/sos.h"
#include "spooler.h"

#include "../file/anyfile.h"


// Für temporäre Datei:

#include <sys/stat.h>               // S_IREAD, stat()
#include <fcntl.h>                  // O_RDONLY

#if defined SYSTEM_WIN
#   include <io.h>                  // open(), read() etc.
#   include <share.h>
#   include <direct.h>              // mkdir
#   include <windows.h>
#else
#   include <stdio.h>               // fileno
#   include <unistd.h>              // read(), write(), close()
#endif

#include <sys/types.h>
#include <sys/timeb.h>


namespace sos {
namespace spooler {

using namespace std;

//--------------------------------------------------------------------------dom_append_text_element

void dom_append_text_element( const xml::Element_ptr& element, const char* element_name, const string& text )
{
    xml::Document_ptr doc       = element->ownerDocument;
    xml::Node_ptr     text_node = doc->createTextNode( as_dom_string( text ) );
    xml::Element_ptr  e         = element->appendChild( doc->createElement( element_name ) );

    e->appendChild( text_node );
}

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

    _timeb  tm;     // Ob die Sommerzeitverschiebung bei der Fehlerzeit berücksichtigt wird, hängt von der _aktuellen_ Zeit ab.
    _ftime( &tm );  // Nicht schön, aber es funktioniert, weil der Spooler sowieso nicht während der Zeitumstellung laufen soll.
    e->setAttribute( "time", as_dom_string( Sos_optional_date_time( x.time() - _timezone - ( tm.dstflag? _dstbias : 0 ) ).as_string() ) );

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

xml::Element_ptr Command_processor::execute_show_threads( bool show_all )
{
    if( _security_level < Security::seclev_info )  throw_xc( "SPOOLER-121" );

    return _spooler->threads_as_xml( _answer, show_all );
}

//------------------------------------------------------------Command_processor::execute_show_state

xml::Element_ptr Command_processor::execute_show_state( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_info )  throw_xc( "SPOOLER-121" );

    string what = as_string( element->getAttribute( "what" ) );
    bool show_all = what == "all";

    xml::Element_ptr state_element = _answer->createElement( "state" );
 
    state_element->setAttribute( "time"                 , as_dom_string( Sos_optional_date_time::now().as_string() ) );
    state_element->setAttribute( "spooler_running_since", as_dom_string( Sos_optional_date_time( _spooler->start_time() ).as_string() ) );
    state_element->setAttribute( "state"                , as_dom_string( _spooler->state_name() ) );
    state_element->setAttribute( "log_file"             , as_dom_string( _spooler->_log.filename() ) );
    state_element->setAttribute( "db"                   , as_dom_string( trim( _spooler->_db.db_name() ) ) );

    double cpu_time = get_cpu_time();
    char buffer [30];
    sprintf( buffer, "%-0.3lf", cpu_time ); 
    state_element->setAttribute( "cpu_time"             , as_dom_string( buffer ) );

    state_element->appendChild( execute_show_threads( show_all ) );

    return state_element;
}

//----------------------------------------------------------Command_processor::execute_show_history

xml::Element_ptr Command_processor::execute_show_history( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_info )  throw_xc( "SPOOLER-121" );

    string job_name = as_string( element->getAttribute( "job" ) );
    
    string id_str = as_string( element->getAttribute( "id" ) );
    int id = id_str != ""? as_uint(id_str) : -1;

    string prev_str = as_string( element->getAttribute( "prev" ) );
    int    next     = prev_str == ""   ? ( id == -1? -10 : 0 ) :
                      prev_str == "all"? -INT_MAX 
                                       : -as_int(prev_str);
    
    string next_str = as_string( element->getAttribute( "next" ) );
    if( next_str != "" )  next = as_uint(next_str);

    string what = as_string( element->getAttribute( "what" ) );
    bool show_all = what == "all";

    Sos_ptr<Job> job = _spooler->get_job( job_name );

    return job->read_history( _answer, id, next, show_all );
}

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
        if( cmd == "let_run_terminate_and_restart" )  _spooler->cmd_let_run_terminate_and_restart();
        else
        if( cmd == "abort_immediately"     )  TerminateProcess(GetCurrentProcess(),1);  // _exit() lässt noch Delphi-Code ausführen.
        else
        if( cmd == "abort_immediately_and_restart" )  { try{ spooler_restart( _spooler->is_service() ); }catch(...){}; TerminateProcess(GetCurrentProcess(),1); }
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

    Job* job = _spooler->get_job( job_name );

    if( cmd )  job->set_state_cmd( cmd );
    
    return jobs_element;
}

//-------------------------------------------------------------Command_processor::execute_kill_task

xml::Element_ptr Command_processor::execute_kill_task( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_all )  throw_xc( "SPOOLER-121" );

    int    id       = as_int( as_string( element->getAttribute( "id" ) ) );
    string job_name = as_string( element->getAttribute( "job" ) );              // Hilfsweise

    _spooler->get_job( job_name )->kill_task( id );
    
    return _answer->createElement( "ok" );
}

//-------------------------------------------------------------Command_processor::execute_start_job

xml::Element_ptr Command_processor::execute_start_job( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_all )  throw_xc( "SPOOLER-121" );

    string job_name        = as_string( element->getAttribute( "job"   ) );
    string task_name       = as_string( element->getAttribute( "name"  ) );
    string after_str       = as_string( element->getAttribute( "after" ) );
    string at_str          = as_string( element->getAttribute( "at"    ) );

    Time start_at;

    if( !after_str.empty() )  start_at = Time::now() + as_int( after_str );

    if( at_str == ""       )  at_str = "now";
    if( at_str == "period" )  start_at = 0;                                     // start="period" => start_at = 0 (sobald eine Periode es zulässt)
                        else  start_at = (Sos_optional_date_time) at_str;       // 

    CComPtr<Com_variable_set> pars = new Com_variable_set;

    for( xml::Element_ptr e = element->firstChild; e; e = e->nextSibling )
    {
        if( e->tagName == "params" )  { pars->set_xml( e );  break; }
    }

    Sos_ptr<Task> task = _spooler->get_job( job_name )->start_without_lock( CComPtr<spooler_com::Ivariable_set>(pars), task_name, start_at );

    return _answer->createElement( "ok" );
}

//---------------------------------------------------------Command_processor::execute_signal_object

xml::Element_ptr Command_processor::execute_signal_object( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_signal )  throw_xc( "SPOOLER-121" );

    string class_name = as_string( element->getAttribute( "class" ) );
    Level  level      = int_from_variant( element->getAttribute( "level" ) );

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
    if( spooler_id.empty()  ||  spooler_id == _spooler->id() )
    {
        _spooler->cmd_load_config( config_element );
    }

    return _answer->createElement( "ok" );
}

//--------------------------------------------------------------Command_processor::execute_add_jobs

xml::Element_ptr Command_processor::execute_add_jobs( const xml::Element_ptr& add_jobs_element )
{
    if( _security_level < Security::seclev_all )  throw_xc( "SPOOLER-121" );

    Sos_ptr<Thread> thread = _spooler->get_thread( as_string( add_jobs_element->getAttribute( "thread" ) ) );
    thread->cmd_add_jobs( add_jobs_element );

    return _answer->createElement( "ok" );
}

//---------------------------------------------------------------Command_processor::execute_command

xml::Element_ptr Command_processor::execute_command( const xml::Element_ptr& element )
{
    if( element->tagName == "show_state"        )  return execute_show_state( element );
    else
    if( element->tagName == "show_history"      )  return execute_show_history( element );
    else
    if( element->tagName == "modify_spooler"    )  return execute_modify_spooler( element );
    else
    if( element->tagName == "modify_job"        )  return execute_modify_job( element );
    else
    if( element->tagName == "start_job"         )  return execute_start_job( element );
    else
    if( element->tagName == "kill_task"         )  return execute_kill_task( element );
    else
    if( element->tagName == "signal_object"     )  return execute_signal_object( element );
    else
    if( element->tagName == "config"            )  return execute_config( element );
    else
    if( element->tagName == "add_jobs"          )  return execute_add_jobs( element );
    else
    {
        throw_xc( "SPOOLER-105", as_string( element->tagName ) ); return NULL;
    }
}

//------------------------------------------------------------------------------------xml_as_string

string xml_as_string( const xml::Document_ptr& document )
{
    char   tmp_filename [MAX_PATH];
    int    ret;
    string result;

    ret = GetTempPath( sizeof tmp_filename, tmp_filename );
    if( ret == 0 )  throw_mswin_error( "GetTempPath" );

    ret = GetTempFileName( tmp_filename, "sos", 0, tmp_filename );
    if( ret == 0 )  throw_mswin_error( "GetTempPath" );

    LOG( "Temporäre Datei " << tmp_filename << " für XML-Antwort\n" );

    try 
    {
        document->save( tmp_filename );
        result = file_as_string( tmp_filename );
        unlink( tmp_filename );
    }
    catch( const Xc&         ) { unlink( tmp_filename ); result = "<?xml version=\"1.0\"?><ERROR/>"; }
    catch( const _com_error& ) { unlink( tmp_filename ); result = "<?xml version=\"1.0\"?><ERROR/>"; }

    return result;
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

  //return _answer->xml;  //Bei save wird die encoding belassen. Eigenschaft xml verwendet stets unicode, was wir nicht wollen.
    
    return xml_as_string( _answer );
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

        _security_level = _host? _spooler->security_level( *_host ) 
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
/*
        xml::DocumentType_ptr doctype = command_doc->doctype;
        if( doctype )  command_doc->removeChild( doctype );

        doctype = command_doc->createDoc
*/
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
    catch( const _com_error& com_error ) { throw_com_error( com_error, "DOM/XML" ); }
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
