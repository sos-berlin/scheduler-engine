// $Id$

var all_classes_1_element = document.getElementById( 'all_classes_1' );
var all_classes_2_element = document.getElementById( 'all_classes_2' );

//-----------------------------------------------------------------api_method_in_table__onmouseover
// Alle Varianten einer Methode in der Übersicht werden gemeinsam selektiert

function api_method_in_table__onmouseover( method_id )
{
    api_method_in_table__onmouse( method_id, "api_method_clickable_hover" );
}

//------------------------------------------------------------------api_method_in_table__onmouseout
// Alle Varianten einer Methode in der Übersicht werden gemeinsam selektiert

function api_method_in_table__onmouseout( method_id )
{
    api_method_in_table__onmouse( method_id, "api_method_clickable" );
}

//---------------------------------------------------------------------api_method_in_table__onmouse
// Alle Varianten einer Methode in der Übersicht werden gemeinsam selektiert

function api_method_in_table__onmouse( method_id, class_name )
{
    var len   = method_id.length;
    var nodes = document.getElementsByTagName( "tr" );
    var n     = nodes.length;

    for( var i = 0; i < n; i++ )
    {
        var id = nodes[ i ].getAttribute( "id" );
        if( id  &&  id.substring( 0, len ) == method_id )  nodes[ i ].className = class_name;
    }
}

//-----------------------------------------------------------------------------position_all_classes
// Für ie6

function set_all_classes_position()
{
    all_classes_1_element.style.position = "relative";
    all_classes_2_element.style.position = "relative";
    all_classes_1_element.style.top = document.documentElement.scrollTop + "px";
    all_classes_2_element.style.top = document.documentElement.scrollTop + "px";
}

//-------------------------------------------------------------------------------------------------
