// $Id: browser_dependencies.js,v 1.1 2004/12/02 10:17:36 jz Exp $

// Anpassungen für DOM und XSL-T

//----------------------------------------------------------------------------------XMLDocument.xml
// Nachbildung von Microsofts DOMDocument.xml

if( window.XMLDocument  &&  !XMLDocument.prototype.xml )
{
	Document.prototype.__defineGetter__
	( 
	    "xml", 
	    
	    function()
	    {
		    return new XMLSerializer().serializeToString( this );
	    } 
	);
}
	
//---------------------------------------------------------------------XMLDocument.selectSingleNode
// Nachbildung von Microsofts DOMDocument.selectSingleNode

if( window.XMLDocument  &&  !XMLDocument.prototype.selectSingleNode )
{
    window.XMLDocument.prototype.selectSingleNode = function( path )
    {
        return this.evaluate( path, this, null, 0, null ).iterateNext();
    }
}

//-------------------------------------------------------------------------Element.selectSingleNode
// Nachbildung von Microsofts IXMLDOMElement.selectSingleNode

if( window.Element  &&  !Element.prototype.selectSingleNode )
{
    window.Element.prototype.selectSingleNode = function( path )
    {
        return this.ownerDocument.evaluate( path, this, null, 0, null ).iterateNext();
    }
}

//--------------------------------------------------------------------------XMLDocument.selectNodes
// Nachbildung von Microsofts DOMDocument.selectNodes

if( window.XMLDocument  &&  !XMLDocument.prototype.selectNodes )
{
    window.XMLDocument.prototype.selectNodes = function( path )
    {
        return this.documentElement.selectNodes( path );
    }
}

//------------------------------------------------------------------------------Element.selectNodes
// Nachbildung von Microsofts IXMLDOMElement.selectNodes

if( window.Element  &&  !Element.prototype.selectNodes )
{
    window.Element.prototype.selectNodes = function( path )
    {
        var result = new Array();
        var xslt_result_set = this.ownerDocument.evaluate( path, this, null, 0, null );
        for( var next = xslt_result_set.iterateNext(); next; next = xslt_result_set.iterateNext() )  result.push( next );
        return result;
    }
}

//------------------------------------------------------------------------XMLDocument.transformNode
// Nachbildung von Microsofts DOMDocument.transformNode

if( window.XMLDocument  &&  !XMLDocument.prototype.transformNode )
{
    window.XMLDocument.prototype.transformNode = function( stylesheet_dom_document )
    {
        var xslt_processor = new XSLTProcessor();
        xslt_processor.importStylesheet( stylesheet_dom_document );
		return new XMLSerializer().serializeToString( xslt_processor.transformToDocument( this ) );
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

//-------------------------------------------------------------------------------------------------
