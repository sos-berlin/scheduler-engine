// $Id: log_categories.js 13686 2008-09-30 13:43:01Z jz $


//----------------------------------------------------------------------------------------------var

var static_log_categories_stylesheet = null;
var _scheduler = null;
var static_log_categories_dom;

//----------------------------------------------------------------------------------------do_onload

function do_onload()
{
    show_log_categories();
}

//------------------------------------------------------------------------------show_log_categories

function show_log_categories()
{
    if( !static_log_categories_stylesheet )  static_log_categories_stylesheet = new Stylesheet( "log_categories.xslt" );
    if( !_scheduler )  _scheduler = new Scheduler();

    window.parent.document.title = "Scheduler " + document.location.host + " Log categories";

    static_log_categories_dom = _scheduler.execute( "<scheduler_log.log_categories.show/>" );
    
    document.getElementById( "stylesheet_output" ).innerHTML = static_log_categories_stylesheet.xml_transform( static_log_categories_dom );
}

//------------------------------------------------------------------------------------execute_input

function execute_input()
{
    var commands = new Array();
    
    var log_categories = static_log_categories_dom.selectNodes( "//log_categories/log_category" );
    
    for( var i = 0; i < log_categories.length; i++ )
    {
        var log_category = log_categories[ i ];
        var path         = log_category.getAttribute( "path" )

        var checkbox = document.getElementById( "checkbox_log_category__" + path );
        if( checkbox )
        {
            var checked = checkbox.checked != false;
            if( checked != ( log_category.getAttribute( "value" ) != '0' ) )
            {
                commands.push( "<scheduler_log.log_categories.set category='" + path + "' value='" + checked + "'/>" );
            }
        }
    }
    
    var delay_input = document.getElementById( "delay_input" );
    if( delay_input ) 
    {
        var delay = 1*delay_input.value;
        if( delay )  commands.push( "<scheduler_log.log_categories.reset delay='" + delay + "'/>" );
    }
    
    if( commands.length > 0 )  
    {
        _scheduler.execute( "<commands>" + commands.join( "" ) + "</commands>" );
    }
    
    show_log_categories();
}

//-----------------------------------------------------------------------------reset_button_onclick

function reset_button_onclick()
{
    _scheduler.execute( "<scheduler_log.log_categories.reset/>" );
    show_log_categories();
}

//-------------------------------------------------------------------------------------------------
