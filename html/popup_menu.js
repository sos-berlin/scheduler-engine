// $Id: popup_menu.js,v 1.3 2004/12/03 18:39:00 jz Exp $

var __current_popup_menu;

//-------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------Popup_menu

function Popup_menu()
{
}

//---------------------------------------------------------------------------------Popup_menu.close

Popup_menu.prototype.close = function()
{
    if( window.createPopup == undefined )
    {
        var div = document.getElementById( "__popup_menu__" );
        if( div )
        {
            div.style.visibility = "hidden";
            div.innerHTML = "";
        }
    }
    else
    {
        this._popup.hide();
    }
    
    __current_popup_menu = null;
}

//-------------------------------------------------------------------------------Popup_menu_builder

function Popup_menu_builder()
{
    this._popup_menu = new Popup_menu();
    this._html_array = new Array();
    
    /*if( this._full_html )
    {
        this._html_array.push( "<html>" );
      //this._html_array.push( "<head>" );
      //this._html_array.push( "<style type='text/css'>" );
      //this._html_array.push( "    td { background-color: menu; color: menutext; font-family: Sans-Serif; font-size: 9pt }" );
      //this._html_array.push( "</style>" );
      //this._html_array.push( "</head><body>" );
        this._html_array.push( "<body>" );
    }*/
    
    this._html_array.push( "<table cellpadding='0' cellspacing='0' width='100%'" +
                                 " style='background-color: menu; line-height: 12pt; border: thin outset'>" );
    this._finished = false;
}

//-------------------------------------------------------------------------------------------------

//Popup_menu_builder.prototype._full_html = window.createPopup != undefined;

//---------------------------------------------------------------------Popup_menu_builder.add_entry

Popup_menu_builder.prototype.add_entry = function( html_entry, call, is_active )
{
    if( is_active == undefined )  is_active = true;
    
    html =  "<tr>";
    html += "<td style='";
    html +=         "font-family: Tahoma, Sans-Serif; font-size: 8pt; ";
    html +=         "color: " + ( is_active? "menutext" : "gray" ) + ";";
    html +=         "cursor: default; white-space: nowrap; ";
    html +=         "padding-left=12pt; padding-right=12pt;'";

    if( call != undefined  &&  is_active )
    {
        html +=   " onmouseover='this.style.backgroundColor=\"highlight\"; this.style.color=\"highlighttext\"'";
        html +=   " onmouseout='this.style.backgroundColor=\"menu\"; this.style.color=\"menutext\"'";
        html +=   " onclick=\"";
        
        if( window.createPopup != undefined )  html += "with( parent )  { ";
        html += call.replace( "\"", "\\\"" );
        if( window.createPopup != undefined )  html += "}";
        
        html +=    "\"";
    }
    
    html +=  ">";
    html +=  html_entry;
    html += "</td>";
    html += "</tr>";
                         
    this._html_array.push( html );
}

//-----------------------------------------------------------------------Popup_menu_builder.add_bar

Popup_menu_builder.prototype.add_bar = function()
{
    html =  "<tr>";
    html += "<td style='color: gray;'>";
    html +=  "<hr size='1'/>";
    html += "</td>";
    html += "</tr>";
                         
    this._html_array.push( html );
}

//--------------------------------------------------------------------------Popup_menu_builder.html

Popup_menu_builder.prototype.html = function()
{
    if( !this._finished )
    {
        if( window.createPopup == undefined )       // Solange das Popupmenü nicht durch einen Klick woandershin verschwindet (wie beim Internet Explorer)
        {
            this.add_bar();
            this.add_entry( "(close menu)", "__current_popup_menu.close()" );
        }
    
        this._html_array.push( "<tr><td style='background-color: menu; line-height: 3pt'>&#160;</td></tr>" );
        this._html_array.push( "</table>" );
        //if( this._full_html )  this._html_array.push( "</body></html>" );
        this._finished = true;
    }
    
    return this._html_array.join( "" );
}

//-------------------------------------------------------------Popup_menu_builder.create_popup_menu

Popup_menu_builder.prototype.create_popup_menu = function()
{
    if( window.createPopup == undefined )
    {
        var div = document.getElementById( "__popup_menu__" );
        if( !div )
        {
            var body = document.getElementsByTagName( "body" )[ 0 ];
            div = body.ownerDocument.createElement( "div" );
            div.setAttribute( "id"   , "__popup_menu__" );
            div.setAttribute( "style", "visibility: hidden" );
          //div.setAttribute( "onblur", "alert(1);__current_popup_menu.close()" );
            body.appendChild( div );
        }
        
        div.innerHTML = this.html();
    }
    else
    {
        if( !this._popup_menu._popup )
        {
            this._popup_menu._popup = window.createPopup();
            this._popup_menu._popup.document.body.innerHTML = this.html();
        }
    }
    
    return this._popup_menu;
}

//---------------------------------------------------------------Popup_menu_builder.show_popup_menu

Popup_menu_builder.prototype.show_popup_menu = function()
{
    this.create_popup_menu();
    
    __current_popup_menu = this._popup_menu;

    if( window.createPopup == undefined )
    {
        var div = document.getElementById( "__popup_menu__" );

        div.style.left       = "0px";
        div.style.top        = "0px";
        div.style.width      = "10px";
        div.style.position   = "absolute";
        div.style.zIndex     = 999;
        div.style.visibility = "visible";
    }
    else    
    {
        this._popup_menu._popup.show( 0, 0, 0, 0 );
        var width  = this._popup_menu._popup.document.body.scrollWidth;
        var height = this._popup_menu._popup.document.body.scrollHeight;
        this._popup_menu._popup.hide();
        this._popup_menu._popup.show( 0, 15, width, height, event.srcElement );
    }
    
    return this._popup_menu;
}

//-------------------------------------------------------------------------------------------------
