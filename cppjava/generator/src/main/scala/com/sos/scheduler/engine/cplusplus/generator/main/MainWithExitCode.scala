package com.sos.scheduler.engine.cplusplus.generator.main

/** Gibt Exception nach stderr aus und beendet die VM mit System.exit() */
object MainWithExitCode {
    def main(args: Array[String]) {
        try Main.main(args)
        catch { case x =>
            showException(x)
            System.exit(1)
        }
    }

    private def showException(x: Throwable) {
        System.err.println("ERROR  " + x)
        //x.printStackTrace(System.err)
    }
}
