// $Id: browser_dependencies.js 3878 2005-09-12 13:14:55Z jz $

// Joacim Zschimmer
//
// Anpassungen für DOM und XSL-T
// Microsoft Internet Explorer 6
// Mozilla Firefox 1

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
	
//--------------------------------------------------------------------------------------Element.xml
// Nachbildung von Microsofts IXMLDOMElement.xml

if( window.Element  &&  !Element.prototype.xml )
{
    Element.prototype.__defineGetter__
    ( 
	"xml", 
	
	function()
	{
		return new XMLSerializer().serializeToString( this );
	} 
    );
}
	
//-------------------------------------------------------------------------------------Element.text
// Nachbildung von Microsofts IXMLDOMElement.text

if( window.Element  &&  !Element.prototype.text )
{
    Element.prototype.__defineGetter__
    ( 
	"text", 
	
	function()
	{
            var result = new Array();

	    for( var e = this.firstChild; e; e = e.nextSibling )
	    {
	        var text = e.text;
            if( text )  result.push( text );
	    }
	    
	    return result;
	} 
    );
}
	
//----------------------------------------------------------------------------------------Text.text
// Nachbildung von Microsofts IXMLDOMText.text

// Firefox 1.0.6: Hier erscheint folgende Meldung in der JavaScript-Konsole, ohne das das Programm abbräche oder Text.text nicht funktionieren würde
//                Fehler: uncaught exception: [Exception... "Illegal operation on WrappedNative prototype object"  nsresult: "0x8057000c (NS_ERROR_XPC_BAD_OP_ON_WN_PROTO)"  location: "JS frame :: http://lillerud:8080/quicklist/browser_dependencies.js :: anonymous :: line 76"  data: no]
/*
if( window.Text  &&  !Text.prototype.text )
{
    Text.prototype.__defineGetter__
    ( 
	"text", 
	
	function()
	{
	    return this.data;
	} 
    );
}
*/

//--------------------------------------------------------------------------------CDATASection.text
// Nachbildung von Microsofts IXMLDOMCDATASection.text

if( window.CDATASection  &&  !CDATASection.prototype.text )
{
    CDATASection.prototype.__defineGetter__
    ( 
        "text", 
        
        function()
        {
	    return this.data;
        } 
    );
}
	
//---------------------------------------------------------------------XMLDocument.selectSingleNode
// Nachbildung von Microsofts DOMDocument.selectSingleNode
/* wir nehmen Sarissa.
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
*/
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

//---------------------------------------------------------------------------------new_dom_document

function new_dom_document()
{
    var result;
    
    if( window.DOMParser )
    {
        //result = new DOMParser().parseFromString( "<a id='a'/>", "text/xml" );
        //result.documentElement.removeChild( result.getElementById( "a" ) );
        //return new DOMDocument();
    }
    else
    {
        result = new ActiveXObject( "MSXML2.DOMDocument" );
    }    
    
    return result;
}

//-------------------------------------------------------------------------------------dom_from_xml

function dom_from_xml( xml )
{
    var result;
    
    if( window.DOMParser )
    {
        result = new DOMParser().parseFromString( xml, "text/xml" );

        if( result.documentElement.nodeName == "parsererror" )  
            throw new Error( "Error in XML document: " + result.documentElement.firstChild.nodeValue );
    }
    else
    {
        result = new ActiveXObject( "MSXML2.DOMDocument" );
        var ok = result.loadXML( xml );
        if( !ok )  throw new Error( "Error in XML document: " + result.parseError.reason );
    }
    
    return result;
}

//-------------------------------------------------------------------------------------------------
