// $Id: scheduler.js,v 1.9 2004/12/10 11:20:40 jz Exp $

//----------------------------------------------------------------------------------------------var

var _popup_menu;

// Die Variablen enthalten die Versionnummer (numerisch, z.B. 5.5) des Browsers, oder 0.
var ie       = 0;   // Microsoft Internet Explorer
var netscape = 0;   // Netscape
var firefox  = 0;   // Mozilla Firefox
var opera    = 0;   // Opera

//--------------------------------------------------------------------------------------------const

var NODE_ELEMENT = 1;

// Fehlercodes von xmlhttp:
var DE_E_INVALID_URL               = 0x800C0002 - 0xFFFFFFFF;
var DE_E_NO_SESSION                = 0x800C0003 - 0xFFFFFFFF;
var DE_E_CANNOT_CONNECT            = 0x800C0004 - 0xFFFFFFFF;
var DE_E_RESOURCE_NOT_FOUND        = 0x800C0005 - 0xFFFFFFFF;
var DE_E_OBJECT_NOT_FOUND          = 0x800C0006 - 0xFFFFFFFF;
var DE_E_DATA_NOT_AVAILABLE        = 0x800C0007 - 0xFFFFFFFF;
var DE_E_DOWNLOAD_FAILURE          = 0x800C0008 - 0xFFFFFFFF;
var DE_E_AUTHENTICATION_REQUIRED   = 0x800C0009 - 0xFFFFFFFF;
var DE_E_NO_VALID_MEDIA            = 0x800C000A - 0xFFFFFFFF;
var DE_E_CONNECTION_TIMEOUT        = 0x800C000B - 0xFFFFFFFF;
var DE_E_INVALID_REQUEST           = 0x800C000C - 0xFFFFFFFF;
var DE_E_UNKNOWN_PROTOCOL          = 0x800C000D - 0xFFFFFFFF;
var DE_E_SECURITY_PROBLEM          = 0x800C000E - 0xFFFFFFFF;
var DE_E_CANNOT_LOAD_DATA          = 0x800C000F - 0xFFFFFFFF;
var DE_E_CANNOT_INSTANTIATE_OBJECT = 0x800C0010 - 0xFFFFFFFF;
var DE_E_REDIRECT_FAILED           = 0x800C0014 - 0xFFFFFFFF;
var DE_E_REDIRECT_TO_DIR           = 0x800C0015 - 0xFFFFFFFF;
var DE_E_CANNOT_LOCK_REQUEST       = 0x800C0016 - 0xFFFFFFFF;

//------------------------------------------------------------------------------------check_browser

function check_browser()
{
    if( window != undefined )
    {
        if( window.navigator != undefined  &&  window.navigator.appName )
        {
            if( window.navigator.appName == "Microsoft Internet Explorer" )
            {
                //if( !window.clientInformation )       Könnte Opera sein, oder auch nicht.
                //{
                //    opera = 7;  // Vermutlich Opera, der sich als ie ausgibt. Aber welche Version von Opera ist das?
                //}
                //else
                {
                    var match = window.navigator.appVersion.match( /MSIE (\d+\.\d+);/ );
                    if( match )  ie = 1 * RegExp.$1;
                }
            }
            else
            if( window.navigator.appName == "Netscape" )
            {
                if( window.navigator.vendor == "Firefox" )
                {
                    if( window.navigator.productSub >= 20041122 )  firefox = 1.0;
                }
                else
                {
                    var match = window.navigator.appVersion.match( /^(\d+\.\d+) / );
                    if( match )  netscape = 1 * RegExp.$1;
                }
            }
        }
    }

    if( ie < 6.0  &&  firefox < 1.0 )
    {
        var msg = "The page may not work with this browser, which doesn't seem to be one of the following:\n" +
                  "Microsoft Internet Explorer 6\n" +
                  "Mozilla Firefox 1";

        if( window.navigator != undefined )
        {
            msg += "\n\n\n";
            msg += "appName="         + window.navigator.appName            + "\n";
            msg += "appVersion="      + window.navigator.appVersion         + "\n";
            msg += "appMinorVersion=" + window.navigator.appMinorVersion    + "\n";
            msg += "vendor="          + window.navigator.vendor             + "\n";
            msg += "product="         + window.navigator.product            + "\n";
            msg += "productSub="      + window.navigator.productSub         + "\n";
        }

        alert( msg );
    }
}

//----------------------------------------------------------------------------------------Scheduler
// public

function Scheduler()
{
    this._url               = "http://" + document.location.host + "/";
    this._xml_http          = window.XMLHttpRequest? new XMLHttpRequest() : new ActiveXObject( "Msxml2.XMLHTTP" );
    this._dependend_windows = new Object();
    //this._configuration = new Scheduler_html_configuration( this._url + "config.xml" );
}

//----------------------------------------------------------------------------------Scheduler.close
// public

Scheduler.prototype.close = function()
{
    //this._configuration = null;
    this._xml_http = null;

    
    var dependend_windows = this._dependend_windows;
    this._depended_windows = new Object();

    for( var window_name in dependend_windows )  this._dependend_windows[ window_name ].close();
}

//--------------------------------------------------------------------------------Scheduler.execute
// public

Scheduler.prototype.execute = function( xml )
{
    this.call_http( xml );

    var dom_document;

    if( window.DOMParser )
    {
        var dom_parser = new DOMParser();
        dom_document = dom_parser.parseFromString( this._xml_http.responseText, "text/xml" );
        if( dom_document.documentElement.nodeName == "parsererror" )  throw new Error( "Fehler in der XML-Antwort: " + dom_document.documentElement.firstChild.nodeValue );
    }
    else
    {
        dom_document = new ActiveXObject( "MSXML2.DOMDocument" );
        var ok = dom_document.loadXML( this._xml_http.responseText );
        if( !ok )  throw new Error( "Fehlerhafte XML-Antwort: " + dom_document.parseError.reason );
    }

    var error_element = dom_document.selectSingleNode( "spooler/answer/ERROR" );
    if( error_element )
    {
        throw new Error( error_element.getAttribute( "text" ) );
    }

    this.modify_datetime_for_xslt( dom_document );

    return dom_document;
}

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
    catch( x )
    {
        if(1)
        //if( x.number == DE_E_CANNOT_CONNECT
        // || x.number == DE_E_DATA_NOT_AVAILABLE
        // || x.number == DE_E_RESOURCE_NOT_FOUND )
        {
            x.message = "No connection to Scheduler\n" +
                        ( x.number? "0x" + hex_string( x.number, 8 ) + ": " : "" ) + x.message;
        }

        throw x;
    }
    finally
    {
        window.status = status;
    }
}

//-------------------------------------------------------Scheduler.add_datetime_attributes_for_xslt

Scheduler.prototype.add_datetime_attributes_for_xslt = function( response, now, attribute_name )
{
    var elements = response.selectNodes( "//*[ @" + attribute_name + "]" );
    for( var i = 0; i < elements.length; i++ )
    {
        var element = elements[ i ];
        var value   = element.getAttribute( attribute_name );
        if( value )
        {
            element.setAttribute( attribute_name + "__xslt_datetime"               , xslt_format_datetime     ( value, now ) );
            element.setAttribute( attribute_name + "__xslt_datetime_diff"          , xslt_format_datetime_diff( value, now, false ) );
            element.setAttribute( attribute_name + "__xslt_datetime_with_diff"     , xslt_format_datetime_with_diff( value, now, false ) );
            element.setAttribute( attribute_name + "__xslt_datetime_with_diff_plus", xslt_format_datetime_with_diff( value, now, true ) );
            element.setAttribute( attribute_name + "__xslt_date_or_time"           , xslt_format_date_or_time ( value, now ) );
        }
    }
}

//---------------------------------------------------------------Scheduler.modify_datetime_for_xslt

Scheduler.prototype.modify_datetime_for_xslt = function( response )
{
    // Für Firefox, dass kein Skript im Stylesheet zulässt.
    var now;

    var datetime = response.selectSingleNode( "/spooler/answer/@time" );
    if( datetime )  now = date_from_datetime( datetime.nodeValue );

    this.add_datetime_attributes_for_xslt( response, now, "time"                  );
    this.add_datetime_attributes_for_xslt( response, now, "spooler_running_since" );
    this.add_datetime_attributes_for_xslt( response, now, "running_since"         );
    this.add_datetime_attributes_for_xslt( response, now, "in_process_since"      );
    this.add_datetime_attributes_for_xslt( response, now, "spooler_running_since" );
    this.add_datetime_attributes_for_xslt( response, now, "next_start_time"       );
    this.add_datetime_attributes_for_xslt( response, now, "start_at"              );
    this.add_datetime_attributes_for_xslt( response, now, "idle_since"            );
    this.add_datetime_attributes_for_xslt( response, now, "enqueued"              );
    this.add_datetime_attributes_for_xslt( response, now, "created"               );
}

//---------------------------------------------------------------------Scheduler.call_error_checked

Scheduler.prototype.call_error_checked = function( method_name, arg1, arg2, arg3, arg4, arg5 )
{
    try
    {
        this[ method_name ]( arg1, arg2, arg3, arg4, arg5 );
    }
    catch( x )
    {
        return handle_exception( x );
    }
}

//---------------------------------------------------------------------------------------Stylesheet

function Stylesheet( url )
{
    var xml_http = window.XMLHttpRequest? new XMLHttpRequest() : new ActiveXObject( "Msxml2.XMLHTTP" );
    xml_http.open( "GET", url, false );
    xml_http.send( null );

    if( window.DOMParser )
    {
        var dom_parser = new DOMParser();
        this._xslt_dom = dom_parser.parseFromString( xml_http.responseText, "text/xml" );
        if( this._xslt_dom.documentElement.nodeName == "parsererror" )  throw new Error( "Fehler im Stylesheet " + url + ": " + this._xslt_dom.documentElement.firstChild.nodeValue );

        this._xslt_processor = new XSLTProcessor();
        this._xslt_processor.importStylesheet( this._xslt_dom );
    }
    else
    {
        this._xslt_dom = new ActiveXObject( "MSXML2.DOMDocument" );
        var ok = this._xslt_dom.loadXML( xml_http.responseText );
        if( !ok )  throw new Error( "Fehlerhafte XML-Antwort: " + this._xslt_dom.parseError.reason );
    }
}

//-------------------------------------------------------------------------Stylesheet.xml_transform
// Liefert einen XML-String

Stylesheet.prototype.xml_transform = function( dom_document )
{
    if( this._xslt_processor )
    {
		return new XMLSerializer().serializeToString( this._xslt_processor.transformToDocument( dom_document ) );
    }
    else
    {
        return dom_document.transformNode( this._xslt_dom );
    }
}

//---------------------------------------------------------------------xslt_format_datetime

function xslt_format_datetime( datetime )
{
    if( !datetime )  return "";
    return datetime.replace( /\.\d*$/, "" );
    /*
    var date = typeof datetime == "string"? date_from_datetime( datetime ) : datetime;

    //var ms = date.getMilliseconds();

    return date.toLocaleDateString() + ", " + date.toLocaleTimeString();
            //+ ( ms? ".<span class='milliseconds'>" + ( ms + "000" ).substring( 0, 3 ) + "</span>" : "" );
    */
}

//-----------------------------------------------------------------xslt_format_date_or_time

function xslt_format_date_or_time( datetime )
{
    if( !datetime )  return "";

    var now = new Date();

    if(    1*datetime.substr( 0, 4 ) == now.getYear()
        && 1*datetime.substr( 5, 2 ) == now.getMonth() + 1
        && 1*datetime.substr( 8, 2 ) == now.getDate()  )
    {
        return datetime.substr( 11, 8 );
    }
    else
    {
        return datetime.substr( 0, 10 );
    }
}

//-----------------------------------------------------------------xslt_format_date_or_time

function xslt_format_datetime_with_diff( datetime, now, show_plus )
{
    var date = date_from_datetime( datetime );
    var result = xslt_format_datetime( datetime );
    if( result && now )  result += " \xA0(" + xslt_format_datetime_diff( date, now, show_plus ) + ")";

    return result;
}

//----------------------------------------------------------------xslt_format_datetime_diff

function xslt_format_datetime_diff( datetime_earlier, datetime_later, show_plus )
{
    var show_ms;
    if( show_ms   == undefined )  show_ms   = false;
    if( show_plus == undefined )  show_plus = false;

    var date_later   = typeof datetime_later   == "string"? date_from_datetime( datetime_later )   : datetime_later;
    var date_earlier = typeof datetime_earlier == "string"? date_from_datetime( datetime_earlier ) : datetime_earlier;

    if( !date_later   )  return "";
    if( !date_earlier )  return "";

    var diff = ( date_later.getTime() - date_earlier.getTime() ) / 1000.0;
    var abs  = Math.abs( diff );
    var result;

    if( abs < 60 )
    {
        if( show_ms )
        {
            result = abs.toString();
            if( result.match( "." ) )  result = result.replace( ".", ".<span class='milliseconds'>" ) + "</span>";
        }
        else
        {
            result = Math.floor( abs );
        }
        result += "s";
    }
    else
    if( abs <    60*60 )  result = Math.floor( abs / (       60 ) ) + "min";
    else
    if( abs < 24*60*60 )  result = Math.floor( abs / (    60*60 ) ) + "h";
    else
                          result = Math.floor( abs / ( 24*60*60 ) ) + "days";

    return diff < 0             ? "-" + result :
           show_plus && diff > 0? "+" + result
                                : result;
}

//-----------------------------------------------------------------------date_from_datetime
// datetime == yyyy-mm-dd-hh-mm-ss[.mmm]

function date_from_datetime( datetime )
{
    if( !datetime )  return null;

    var date = new Date( 1*datetime.substr( 0, 4 ),
                         1*datetime.substr( 5, 2 ) - 1,
                         1*datetime.substr( 8, 2 ),
                         1*datetime.substr( 11, 2 ),
                         1*datetime.substr( 14, 2 ),
                         1*datetime.substr( 17, 2 ),
                         datetime.length < 23? 0 : 1*datetime.substr( 20, 3 ) );

    return date;
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
//-------------------------------------------------------------------------------call_error_checked

function call_error_checked( f, arg1, arg2, arg3, arg4, arg5 )
{
    try
    {
        f( arg1, arg2, arg3, arg4, arg5 );
    }
    catch( x )
    {
        return handle_exception( x );
    }
}

//---------------------------------------------------------------------------------handle_exception

function handle_exception( x )
{
    var msg = "";
    var error;

    if( typeof x == "object" )
    {
        if( x.number )  msg += "0x" + hex_string( x.number, 8 ) + "  ";
        msg += x.message;

        error = x;
    }
    else
    {
        msg = x;
        error.message = msg;
    }

    var e = window.parent.left_frame.document.getElementById( "error_message" );
    if( e )
    {
        e.innerHTML = xml_encode( msg ).replace( "\n", "<br/>" ).replace( "  ", "\xA0 " ); // + "<p>&#160;</p>";

        if( typeof x == "object"  &&  x.stack )  e.style.title = x.stack;  // Firefox?
    }
    else
        alert( msg );

    return error;
}

//--------------------------------------------------------------------------------------reset_error

function reset_error()
{
    var e = window.parent.left_frame.document.getElementById( "error_message" );
    if( e )
    {
        e.innerHTML = "";
        e.style.title = "";
    }

    window.status = "";
}

//----------------------------------------------------------------------------------update__onclick

function update__onclick()
{
    window.parent.left_frame.reset_error();
    update();
}

//----------------------------------------------------------------show_order_jobs_checkbox__onclick

function show_order_jobs_checkbox__onclick()
{
    save_checkbox_state( "show_order_jobs_checkbox" );
    window.parent.left_frame.reset_error();
    update();
}

//---------------------------------------------------------------------show_tasks_checkbox__onclick

function show_tasks_checkbox__onclick()
{
    save_checkbox_state( "show_tasks_checkbox" );
    window.parent.left_frame.reset_error();
    update();
}

//------------------------------------------------------------show_job_chain_jobs_checkbox__onclick

function show_job_chain_jobs_checkbox__onclick( event )
{
    save_checkbox_state( "show_job_chain_jobs_checkbox" );
    window.parent.left_frame.reset_error();
    update();
}

//----------------------------------------------------------show_job_chain_orders_checkbox__onclick

function show_job_chain_orders_checkbox__onclick()
{
    save_checkbox_state( "show_job_chain_orders_checkbox" );
    window.parent.left_frame.reset_error();
    update();
}

//-------------------------------------------------------------------Popup_menu_builder.add_command
// Erweiterung von Popup_menu_builder, s. popup_builder.js

function Popup_menu_builder__add_command( html, xml_command, is_active )
{
    this.add_entry( html, "call_error_checked( popup_menu__execute, &quot;" + xml_command + "&quot; )", is_active );
}

//-------------------------------------------------------------------------------popup_menu.execute
// Für Popup_menu_builder.add_command()

function popup_menu__execute( xml_command )
{
    _popup_menu.close();

    _scheduler.execute( xml_command );

    window.parent.left_frame.update();
}

//------------------------------------------------------------------Popup_menu_builder.add_show_log
// Erweiterung von Popup_menu_builder, s. popup_builder.js

function Popup_menu_builder__add_show_log( html, show_log_command, window_name, is_active )
{
    this.add_entry( html, "popup_menu__show_log__onclick( &quot;" + show_log_command + "&quot;, &quot;" + window_name + "&quot; )", is_active );
}

//--------------------------------------------------------------------popup_menu__show_log__onclick
// Für Popup_menu_builder.add_show_log()

function popup_menu__show_log__onclick( show_log_command, window_name )
{
    //window_name = window_name.replace( /[~a-zA-Z0-9]/g, "_" );
    window_name = "show_log";  // Nur ein Fenster. ie6 will nicht mehrere Logs gleichzeitig lesen, nur nacheinander

    var features = "menubar=no, toolbar=no, location=no, directories=no, scrollbars=yes, resizable=yes, status=no";
    features +=  ", width="       + ( window.screen.availWidth - 11 ) +
                 ", innerwidth="  + ( window.screen.availWidth - 11 ) +                             // Für Firefox
                 ", height="      + ( Math.floor( window.screen.availHeight * 0.2 ) - 32 ) +
                 ", innerheight=" + ( Math.floor( window.screen.availHeight * 0.2 ) - 32 ) +        // Für Firefox
                 ", left=0"       +
                 ", top="         +  Math.floor( window.screen.availHeight * 0.8 );

    var log_window = window.open( document.location.href.replace( /\/[^\/]*$/, "/" ) + show_log_command, window_name, features, true );
    log_window.focus();

    if( _scheduler )  _scheduler._dependend_windows[ window_name ] = log_window;

    _popup_menu.close();
}

//--------------------------------------------------------------------------scheduler_menu__onclick

function scheduler_menu__onclick( x, y )
{
    var popup_builder = new Popup_menu_builder();

    var state = _response.selectSingleNode( "spooler/answer/state" ).getAttribute( "state" );

    var command = function( cmd ) { return "<modify_spooler cmd='" + cmd + "'/>"; }

    popup_builder.add_show_log( "Show log"                       , "show_log?", "show_log" );
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

    _popup_menu = popup_builder.show_popup_menu( x, y );
}

//--------------------------------------------------------------------------------job_menu__onclick

function job_menu__onclick( job_name, x, y )
{
    var popup_builder = new Popup_menu_builder();

    var job_element = //document.all._job_element != undefined?
                        _job_element  // _job_element in task_frame.html (detail_frame)                                                                                  // Rechter Rahmen
                      //: _response.selectSingleNode( "spooler/answer/state/jobs/job [ @job = '" + job_name + "' ]" );  // Linker Rahmen

    var state = job_element.getAttribute( "state" );

    popup_builder.add_show_log( "Show log"        , "show_log?job=" + job_name, "show_log_job_" + job_name );

    //var description_element = job_element.selectSingleNode( "description" );
    //var is_active = description_element? description_element.text != "" : false;
    var is_active = job_element.getAttribute( "has_description" ) == "yes";
    popup_builder.add_entry   ( "Show description", "show_job_description()", is_active );

    popup_builder.add_bar();
    popup_builder.add_command ( "Start task now", "<start_job job='" + job_name + "'/>" );
    popup_builder.add_command ( "Stop"          , "<modify_job job='" + job_name + "' cmd='stop'    />", state != "stopped"  &&  state != "stopping" );
    popup_builder.add_command ( "Unstop"        , "<modify_job job='" + job_name + "' cmd='unstop'  />", state == "stopped"  ||  state == "stopping" );
//  popup_builder.add_command ( "Wake"          , "<modify_job job='" + job_name + "' cmd='wake'    />" );
    popup_builder.add_command ( "Start at &lt;runtime&gt;", "<modify_job job='" + job_name + "' cmd='start'   />" );
    popup_builder.add_command ( "Reread"        , "<modify_job job='" + job_name + "' cmd='reread'  />" );
    popup_builder.add_bar();
    popup_builder.add_command ( "End tasks"     , "<modify_job job='" + job_name + "' cmd='end'     />" );
    popup_builder.add_command ( "Suspend tasks" , "<modify_job job='" + job_name + "' cmd='suspend' />" );
    popup_builder.add_command ( "Continue tasks", "<modify_job job='" + job_name + "' cmd='continue'/>" );

    _popup_menu = popup_builder.show_popup_menu( x, y );
}

//-------------------------------------------------------------------------------task_menu__onclick

function task_menu__onclick( task_id, x, y )
{
    var popup_builder = new Popup_menu_builder();

    popup_builder.add_show_log( "Show log"        , "show_log?task=" + task_id, "show_log_task_" + task_id );
    popup_builder.add_bar();
    popup_builder.add_command ( "End"             , "<kill_task job='" + _job_name + "' id='" + task_id + "'/>" );
    popup_builder.add_command ( "Kill immediately", "<kill_task job='" + _job_name + "' id='" + task_id + "' immediately='yes'/>" );

    _popup_menu = popup_builder.show_popup_menu( x, y );
}

//------------------------------------------------------------------------------order_menu__onclick

function order_menu__onclick( job_chain_name, order_id, x, y )
{
    var popup_builder = new Popup_menu_builder();

    popup_builder.add_show_log( "Show log"        , "show_log?job_chain=" + job_chain_name +
                                                            "&order=" + order_id, "show_log_order_" + job_chain_name + "__" + order_id );

    _popup_menu = popup_builder.show_popup_menu( x, y );
}

//-----------------------------------------------------------------------------------------open_url

function open_url( url, window_name )
{
    var features = "menubar=no, toolbar=no, location=no, directories=no, scrollbars=yes, resizable=yes, status=no";

    var my_window = window.open( url, window_name, features, true );
    my_window.focus();

    if( window_name  &&  _scheduler )  _scheduler._dependend_windows[ window_name ] = my_window;
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

//-------------------------------------------------------------------------------string_from_object

function string_from_object( object )
{
    var result = "{";

    for( var i in object )
    {
        result += i + "=";

        try
        {
            result += object[ i ] + " ";
        }
        catch( x )
        {
            result += "(ERROR " + x.message + ") ";
        }
    }

    return result + "}";
}

//-------------------------------------------------------------------------------------------------
