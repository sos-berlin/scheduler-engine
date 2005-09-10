// $Id$


//--------------------------------------------------------------------------human_language__onclick
/*
function human_language__onclick()
{
    // ?
}
*/
//--------------------------------------------------------------------programming_language__onclick

function programming_language__onclick()
{
}

//----------------------------------------------------------------------------------------------Api

function Api()
{
    this._xml_http = window.XMLHttpRequest? new XMLHttpRequest() : new ActiveXObject( "Msxml2.XMLHTTP" );
    this._xslt_processor = new XSLTProcessor();
    this._xslt_processor.importStylesheet( dom_from_xml( this.fetch_by_url( "api.xsl" ) ) );
}

//---------------------------------------------------------------------Api.class_reference__onclick

Api.prototype.class_reference__onclick = function( class_name )
{
    try
    {
        var dom_document = dom_from_xml( this.fetch_by_url( class_name + ".xml" ) );
        dom_document.documentElement.setAttribute( "show_list", "true" );        
        
        document.getElementById( "class" ).innerHTML = new XMLSerializer().serializeToString( this._xslt_processor.transformToDocument( dom_document ) );
    }
    catch( x ) { alert( x ); }
}

//---------------------------------------------------------------------------------Api.fetch_by_url

Api.prototype.fetch_by_url = function( url )
{
    this._xml_http.open( "GET", url, false );
    
    var status = window.status;
    window.status = "fetching " + url + " ..."

    //try
    {
        this._xml_http.send( "" );
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

        throw x;
    }
    // Firefox setzt hier x = undefined: 
    finally
    */
    {
        window.status = status;
    }
    
    return this._xml_http.responseText;
}

