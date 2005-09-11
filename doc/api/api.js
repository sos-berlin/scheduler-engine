// $Id$

// Sarissa wählt die höchste Version aus. Das ist nicht gut.
_SARISSA_DOM_PROGID = "MSXML2.DOMDocument";

//--------------------------------------------------------------------------human_language__onclick
/*
function human_language__onclick()
{
    // ?
}
*/
//---------------------------------------------------------------Api.programming_language__onchange

Api.prototype.programming_language__onchange = function()
{
    var select = document.getElementById( "programming_languages_select" );

    var programming_language = select.options[ select.selectedIndex ].text.toLowerCase();
    if( programming_language != api._programming_language )
    {
        api._programming_language = programming_language;
        api.show();
    }
}

//----------------------------------------------------------------------------------------------Api

function Api()
{
    this._programming_language = "javascript";
    
    var stylesheet_name = "api.xsl";
    
    if( window.ActiveXObject )
    {
        var stylesheet = new ActiveXObject( "MSXML2.FreeThreadedDOMDocument" ); 
        var ok = stylesheet.loadXML( this.fetch_by_url( stylesheet_name ) );
        if( !ok )  throw new Error( "Fehler bei loadXML" );
        
        // Einen Fehler in ie6 umgehen: <xsl:include> führt zu "Access denied". 
        // Aber nicht, wenn wir den Pfad im Attribut href absolut machen:
        
        var href_base        = document.location.href.replace( /\/[^\/]*$/, "/" );   // Alles bis zum letzten Schräger
        var include_elements = stylesheet.selectNodes( "/xsl:stylesheet/xsl:include" );
        for( var i = 0; i < include_elements.length; i++ )
        {
            var href = include_elements[i].getAttribute( "href" );
            if( href.search( "://" ) == -1 )
            {
                include_elements[i].setAttribute( "href", href_base + href );
                //alert(include_elements[i].getAttribute( "href" ) );
            }
        }
        
        
        var template = new ActiveXObject( "MSXML2.XSLTemplate" );
        template.stylesheet = stylesheet;
        
        this._xslt_processor = template.createProcessor();
    }
    else
    {    
        this._xslt_processor = new XSLTProcessor();
        this._xslt_processor.importStylesheet( dom_from_xml( this.fetch_by_url( stylesheet_name ) ) );
    }


    var programming_language = get_cookie( "programming_language" );
    if( !programming_language )  programming_language = this._programming_language;

    // Dropdown-Box einstellen:
    
    var select = document.getElementById( "programming_languages_select" );

    for( var i = 0; i < select.options.length; i++ )
    {
        if( select.options[ i ].text.toLowerCase() == programming_language )
        {
            select.selectedIndex = i;
            this._programming_language = programming_language;
            break;
        }
    }
        
    var class_name = get_cookie( "class" );
    if( class_name )
    {
        this.class_reference__onclick( class_name );
        this.show();
    }
}

//---------------------------------------------------------------------Api.class_reference__onclick

Api.prototype.class_reference__onclick = function( class_name )
{
    if( this._class_name )  document.getElementById( "class_reference_" + this._class_name ).style.fontWeight = "normal";

    this._class_name = class_name;
    this.show();
}

//-------------------------------------------------------------------------------------------------

Api.prototype.show = function()
{
    var class_element = document.getElementById( "class" );
    
    try
    {
        var dom_document = dom_from_xml( this.fetch_by_url( this._class_name + ".xml" ) );
        dom_document.documentElement.setAttribute( "show_list", "true" );
        dom_document.documentElement.setAttribute( "programming_language", this._programming_language );
        
        if( this._programming_language == "java" )  dom_document.documentElement.removeAttribute( "language_has_properties" );
                                              else  dom_document.documentElement.setAttribute( "language_has_properties", "true" );
        
        class_element.style.color = "";
        
        if( window.ActiveXObject )
        {
            this._xslt_processor.reset();
            this._xslt_processor.input = dom_document;
            var ok = this._xslt_processor.transform();
            if( !ok )  throw new Error( "Fehler in XSLProcessor.transform()" );
            class_element.innerHTML = this._xslt_processor.output;
        }
        else
        {
            class_element.innerHTML = new XMLSerializer().serializeToString( this._xslt_processor.transformToDocument( dom_document ) );
        }
    }
    catch( x ) 
    { 
        class_element.innerHTML = ( x? x.message? x.message : x.toString() : "" + x ).replace( /&/g, "&amp;" ).replace( /</g, "&lt;" );
        class_element.style.color = "red";
    }
        
    try 
    {
        document.getElementById( "class_reference_" + this._class_name ).style.fontWeight = "bold";

        document.cookie = "class=" + this._class_name;
        document.cookie = "programming_language=" + this._programming_language;
    }
    catch( x ) { alert( x ); }
}

//---------------------------------------------------------------------------------Api.fetch_by_url

Api.prototype.fetch_by_url = function( url )
{
    if( !this._xml_http )  this._xml_http = window.XMLHttpRequest? new XMLHttpRequest() : new ActiveXObject( "Msxml2.XMLHTTP" );
    
    var xml_http = this._xml_http;
    this._xml_http = null;      // Falls ein Fehler auftritt, muss _xml_http erneuert werden (Firefox 1.0.6)
    xml_http.open( "GET", url, false );
    
    var status = window.status;
    window.status = "fetching " + url + " ..."
    
    //try
    {
        xml_http.send( "" );
    }
    /*
    catch( x )
    {
        if(1)
        //if( x.number == DE_E_CANNOT_CONNECT
        // || x.number == DE_E_DATA_NOT_AVAILABLE
        // || x.number == DE_E_RESOURCE_NOT_FOUND )
        {
            x.message = "No connection to " + url + "\n" +
                        ( x.number? "0x" + hex_string( x.number, 8 ) + ": " : "" ) + x.message;
        }
        this._xml_http = null;        
        throw x;
    }
    // Firefox setzt hier x = undefined: 
    finally
    {
        window.status = status;
    }
*/
    
    this._xml_http = xml_http;
    
    return this._xml_http.responseText;
}

//---------------------------------------------------------------------------------------get_cookie

function get_cookie( name )
{
    if( document.cookie )
    {
        var prefix = name + "=";
        var cookies = document.cookie.split( "; " );
        for( i = 0; i < cookies.length; i++ )
        {
            if( cookies[i].substring( 0, prefix.length ) == prefix )  return cookies[i].substring( prefix.length );
        }
    }
}
    

