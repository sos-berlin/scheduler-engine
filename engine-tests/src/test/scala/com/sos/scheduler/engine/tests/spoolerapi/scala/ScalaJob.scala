package com.sos.scheduler.engine.tests.spoolerapi.scala

import sos.spooler.Job_impl

final class ScalaJob extends Job_impl {
    private val myListIterator = Iterator("Step one", "Step two")

    private lazy val logLevel = spooler_task.params.value("logLevel").toInt  // lazy, weil spooler_task verzögert gesetzt wird.

    override def spooler_init() = {
        trace("spooler_init")
        true
    }

    override def spooler_exit() {
        trace("spooler_exit")
    }

    override def spooler_open() = {
        trace("spooler_open")
        myListIterator.hasNext
    }

    override def spooler_close() {
        trace("spooler_close")
        spooler_log.log(logLevel, "spooler_close")
    }

    override def spooler_process() = {
        trace("spooler_process")
        val a = myListIterator.next()
        spooler_log.info(a)
        myListIterator.hasNext
    }

    override def spooler_on_success() {
        trace("spooler_on_success")
    }

    override def spooler_on_error() {
        trace("spooler_on_error")
    }

    private def trace(call: String) {
        val v = "test."+ logLevel +"."+ call
        val n = spooler.variables.value(v) match {
            case "" => 0
            case o => o.toInt
        }
        spooler.variables.set_value(v, (n+1).toString)
    }
}