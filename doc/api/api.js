// $Id$


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
    this._xml_http = window.XMLHttpRequest? new XMLHttpRequest() : new ActiveXObject( "Msxml2.XMLHTTP" );
    this._xslt_processor = new XSLTProcessor();
    this._xslt_processor.importStylesheet( dom_from_xml( this.fetch_by_url( "api.xsl" ) ) );

        
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
    this._class_name = class_name;
}

//-------------------------------------------------------------------------------------------------

Api.prototype.show = function()
{
    try
    {
        var dom_document = dom_from_xml( this.fetch_by_url( this._class_name + ".xml" ) );
        dom_document.documentElement.setAttribute( "show_list", "true" );
        dom_document.documentElement.setAttribute( "programming_language", this._programming_language );
        
        document.getElementById( "class" ).innerHTML = new XMLSerializer().serializeToString( this._xslt_processor.transformToDocument( dom_document ) );
        
        document.cookie = "class=" + this._class_name;
        document.cookie = "programming_language=" + this._programming_language;
        
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
    

