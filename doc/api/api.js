// $Id$

// Sarissa wählt die höchste Version aus. Das ist nicht gut.
//_SARISSA_DOM_PROGID = "MSXML2.DOMDocument";

//var base_dir  = "../";
var href_base = document.location.href.replace( /\/[^\/]*$/, "/" );   // Alles bis zum letzten Schräger


function Xslt_stylesheet()
{
}

//---------------------------------------------------------------------------------------------Xslt

if( window.ActiveXObject )
{
    Xslt_stylesheet.prototype.load = function( stylesheet_dom_document )
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


    Xslt_stylesheet.prototype.xml_transform = function( dom_document )
    {
        this._xslt_processor.reset();
        this._xslt_processor.input = dom_document;
        
        var ok = this._xslt_processor.transform();
        if( !ok )  throw new Error( "Fehler in XSLProcessor.transform()" );
        
        return this._xslt_processor.output;
    }
    
    
    Xslt_stylesheet.prototype.set_parameter = function( name, value )
    {
        this._xslt_processor.addParameter( name, value );
    }    
}
else
{
    Xslt_stylesheet.prototype.load = function( stylesheet_dom_document )
    {
        this._xslt_processor = new XSLTProcessor();
        this._xslt_processor.importStylesheet( stylesheet_dom_document );
    }


    Xslt_stylesheet.prototype.xml_transform = function( dom_document )
    {
        return new XMLSerializer().serializeToString( this._xslt_processor.transformToDocument( dom_document ) );
    }


    Xslt_stylesheet.prototype.set_parameter = function( name, value )
    {
        this._xslt_processor.setParameter( name, value );
    }    
}

//--------------------------------------------------------------------------human_language__onclick
/*
function human_language__onclick()
{
    // ?
}
*/
//---------------------------------------------------------------Api.programming_language__onchange
/*
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
*/

//-------------------------------------------------------Api.programming_language_selector__onclick

Api.prototype.programming_language_selector__onclick = function( programming_language )
{
    //try
    {
        if( programming_language != api._programming_language )
        {
            var e = document.getElementById( "programming_language_selector__" + this._programming_language );
            if( e )  e.style.fontWeight = "normal";
            
            api._programming_language = programming_language.toLowerCase();
            api.show();
        }
    }
    //catch( x )  { alert( x && x.message? x.message : ( "" + x ) ); }
}

//----------------------------------------------------------------------------------------------Api

function Api()
{
    var stylesheet_name = base_dir + "api/api.xsl";
    var stylesheet_href_base = stylesheet_name.replace( /\/[^\/]*$/, "/" );   // Alles bis zum letzten Schräger

    this._class_name           = get_cookie( "class", "" );
    this._programming_language = get_cookie( "programming_language", "javascript" );
    
    
    if( window.ActiveXObject )
    {
        var stylesheet = new ActiveXObject( "MSXML2.FreeThreadedDOMDocument" ); 
        var ok = stylesheet.loadXML( this.fetch_by_url( stylesheet_name ) );
        if( !ok )  throw new Error( "Fehler bei loadXML" );
        
        // Einen Fehler in ie6 umgehen: <xsl:include> führt zu "Access denied". 
        // Aber nicht, wenn wir den Pfad im Attribut href absolut machen:
        
        var include_elements = stylesheet.selectNodes( "/xsl:stylesheet/xsl:include" );
        for( var i = 0; i < include_elements.length; i++ )
        {
            var href = include_elements[i].getAttribute( "href" );
            if( href.search( "://" ) == -1 )
            {
                include_elements[i].setAttribute( "href", stylesheet_href_base + href );
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

        
    //this.show();
}

//---------------------------------------------------------------------Api.class_reference__onclick

Api.prototype.class_reference__onclick = function( class_name )
{
    var e = document.getElementById( "class_reference_" + this._class_name );
    if( e )  e.style.fontWeight = "normal";

    this._class_name = class_name;
    this.show();
}

//-------------------------------------------------------------------------------------------------

Api.prototype.apply_xslt_stylesheet = function( html_element, dom_document )
{
    
    if( window.ActiveXObject )
    {
        this._xslt_processor.reset();
        this._xslt_processor.input = dom_document;
        var ok = this._xslt_processor.transform();
        if( !ok )  throw new Error( "Fehler in XSLProcessor.transform()" );
        html_element.innerHTML = this._xslt_processor.output;
    }
    else
    {
        html_element.innerHTML = new XMLSerializer().serializeToString( this._xslt_processor.transformToDocument( dom_document ) );
    }
}

//-------------------------------------------------------------------------------------------------

Api.prototype.show = function()
{
    var class_headline_element = document.getElementById( "class_headline" );
    var class_element          = document.getElementById( "class" );
    var methods_element        = document.getElementById( "methods" );

    class_headline_element.innerHTML = "";
    class_element.innerHTML          = "";
    class_element.style.color        = "";
    methods_element.innerHTML        = "";
    
    //try
    {
        //if( this._programming_language.toLowerCase() == "javadoc" )
        //{
        //    class_headline_element.innerHTML = "";
        //    class_element.style.color = "";
        //    class_element.innerHTML = this.fetch_by_url( base_dir + "javadoc/sos/spooler/" + this._class_name + ".html" );
        //    methods_element.innerHTML = "";
        //}
        //else
        {
            var dom_document = dom_from_xml( this.fetch_by_url( this._class_name? "classes/" + this._class_name + ".xml" : "introduction.xml" ) );
            dom_document.documentElement.setAttribute( "programming_language", this._programming_language );
            
            if( this._programming_language == "java"
             || this._programming_language == "perl" )  dom_document.documentElement.removeAttribute( "language_has_properties" );
                                                  else  dom_document.documentElement.setAttribute   ( "language_has_properties", "true" );
            
            if( this._class_name )
            {                                      
                dom_document.documentElement.setAttribute( "show_table", "true" );
                this.apply_xslt_stylesheet( class_element, dom_document );                                             
                dom_document.documentElement.removeAttribute( "show_table");
                
                dom_document.documentElement.setAttribute( "show_headline", "true" );
                this.apply_xslt_stylesheet( class_headline_element, dom_document );                                             
                dom_document.documentElement.removeAttribute( "headline");

                dom_document.documentElement.setAttribute( "show_detailed_methods", "true" );
                this.apply_xslt_stylesheet( methods_element, dom_document );                                             
                dom_document.documentElement.removeAttribute( "show_methods");
            }
            else
            {
                this.apply_xslt_stylesheet( class_headline_element, dom_document );                                             
            }
        }
        
        
        var e = document.getElementById( "class_reference_" + this._class_name )
        if( e )  e.style.fontWeight = "bold";
        
        var e = document.getElementById( "programming_language_selector__" + this._programming_language );
        if( e )  e.style.fontWeight = "bold";

        document.cookie = "class=" + this._class_name;
        document.cookie = "programming_language=" + this._programming_language;
    }
    if(0)//catch( x ) 
    { 
        class_element.innerHTML = ( x? x.message? x.message : x.toString() : "" + x ).replace( /&/g, "&amp;" ).replace( /</g, "&lt;" );
        class_element.style.color = "red";
    }
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
    
    window.status = "";
    this._xml_http = xml_http;
    
    return this._xml_http.responseText;
}

//---------------------------------------------------------------------------------------get_cookie

function get_cookie( name, deflt )
{
    if( document.cookie )
    {
        var prefix = name + "=";
        var cookies = document.cookie.split( /; */ );
        for( i = 0; i < cookies.length; i++ )
        {
            if( cookies[i].substring( 0, prefix.length ) == prefix )  return cookies[i].substring( prefix.length );
        }
    }
    
    return deflt;
}
    
//-------------------------------------------------------------------------------------------------
