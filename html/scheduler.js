// $Id: scheduler.js,v 1.1 2004/07/16 22:05:44 jz Exp $

//----------------------------------------------------------------------------------------Scheduler
// public

function Scheduler()
{
    this._url           = "http://" + document.location.host + "/";
    this._xml_http      = new ActiveXObject( "Msxml2.XMLHTTP" );
    
    this._configuration = new Scheduler_html_configuration( this._url + "config.xml" );
}

//----------------------------------------------------------------------------------Scheduler.close
// public

Scheduler.prototype.close = function()
{
    this._configuration = null;
    this._xml_http = null;
}

//--------------------------------------------------------------------------------Scheduler.execute
// public

Scheduler.prototype.execute = function( xml )
{
    var response = this.call_http( xml );

    var dom_document = new ActiveXObject( "MSXML2.DOMDocument" );
    
    var ok = dom_document.loadXML( this._xml_http.responseText );
    if( !ok )  throw new Error( "Fehlerhafte XML-Antwort:  " + dom_document.parseError.reason );
    
    return dom_document;
}

//------------------------------------------------------------------Scheduler.execute_and_fill_html
// public

Scheduler.prototype.execute_and_fill_html = function( xml_query )
{
    var dom_document   = this.execute( xml_query );
    var answer_element = dom_document.selectSingleNode( "spooler/answer" );
    
    //dom_document.setProperty( "SelectionLanguage", "XPath" );
    if( !answer_element )  return;

    this.fill_html_state( document.all.scheduler_state, answer_element.selectSingleNode( "state" ) );
    this.fill_html_jobs ( document.all.scheduler_jobs , answer_element.selectSingleNode( "state/jobs" ) );
    
/*                                                            
    var span_elements = document.body.getElementsByTagName( "span" );
    for( var i = 0; i < span_elements.length; i++ )
    {
        var span_element = span_elements[ i ];
        var text = span_element.innerText;
        if( text.substring( 0, 1 ) == "=" )
        {
            try
            {
                var found_element = answer_element.selectSingleNode( text.substring( 1, text.length ) );
                if( found_element )  span_element.innerText = found_element.text;
            }
            catch( x ) { span_element.innerText = x.message; }
        }
    }
*/    
}

//------------------------------------------------------------------------Scheduler.fill_html_state

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

//-------------------------------------------------------------------------Scheduler.fill_html_jobs

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

//-------------------------------------------------------------------------Scheduler.fill_html_jobs

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

//-----------------------------------------------------------------------------Scheduler.fill_spans

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

//------------------------------------------------------------------------------Scheduler.call_http

Scheduler.prototype.call_http = function( text, debug_text )
{
    this._xml_http.open( "POST", this._url, false );
    this._xml_http.setRequestHeader( "Cache-Control", "no-cache" );

    var status = window.status;
    try
    {
        window.status = text;
        this._xml_http.send( text );
    }
    finally
    {
        window.status = status;
    }
}

//---------------------------------------------------------------------Scheduler_html_configuration

function Scheduler_html_configuration( url )
{
    this._dom = new ActiveXObject( "MSXML2.DOMDocument" );

    this._dom.async = false;
    this._dom.validateOnParse = false;  // Wegen DOCTYPE in config.xml (aber warum?)
    var ok  = this._dom.load( url );
    if( !ok )  throw new Error( "Fehler in der Konfiguration " + url + ": " + this._dom.parseError.reason );
}

//-------------------------------------------------------------------------------------------------
