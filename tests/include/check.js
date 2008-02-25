// check.js

function check_task_param( name, expected_value )
{
    var value = spooler_task.params( name );
    var line = name + "=" + value;
    
    if( value != expected_value )
    {
        spooler_log.error( line + ",  richtig wäre " + expected_value );
    }
    else
    {
        spooler_log.info( line );
    }
}

var check_js = true;

FEHLER
