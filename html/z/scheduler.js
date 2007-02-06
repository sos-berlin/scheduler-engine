// $Id$

//----------------------------------------------------------------------------------------------var

var debug            = false;
var is_logging_times = false;

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

        alert( "Scheduler: " + msg );
    }
}

//-----------------------------------------------------------------------------------------Time_log

function Time_log()
{
    this.times_array = new Array();
    this.log( "start" );
    //this.times_array.push( new Array( "start", 1 * new Date() ) );
}

//-------------------------------------------------------------------------------------Time_log.log

Time_log.prototype.log = function Time_log__log( what )
{
    if( is_logging_times )  this.times_array.push( new Array( what, 1 * new Date() ) );
}

//--------------------------------------------------------------------------------Time_log.get_line

Time_log.prototype.get_line = function Time_log__get_line()
{
    var a = new Array;

    this.log( "total" );
    
    var total = ( this.times_array[ this.times_array.length - 1 ][1] - this.times_array[ 0 ][1] ) / 1000;
    a.push( "total " + total + "s" );

    for( var i = 1; i < this.times_array.length - 1; i++ )
    {
        var t = this.times_array[i];
        var ti = ( t[1] - this.times_array[i-1][1] ) / 1000;
        var p = ti / total;
        if( p >= 0.05 ) a.push( t[0] + " " + Math.round( 100*p ) + "% " + ti + "s"  );
    }
    
    return a.join( ",  " );
}

//----------------------------------------------------------------------------------------Scheduler
// public

function Scheduler()
{
    this._url               = "http://" + document.location.host + "/";
    this._xml_http          = window.XMLHttpRequest? new XMLHttpRequest() : new ActiveXObject( "Msxml2.XMLHTTP" );
    this._dependend_windows = new Object();
    this._time_log          = new Time_log;
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

Scheduler.prototype.execute = function Scheduler__execute( xml )
{
    this.call_http( xml );
    //this._time_log.log( xml );
    if( is_logging_times )  this._time_log.log( ( xml.match( /^(<[^ >]+)[ >]/ )? RegExp.$1 + ">" : "Scheduler.excecute" ) + " " + Math.round( this._xml_http.responseText.length / 1024 ) + "KB" );

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
    this._time_log.log( "DOM" );

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


    if( debug )
    {
        this._xml_http.send( text );
    }
    else
    {
        var status = window.status;
        window.status = text + "    Waiting for response from scheduler ...";
    
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
            element.setAttribute( attribute_name + "__xslt_date_or_time_with_diff" , xslt_format_date_or_time_with_diff( value, now ) );

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
    this.add_datetime_attributes_for_xslt( response, now, "setback"               );
    this.add_datetime_attributes_for_xslt( response, now, "start_time"            );
    this.add_datetime_attributes_for_xslt( response, now, "end_time"              );
    this.add_datetime_attributes_for_xslt( response, now, "connected_at"          );
    this.add_datetime_attributes_for_xslt( response, now, "disconnected_at"       );
    this.add_datetime_attributes_for_xslt( response, now, "wait_until"            );
    this.add_datetime_attributes_for_xslt( response, now, "resume_at"             );
  //this.add_datetime_attributes_for_xslt( response, now, "last_write_time"       );  ist GMT
  
    this._time_log.log( "modify_datetime_for_xslt" );
}

//---------------------------------------------------------------------Scheduler.call_error_checked

Scheduler.prototype.call_error_checked = function( method_name, arg1, arg2, arg3, arg4, arg5 )
{
    if( debug )
    {
        this[ method_name ]( arg1, arg2, arg3, arg4, arg5 );
    }
    else
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
    var result;
    
    if( this._xslt_processor )
    {
        result = new XMLSerializer().serializeToString( this._xslt_processor.transformToDocument( dom_document ) );
    }
    else
    {
        result = dom_document.transformNode( this._xslt_dom );
    }

    _scheduler._time_log.log( "XSLT" );    
    
    return result;
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

//-------------------------------------------------------------------------xslt_format_date_or_time

function xslt_format_date_or_time( datetime )
{
    if( !datetime )  return "";
    if( datetime == "never" )  return datetime;
    if( datetime == "now"   )  return datetime;

    var now = new Date();

    if(    1*datetime.substr( 0, 4 ) == now.getYear() + 1900
        && 1*datetime.substr( 5, 2 ) == now.getMonth() + 1
        && 1*datetime.substr( 8, 2 ) == now.getDate()  )
    {
        return datetime.substr( 11, 8 );
    }
    else
    {
        var tomorrow = new Date( now );
        tomorrow.setDate( tomorrow.getDate() + 1 );

        if(    1*datetime.substr( 0, 4 ) == tomorrow.getYear() + 1900
            && 1*datetime.substr( 5, 2 ) == tomorrow.getMonth() + 1
            && 1*datetime.substr( 8, 2 ) == tomorrow.getDate()  )
        {
            return "tomorrow";
        }
        else    
        {
            return datetime.substr( 0, 10 );
        }
    }
}

//-------------------------------------------------------------------------xslt_format_date_or_time

function xslt_format_datetime_with_diff( datetime, now, show_plus )
{
    var date = date_from_datetime( datetime );
    var result = xslt_format_datetime( datetime );
    
    if( result && now )
    {
        var diff = xslt_format_datetime_diff( date, now, show_plus );
        if( diff )  result += " \xA0(" + diff + ")";
    }

    return result;
}

//---------------------------------------------------------------xslt_format_date_or_time_with_diff

function xslt_format_date_or_time_with_diff( datetime, now )
{
    var date = date_from_datetime( datetime );
    var result = xslt_format_date_or_time( datetime );
    if( result && now )
    {
        var diff = xslt_format_datetime_diff( date, now )
        if( diff )  result += " \xA0(" + diff + ")";
    }

    return result;
}

//------------------------------------------------------------------------xslt_format_datetime_diff

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
    var integer;
    var fraction = "";
    var fraction_characters = " ";

    if( abs <= 99 )
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
    if( abs <= 99*60 )      result = xslt_short_number( abs / (       60 ) ) + "m";
    else
    if( abs <= 48*60*60 )   result = xslt_short_number( abs / (    60*60 ) ) + "h";
    else
                            result = xslt_short_number( abs / ( 24*60*60 ) ) + "d";

    return diff < 0             ? "-" + result :
           show_plus && diff > 0? "+" + result
                                : result;
}

//----------------------------------------------------------------------------------xslt_short_time

function xslt_short_number( positive_number )
{
    var n        = Math.floor( positive_number );
    var fragment = positive_number - n;
    
    return n + ( n >= 10  ||
                 fragment < 0.25? "" :
                 fragment < 0.5 ? "¼" :
                 fragment < 0.75? "½"
                                : "¾" );
}

//-----------------------------------------------------------------------date_from_datetime
// datetime == yyyy-mm-dd-hh-mm-ss[.mmm]

function date_from_datetime( datetime )
{
    if( !datetime || datetime == "never" || datetime == "now" )  return null;

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
    _scheduler._time_log = new Time_log;

    if( debug )
    {
        f( arg1, arg2, arg3, arg4, arg5 );
    }
    else
    try
    {
        f( arg1, arg2, arg3, arg4, arg5 );
    }
    catch( x )
    {
        return handle_exception( x );
    }

    if( is_logging_times )  window.status = _scheduler._time_log.get_line();
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
        alert( "Scheduler: " + msg );

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

//----------------------------------------------------------------------------------modify_response

function modify_response( response )
{
    // Antwort manipulieren: <tasks> mit <task> auffüllen, bis <job tasks=".."> erreicht ist.

    var job_elements = response.selectNodes( ".//job" );
    for( var j = 0; j < job_elements.length; j++ )
    {
        var job_element = job_elements[j];
        
        var tasks_element = job_element.selectSingleNode( "tasks" );
        if( tasks_element )
        {
            var task_elements = tasks_element.selectNodes( "task" );
            var n = 1*job_element.getAttribute( "tasks" ) - task_elements.length;
            for( var t = 0; t < n; t++ )
            {
                tasks_element.appendChild( tasks_element.ownerDocument.createElement( "task" ) );
            }
        } 
    }
    

    // Werte der Checkboxen show_tasks_checkbox und show_my_orders ins XML-Dokument übernehmen (für XSL)
    
    var spooler_element = response.selectSingleNode( "spooler" );
    if( spooler_element )
    {    
        restore_checkbox_state( response, "update_periodically_checkbox"   );
        restore_checkbox_state( response, "show_order_jobs_checkbox"       );
        restore_checkbox_state( response, "show_tasks_checkbox"            );
        restore_checkbox_state( response, "show_job_chain_jobs_checkbox"   );
        restore_checkbox_state( response, "show_job_chain_orders_checkbox" );
        restore_checkbox_state( response, "show_order_history_checkbox"    );

        spooler_element.setAttribute( "my_max_orders"    , window.parent.left_frame._max_orders    );
        spooler_element.setAttribute( "my_show_card"     , window.parent.left_frame._show_card     );
        spooler_element.setAttribute( "my_update_seconds", window.parent.left_frame.update_seconds );
        spooler_element.setAttribute( "my_url_base"      , document.location.href    .replace( /\/[^\/]*$/, "/" ) );   // Alles bis zum letzten Schräger
        spooler_element.setAttribute( "my_url_path_base" , document.location.pathname.replace( /\/[^\/]*$/, "/" ) );   // Pfad bis zum letzten Schräger
    }

    _scheduler._time_log.log( "modify_response" );
}

//------------------------------------------------------------------------------save_checkbox_state

function save_checkbox_state( name )
{
    if( document.getElementById( name ) )
    {
        window.parent.left_frame._checkbox_states[ name ] = document.getElementById( name ).checked;
    }
}

//---------------------------------------------------------------------------restore_checkbox_state

function restore_checkbox_state( response, name )
{
    var spooler_element = response.selectSingleNode( "spooler" );
    if( spooler_element )
    {
        //if( document.all[ name ]  &&  document.all[ name ].checked )
        //{
         //   spooler_element.setAttribute( name, "yes" );
        //}
        //else
        {
            if( window.parent.left_frame._checkbox_states[ name ] )  spooler_element.setAttribute( name, "yes" );
        }
    }
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

//-------------------------------------------------------------show_order_history_checkbox__onclick

function show_order_history_checkbox__onclick()
{
    save_checkbox_state( "show_order_history_checkbox" );
    window.parent.left_frame.reset_error();
    window.parent.left_frame.update();
}

//-------------------------------------------------------------------Popup_menu_builder.add_command
// Erweiterung von Popup_menu_builder, s. popup_builder.js

function Popup_menu_builder__add_command( html, xml_command, is_active )
{
    this.add_entry( html, "call_error_checked( popup_menu__execute, &quot;" + xml_command.replace( /\\/g, "\\\\" ) + "&quot; )", is_active );
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
    var cmd = show_log_command.replace( /\\/g, "\\\\" ).replace( /"/g, "\\&quot;" );
    var w = window_name.replace( /\\/g, "\\\\" ).replace( /"/g, "\\&quot;" );
    this.add_entry( html, "popup_menu__show_log__onclick( &quot;" + cmd + "&quot;, &quot;" + w + "&quot; )", is_active );
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

    if( log_window )   // null, wenn Popups blockiert sind.
    {
        log_window.focus();
        if( _scheduler )  _scheduler._dependend_windows[ window_name ] = log_window;
    }

    _popup_menu.close();
}

//--------------------------------------------------------------------------scheduler_menu__onclick

function scheduler_menu__onclick( x, y )
{
    var popup_builder = new Popup_menu_builder();

    var state         = _response.selectSingleNode( "spooler/answer/state" ).getAttribute( "state" );
    var waiting_errno = _response.selectSingleNode( "spooler/answer/state" ).getAttribute( "waiting_errno" );

    var command = function( cmd ) { return "<modify_spooler cmd='" + cmd + "'/>"; }

    popup_builder.add_show_log( "Show log"                       , "show_log?", "show_log" );
    popup_builder.add_bar();
  //popup_builder.add_command ( "Stop"                           , command( "stop" ), state != "stopped"  &&  state != "stopping"  &&  state != "stopping_let_run" );
    popup_builder.add_command ( "Pause"                          , command( "pause"    ) , state != "paused" && !waiting_errno );
    popup_builder.add_command ( "Continue"                       , command( "continue" ) , state == "paused" || waiting_errno );
    popup_builder.add_bar();
  //popup_builder.add_command ( "Reload"                         , command( "reload"   ), state != "stopped"  &&  state != "stopping" );
  
    
    if( _response.selectSingleNode( "spooler/answer/state/cluster" ) )
    {
        popup_builder.add_command ( "Terminate cluster"            , "<terminate all_schedulers='yes'/>"              , !waiting_errno );
        popup_builder.add_command ( "Terminate cluster within ~10s", "<terminate all_schedulers='yes' timeout='10'/>" , !waiting_errno );
        popup_builder.add_command ( "Terminate and restart cluster", "<terminate all_schedulers='yes' restart='yes'/>", !waiting_errno );
        
        if( _response.selectSingleNode( "spooler/answer/state" ).getAttribute( "exclusive" ) == "yes" )
            popup_builder.add_command ( "Terminate and continue exclusive operation", "<terminate continue_exclusive_operation='yes'/>", !waiting_errno );
    }
    else
    {
        popup_builder.add_command ( "Terminate"            , "<terminate/>"              , !waiting_errno );
        popup_builder.add_command ( "Terminate within 10s" , "<terminate timeout='10'/>" , !waiting_errno );
        popup_builder.add_command ( "Terminate and restart", "<terminate restart='yes'/>", !waiting_errno );
    }
    
    
  //popup_builder.add_command ( "Let run, terminate and restart" , command( "let_run_terminate_and_restart" ), !waiting_errno );
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
    var is_initialized = state != "not_initialized";

    popup_builder.add_show_log( "Show log"      , "show_log?job=" + job_name, "show_log_job_" + job_name, is_initialized );

    //var description_element = job_element.selectSingleNode( "description" );
    //var is_active = description_element? description_element.text != "" : false;
    var is_active = job_element.getAttribute( "has_description" ) == "yes";
    popup_builder.add_entry   ( "Show description", "show_job_description()", is_active );

    popup_builder.add_bar();
    popup_builder.add_command ( "Start task now", "<start_job job='" + job_name + "'/>"                , is_initialized );
    popup_builder.add_command ( "Start at &lt;runtime&gt;", "<modify_job job='" + job_name + "' cmd='start' />", is_initialized );
    popup_builder.add_command ( "Stop"          , "<modify_job job='" + job_name + "' cmd='stop'    />", is_initialized  &&  state != "stopped"  &&  state != "stopping" );
    popup_builder.add_command ( "Unstop"        , "<modify_job job='" + job_name + "' cmd='unstop'  />", is_initialized  &&  ( state == "stopped"  ||  state == "stopping" ) );
//  popup_builder.add_command ( "Wake"          , "<modify_job job='" + job_name + "' cmd='wake'    />", is_initialized );
    popup_builder.add_command ( "Reread"        , "<modify_job job='" + job_name + "' cmd='reread'  />", is_initialized );
    popup_builder.add_command ( "Remove job"    , "<modify_job job='" + job_name + "' cmd='remove'  />", is_initialized  &&  job_element.getAttribute( "order" ) != "yes" );
    popup_builder.add_bar();
    popup_builder.add_command ( "End tasks"     , "<modify_job job='" + job_name + "' cmd='end'     />", is_initialized );
    popup_builder.add_command ( "Suspend tasks" , "<modify_job job='" + job_name + "' cmd='suspend' />", is_initialized );
    popup_builder.add_command ( "Continue tasks", "<modify_job job='" + job_name + "' cmd='continue'/>", is_initialized );

    _popup_menu = popup_builder.show_popup_menu( x, y );
}

//-------------------------------------------------------------------------------task_menu__onclick

function task_menu__onclick( task_id, x, y )
{
    var popup_builder = new Popup_menu_builder();

    popup_builder.add_show_log( "Show log"        , "show_log?task=" + task_id, "show_log_task_" + task_id );
    popup_builder.add_bar();
    popup_builder.add_command ( "End"             , "<kill_task job='" + _job_name + "' id='" + task_id + "'/>" );
  //popup_builder.add_command ( "Suspend"         , "<modify_task id='" + task_id + "'/> cmd='suspend'" );
  //popup_builder.add_command ( "Continue"        , "<modify_task id='" + task_id + "'/> cmd='continue'" );
    popup_builder.add_command ( "Kill immediately", "<kill_task job='" + _job_name + "' id='" + task_id + "' immediately='yes'/>" );

    _popup_menu = popup_builder.show_popup_menu( x, y );
}

//-----------------------------------------------------------------------history_task_menu__onclick

function history_task_menu__onclick( task_id, x, y )
{
    var popup_builder = new Popup_menu_builder();

    popup_builder.add_show_log( "Show log"        , "show_log?task=" + task_id, "show_log_task_" + task_id );

    _popup_menu = popup_builder.show_popup_menu( x, y );
}

//------------------------------------------------------------------------------order_menu__onclick

function order_menu__onclick( job_chain_name, order_id, x, y )
{
    var popup_builder = new Popup_menu_builder();

    popup_builder.add_show_log( "Show log"        , "show_log?job_chain=" + job_chain_name +
                                                            "&order=" + order_id, "show_log_order_" + job_chain_name + "__" + order_id );
    popup_builder.add_command ( "suspended=yes"   , "<modify_order job_chain='" + xml_encode_attribute( job_chain_name ) + "' order='" + xml_encode_attribute( order_id ) + "' suspended='yes' />" );
    popup_builder.add_command ( "suspended=no"    , "<modify_order job_chain='" + xml_encode_attribute( job_chain_name ) + "' order='" + xml_encode_attribute( order_id ) + "' suspended='no' />" );
    popup_builder.add_command ( "Continue now"    , "<modify_order job_chain='" + xml_encode_attribute( job_chain_name ) + "' order='" + xml_encode_attribute( order_id ) + "' at='now' />" );
  //popup_builder.add_command ( "setback=no"      , "<modify_order job_chain='" + xml_encode_attribute( job_chain_name ) + "' order='" + xml_encode_attribute( order_id ) + "' setback='no' />" );
    popup_builder.add_command ( "Remove"          , "<remove_order job_chain='" + xml_encode_attribute( job_chain_name ) + "' order='" + xml_encode_attribute( order_id ) + "' />" );

    _popup_menu = popup_builder.show_popup_menu( x, y );
}

//----------------------------------------------------------------------history_order_menu__onclick

function history_order_menu__onclick( job_chain_name, order_id, history_id, x, y )
{
    var popup_builder = new Popup_menu_builder();

    popup_builder.add_show_log( "Show log"        , "show_log?job_chain=" + job_chain_name + "&order=" + order_id + "&history_id=" + history_id,
                                                    "show_log_order_" + job_chain_name + "__" + order_id );

    _popup_menu = popup_builder.show_popup_menu( x, y );
}

//--------------------------------------------------------------------------cluster_member__onclick

function cluster_member__onclick( cluster_member_id, x, y )
{
    if( cluster_member_id+"" != "" )
    {
        var popup_builder = new Popup_menu_builder();
        var is_dead = _response.selectSingleNode( "spooler/answer//cluster_member [ @cluster_member_id='" + cluster_member_id + "' ]" ).getAttribute( "dead" ) == "yes";

        if( is_dead )
        {
            popup_builder.add_command( "delete entry"    , "<terminate cluster_member_id='" + xml_encode_attribute( cluster_member_id ) + "' delete_dead_entry='yes' />" );
        }
        else
        {
            popup_builder.add_command( "terminate"       , "<terminate cluster_member_id='" + xml_encode_attribute( cluster_member_id ) + "' />" );
            popup_builder.add_command( "restart"         , "<terminate cluster_member_id='" + xml_encode_attribute( cluster_member_id ) + "' restart='yes' />" );
        }
 
        _popup_menu = popup_builder.show_popup_menu( x, y );
    }
}

//-----------------------------------------------------------------------------------------open_url

function open_url( url, window_name, features, replace )
{
    //var features = "menubar=no, toolbar=no, location=no, directories=no, scrollbars=yes, resizable=yes, status=no";
    if( features == undefined )  features = "";
    if( replace  == undefined )  replace  = false;

    var my_window = window.open( url, window_name, features, replace );
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
}

//-----------------------------------------------------------------------------xml_encode_attribute

function xml_encode_attribute( text )
{
    if( text == null )  return "";
    return xml_encode( text ).replace( /"/g, "&quot;" );
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
