package com.sos.scheduler.kernel.cplusplus.generator.main

/** Gibt Exception nach stderr aus und beendet die VM mit System.exit() */
object MainWithExitCode {
    def main(args: Array[String]) {
        try new Main(args).apply()
        catch {
            case x =>
                showException(x)
                System.exit(1)
        }
    }

    private def showException(x: Throwable) {
        System.err.println("ERROR  " + x)
        //x.printStackTrace(System.err)
    }
}
