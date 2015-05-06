// Same logic as in Java TestJob

var stepCount = 0

function spooler_init() {
    return run("spooler_init")
}

function spooler_exit() {
    return run("spooler_exit")
}

function spooler_open() {
    return run("spooler_open")
}

function spooler_close() {
    return run("spooler_close")
}

function spooler_process() {
    stepCount += 1
    return run("spooler_process") && stepCount == 1
}

function spooler_on_error() {
    return run("spooler_on_error")
}

function spooler_on_success() {
    return run("spooler_on_success")
}

function run(name) {
    spooler_log.info(">"+ name +"< CALLED")
    switch (spooler_task.params.value(name)) {
        case "true": return true;
        case "false": return false;
        case o: throw new Error("TEST EXCEPTION")
    }
}
