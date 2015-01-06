// $Id: browser_dependencies.js 11759 2005-10-26 18:12:09Z jz $

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

//-------------------------------------------------------------------------------------------------
