package com.sos.scheduler.engine.playground.zschimmer.plugin.watchdog


@deprecated("") trait Terminatable {
    this: Thread =>
    private var terminateReceived = false
    private var sleeping = false

    def terminateThreadReceived = terminateReceived

    protected def sleepThread(ms: Int) {
        synchronized {
            sleeping = true
            try Thread.sleep(ms)
            finally sleeping = false
        }
    }

    def terminateThread() {
        synchronized {
            terminateReceived = true
            if (sleeping)  interrupt()
        }
    }

    def untilTerminated(f: => Unit) {
        while (!terminateReceived)  f
    }

    def close() {
        require(this != Thread.currentThread)
        terminateThread()
        join()
    }
}
