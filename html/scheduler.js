// $Id: scheduler.js,v 1.9 2004/07/23 17:53:39 jz Exp $

//----------------------------------------------------------------------------------------Scheduler
// public

function Scheduler()
{
    this._url           = "http://" + document.location.host + "/";
    this._xml_http      = new ActiveXObject( "Msxml2.XMLHTTP" );
    
    //this._configuration = new Scheduler_html_configuration( this._url + "config.xml" );
}

//----------------------------------------------------------------------------------Scheduler.close
// public

Scheduler.prototype.close = function()
{
    //this._configuration = null;
    this._xml_http = null;
}

//--------------------------------------------------------------------------------Scheduler.execute
// public

Scheduler.prototype.execute = function( xml )
{
    var response = this.call_http( xml );

    var dom_document = new ActiveXObject( "MSXML2.DOMDocument" );
    
    var ok = dom_document.loadXML( this._xml_http.responseText );
    if( !ok )  throw new Error( "Fehlerhafte XML-Antwort: " + dom_document.parseError.reason );
    
    var error_element = dom_document.selectSingleNode( "spooler/answer/ERROR" );
    if( error_element )
    {
        throw new Error( error_element.getAttribute( "text" ) );
    }
    
    return dom_document;
}

//------------------------------------------------------------------Scheduler.execute_and_fill_html
// public
/*
Scheduler.prototype.execute_and_fill_html = function( xml_query )
{
    var dom_document   = this.execute( xml_query );
    var answer_element = dom_document.selectSingleNode( "spooler/answer" );
    
    //dom_document.setProperty( "SelectionLanguage", "XPath" );
    if( !answer_element )  return;

    this.fill_html_state( document.all.scheduler_state, answer_element.selectSingleNode( "state" ) );
    this.fill_html_jobs ( document.all.scheduler_jobs , answer_element.selectSingleNode( "state/jobs" ) );
}
*/
//------------------------------------------------------------------------Scheduler.fill_html_state
/*
Scheduler.prototype.fill_html_state = function( html_element, state_response_element )
{
    if( html_element )
    {
        var htmls = new Array();
        var state_config_element = this._configuration._dom.selectSingleNode( "scheduler/html/state" );
        
        if( state_response_element && state_config_element )
        {
            var state_element = state_config_element.cloneNode( true );
            this.fill_spans( state_element, state_response_element );
            
            for( var child = state_element.firstChild; child; child = child.nextSibling )  htmls.push( child.xml );
        }
        
        html_element.innerHTML = htmls.join( "" );
    }
}
*/
//-------------------------------------------------------------------------Scheduler.fill_html_jobs
/*
Scheduler.prototype.fill_html_jobs = function( html_element, jobs_response_element )
{
    if( html_element )
    {
        var htmls = new Array();
        var job_config_element = this._configuration._dom.selectSingleNode( "scheduler/html/job" );
        
        if( jobs_response_element && job_config_element )
        {
            var job_response_elements = jobs_response_element.selectNodes( "job" );
            for( var i = 0; i < job_response_elements.length; i++ )
            {
                var job_response_element = job_response_elements[ i ];

                var job_element = job_config_element.cloneNode( true );
                
                var tasks_element = job_element.selectSingleNode( ".//scheduler.tasks" );
                if( tasks_element )
                {
                    this.fill_html_tasks( tasks_element, jobs_response_element.selectSingleNode( "tasks" ) );
                    tasks_element.nodeName = "span";
                }
                
                this.fill_spans( job_element, job_response_element );
                for( var child = job_element.firstChild; child; child = child.nextSibling )  htmls.push( child.xml );
            }
        }    

        html_element.innerHTML = htmls.join( "" );
    }
}
*/
//-------------------------------------------------------------------------Scheduler.fill_html_jobs
/*
Scheduler.prototype.make_html = function( html_element, response_element, config_element, sub_element_name )
{
    var result = null;
    
    if( html_element )
    {
        var htmls = new Array();
        
        if( response_element && config_element )
        {
            var sub_response_elements = response_element.selectNodes( sub_element_name );

            for( var i = 0; i < sub_response_elements.length; i++ )
            {
                var sub_response_element = sub_response_elements[ i ];

                var result = config_element.cloneNode( true );
                
                var tasks_element = job_element.selectSingleNode( ".//scheduler.tasks" );
                if( tasks_element )
                {
                    this.fill_html_tasks( tasks_element, jobs_response_element.selectSingleNode( "tasks" ) );
                    tasks_element.nodeName = "span";
                }
                
                this.fill_spans( job_element, job_response_element );
            }
        }    
    }

    return result;
}
*/
//-----------------------------------------------------------------------------Scheduler.fill_spans
/*
Scheduler.prototype.fill_spans = function( html_element, xml_element )
{
    var span_elements = html_element.getElementsByTagName( "span" );
    for( var i = 0; i < span_elements.length; i++ )
    {
        var span_element = span_elements[ i ];
        var text = span_element.text;
        if( text.substring( 0, 1 ) == "=" )
        {
            try
            {
                var found_element = xml_element.selectSingleNode( text.substring( 1, text.length ) );
                span_element.text = found_element? found_element.text : "";
            }
            catch( x ) { span_element.text = x.message; }
        }
    }
}
*/
//------------------------------------------------------------------------------Scheduler.call_http

Scheduler.prototype.call_http = function( text, debug_text )
{
    this._xml_http.open( "POST", this._url, false );
    this._xml_http.setRequestHeader( "Cache-Control", "no-cache" );

    var status = window.status;
    window.status = "Waiting for response from scheduler ...";//text;
    
    try
    {
        this._xml_http.send( text );
    }
    finally
    {
        window.status = status;
    }
}

//---------------------------------------------------------------------Scheduler_html_configuration
/*
function Scheduler_html_configuration( url )
{
    this._dom = new ActiveXObject( "MSXML2.DOMDocument" );

    this._dom.async = false;
    this._dom.validateOnParse = false;  // Wegen DOCTYPE in config.xml (aber warum?)
    var ok  = this._dom.load( url );
    if( !ok )  throw new Error( "Fehler in der Konfiguration " + url + ": " + this._dom.parseError.reason );
}
*/
//---------------------------------------------------------------------show_tasks_checkbox__onclick

function show_tasks_checkbox__onclick()
{
    window.parent.jobs_frame.reset_error();
    update();
}

//--------------------------------------------------------------------show_orders_checkbox__onclick

function show_orders_checkbox__onclick()
{
    window.parent.jobs_frame.reset_error();
    update();
}

//----------------------------------------------------------------------------------update__onclick

function update__onclick()
{
    window.parent.jobs_frame.reset_error();
    update();
}

//------------------------------------------------------------------------Popup_menu_builder.add_command
// Erweiterung von Popup_menu_builder, s. popup_builder.js

function Popup_menu_builder__add_command( html, xml_command, is_active )
{
    this.add_entry( html, "parent.popup_menu__execute( &quot;" + xml_command + "&quot; )", is_active );
}

//-------------------------------------------------------------------------------popup_menu.execute
// Für Popup_menu_builder.add_command()

function popup_menu__execute( xml_command )
{
    _popup.hide();
    
    try
    {
        _scheduler.execute( xml_command );
        window.parent.jobs_frame.update();
    }
    catch( x )
    {
        if( x.number + 0xFFFFFFFF == 0x800C0007 )
        {
            alert( "Scheduler connection closed" );
        }
        else
        {
            throw x;
            //alert( "Error 0x" + hex_string( x.number, 8 ) + ": " + x.message );
        }
    }
}

//-----------------------------------------------------------------------Popup_menu_builder.add_show_log
// Erweiterung von Popup_menu_builder, s. popup_builder.js

function Popup_menu_builder__add_show_log( html, show_log_command, window_name, is_active )
{
    this.add_entry( html, "parent.popup_menu__show_log__onclick( &quot;" + show_log_command + "&quot;, &quot;" + window_name + "&quot; )", is_active );
}

//--------------------------------------------------------------------popup_menu__show_log__onclick
// Für Popup_menu_builder.add_show_log()

function popup_menu__show_log__onclick( show_log_command, window_name )
{
    window_name = window_name.replace( /[~a-zA-Z0-9]/g, "_" );
    
    var log_window = window.open( "http://" + document.location.host + "/" + show_log_command, window_name, "menubar=no, toolbar=no, location=no, directories=no, scrollbars=yes, resizable=yes, status=no", true );
    log_window.focus();
    _popup.hide();
}

//--------------------------------------------------------------------------scheduler_menu__onclick

function scheduler_menu__onclick()
{
    var popup_builder = new Popup_menu_builder();
    
    var state = _response.selectSingleNode( "spooler/answer/state" ).getAttribute( "state" );
    
    var command = function( cmd ) { return "<modify_spooler cmd='" + cmd + "'/>"; }
    
    popup_builder.add_show_log( "Show log"                       , "show_log", "show_log" );
    popup_builder.add_bar();
  //popup_builder.add_command ( "Stop"                           , command( "stop" ), state != "stopped"  &&  state != "stopping"  &&  state != "stopping_let_run" );
    popup_builder.add_command ( "Pause"                          , command( "pause"                         ), state != "paused" );
    popup_builder.add_command ( "Continue"                       , command( "continue"                      ), state == "paused" );
    popup_builder.add_bar();
  //popup_builder.add_command ( "Reload"                         , command( "reload"                        ), state != "stopped"  &&  state != "stopping" );
    popup_builder.add_command ( "Terminate"                      , command( "terminate"                     ) );
    popup_builder.add_command ( "Terminate and restart"          , command( "terminate_and_restart"         ) );
    popup_builder.add_command ( "Let run, terminate and restart" , command( "let_run_terminate_and_restart" ) );
    popup_builder.add_bar();
    popup_builder.add_command ( "Abort immediately"              , command( "abort_immediately"             ) );
    popup_builder.add_command ( "Abort immediately and restart"  , command( "abort_immediately_and_restart" ) );
    
    _popup = popup_builder.show_popup();
}

//--------------------------------------------------------------------------------job_menu__onclick

function job_menu__onclick( job_name )
{
    var popup_builder = new Popup_menu_builder();
    
    var state = _job_element.getAttribute( "state" );
    
    popup_builder.add_show_log( "Show log"      , "show_log&job=" + job_name, "show_log_job_" + job_name );
    popup_builder.add_bar();
    popup_builder.add_command ( "Start task now", "parent.task_menu__start_task_now__onclick('" + job_name + "')" );
    popup_builder.add_command ( "Stop"          , "<modify_job job='" + job_name + "' cmd='stop'    />", state != "stopped"  &&  state != "stopping" );
    popup_builder.add_command ( "Unstop"        , "<modify_job job='" + job_name + "' cmd='unstop'  />", state == "stopped"  ||  state == "stopping" );
    popup_builder.add_command ( "Wake"          , "<modify_job job='" + job_name + "' cmd='wake'    />" );
    popup_builder.add_command ( "Start"         , "<modify_job job='" + job_name + "' cmd='start'   />" );
    popup_builder.add_command ( "Reread"        , "<modify_job job='" + job_name + "' cmd='reread'  />" );
    popup_builder.add_bar();
    popup_builder.add_command ( "End tasks"     , "<modify_job job='" + job_name + "' cmd='end'     />" );
    popup_builder.add_command ( "Suspend tasks" , "<modify_job job='" + job_name + "' cmd='suspend' />" );
    popup_builder.add_command ( "Continue tasks", "<modify_job job='" + job_name + "' cmd='continue'/>" );
    
    _popup = popup_builder.show_popup();
}

//-------------------------------------------------------------------------------task_menu__onclick

function task_menu__onclick( task_id )
{
    var popup_builder = new Popup_menu_builder();

    popup_builder.add_show_log( "Show log"        , "show_log&task=" + task_id, "show_log_task_" + task_id );
    popup_builder.add_bar();
    popup_builder.add_command ( "End"             , "<kill_task job='" + _job_name + "' id='" + task_id + "'/>" );
    popup_builder.add_command ( "Kill immediately", "<kill_task job='" + _job_name + "' id='" + task_id + "' immediately='yes'/>" );
    
    _popup = popup_builder.show_popup();
}

//-------------------------------------------------------------------------------task_menu__onclick

function task_menu__onclick( task_id )
{
    var popup_builder = new Popup_menu_builder();

    popup_builder.add_show_log( "Show log"        , "show_log&task=" + task_id, "show_log_task_" + task_id );
    popup_builder.add_bar();
    popup_builder.add_command ( "End"             , "<kill_task job='" + _job_name + "' id='" + task_id + "'/>" );
    popup_builder.add_command ( "Kill immediately", "<kill_task job='" + _job_name + "' id='" + task_id + "' immediately='yes'/>" );
    
    _popup = popup_builder.show_popup();
}

//------------------------------------------------------------------------------order_menu__onclick

function order_menu__onclick( job_chain_name, order_id )
{
    var popup_builder = new Popup_menu_builder();

    popup_builder.add_show_log( "Show log"        , "show_log&job_chain=" + job_chain_name + 
                                                            "&order=" + order_id, "show_log_order_" + job_chain_name + "__" + order_id );
    
    _popup = popup_builder.show_popup();
}

//-------------------------------------------------------------------------------string_from_object

function string_from_object( object )
{
    var result = "{";
    for( var i in object )  result += i + "=" + object[ i ] + " ";
    return result + "}";
}

//--------------------------------------------------------------------------------------hex_string

function hex_string( value, min_length )
{
    var result = "";
    var hex    = "0123456789ABCDEF";

    if( min_length == undefined )  min_length = 1

    do
    {
        var digit = value % 16;
        if( digit < 0 )  digit += 15;
        result = hex.substring( digit, digit + 1 ) + result;
        value >>>= 4;
    }
    while( value != 0  ||  result.length < min_length );

    return result;
}

//---------------------------------------------------------------------------------------xml_encode

function xml_encode( text )
{
    if( text == null )  return "";
    return text.toString().replace( /&/g, "&amp;" ).replace( /</g, "&lt;" ).replace( />/g, "&gt;" );
    //TODO Reguläre Ausdrücke vorkompilieren
}

//-----------------------------------------------------------------------------------scheduler_init

function scheduler_init()
{
    Popup_menu_builder.prototype.add_command  = Popup_menu_builder__add_command;
    Popup_menu_builder.prototype.add_show_log = Popup_menu_builder__add_show_log;
}

//-------------------------------------------------------------------------------------------------

/* Fehlercodes von xmlhttp:
#define DE_E_INVALID_URL               0x800C0002
#define DE_E_NO_SESSION                0x800C0003
#define DE_E_CANNOT_CONNECT            0x800C0004
#define DE_E_RESOURCE_NOT_FOUND        0x800C0005
#define DE_E_OBJECT_NOT_FOUND          0x800C0006
#define DE_E_DATA_NOT_AVAILABLE        0x800C0007
#define DE_E_DOWNLOAD_FAILURE          0x800C0008
#define DE_E_AUTHENTICATION_REQUIRED   0x800C0009
#define DE_E_NO_VALID_MEDIA            0x800C000A
#define DE_E_CONNECTION_TIMEOUT        0x800C000B
#define DE_E_INVALID_REQUEST           0x800C000C
#define DE_E_UNKNOWN_PROTOCOL          0x800C000D
#define DE_E_SECURITY_PROBLEM          0x800C000E
#define DE_E_CANNOT_LOAD_DATA          0x800C000F
#define DE_E_CANNOT_INSTANTIATE_OBJECT 0x800C0010
#define DE_E_REDIRECT_FAILED           0x800C0014
#define DE_E_REDIRECT_TO_DIR           0x800C0015
#define DE_E_CANNOT_LOCK_REQUEST       0x800C0016
*/
