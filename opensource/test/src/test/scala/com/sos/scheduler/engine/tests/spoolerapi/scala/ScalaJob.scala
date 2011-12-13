package com.sos.scheduler.engine.tests.spoolerapi.scala

import sos.spooler.Job_impl

final class ScalaJob extends Job_impl {
    private val myListIterator = Iterator("Step one", "Step two")

    override def spooler_init() = {
        spooler_log.info("spooler_init")
        true
    }

    override def spooler_exit() {
        spooler_log.info("spooler_exit")
    }

    override def spooler_open() = {
        spooler_log.info("spooler_open")
        myListIterator.hasNext
    }

    override def spooler_close() {
        spooler_log.log(spooler_task.params.value("logLevel").toInt, "spooler_close")
    }

    override def spooler_process() = {
        spooler_log.info("spooler_process "+ myListIterator.next)
        myListIterator.hasNext
    }

    override def spooler_on_success() {
        spooler_log.info("spooler_on_success")
    }

    override def spooler_on_error() {
        spooler_log.info("spooler_on_error")
    }
}