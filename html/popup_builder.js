// $Id: popup_builder.js,v 1.1 2004/07/20 19:43:15 jz Exp $

//------------------------------------------------------------------------------------Popup_builder

function Popup_builder()
{
    this._html_array = new Array();
    this._html_array.push( "<html><body><table cellpadding='0' cellspacing='0' style='padding-left=3px; padding-right=3px' width='100%'>" );
    this._finished = false;
}

//--------------------------------------------------------------------------Popup_builder.add_entry

Popup_builder.prototype.add_entry = function( html, call )
{
    this._html_array.push( "<tr>" +
                               "<td style='background-color: menu; color: menutext; font-family: Sans-Serif; font-size: 9pt'" +
                                  " onmouseover='this.style.backgroundColor=\"highlight\"; this.style.color=\"highlighttext\"'" +
                                  " onmouseout='this.style.backgroundColor=\"menu\"; this.style.color=\"menutext\"'" +
                                  " style='cursor: default' onclick=\"" + call.replace( "\"", "\\\"" ) + "\">" +
                                      html +
                               "</td>" +
                           "</tr>"
                         );
}

//-------------------------------------------------------------------------------Popup_builder.html

Popup_builder.prototype.html = function()
{
    if( !this._finished )
    {
        this._html_array.push( "</table></body></html>" );
        this._finished = true;
    }
    
    return this._html_array.join( "" );
}

//-----------------------------------------------------------------------Popup_builder.create_popup

Popup_builder.prototype.create_popup = function()
{
    var popup = window.createPopup();
    popup.document.body.innerHTML = this.html();
    
    return popup;
}

//-------------------------------------------------------------------------------------------------
