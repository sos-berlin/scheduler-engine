// spooler_process wird so lange gerufen, bis false zurück gegeben wird

function spooler_task_before()
{
    spooler_log.info( "SPOOLER_TASK_BEFORE()" );
    return true;
}

function spooler_task_after()
{
    spooler_log.info( "SPOOLER_TASK_AFTER()" );
}

function spooler_process_before()
{
    spooler_log.info( "SPOOLER_PROCESS_BEFORE()" );
    return true;
}

function spooler_process_after(process_result) {
    spooler_log.info( "SPOOLER_PROCESS_AFTER(" + process_result + ")" );
    return process_result;
}
