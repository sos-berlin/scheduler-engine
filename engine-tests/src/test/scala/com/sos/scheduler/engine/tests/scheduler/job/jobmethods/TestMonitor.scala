package com.sos.scheduler.engine.tests.scheduler.job.jobmethods

import com.sos.scheduler.engine.tests.scheduler.job.jobmethods.TestMonitor._

/**
  * @author Joacim Zschimmer
  */
final class TestMonitor extends sos.spooler.Monitor_impl {

  private var stepCount = 0

  override def spooler_task_before() = run(SpoolerTaskBeforeName)

  override def spooler_task_after() = run(SpoolerTaskAfterName)

  override def spooler_process_before() = run(SpoolerProcessBeforeName)

  override def spooler_process_after(result: Boolean) = {
    spooler_log.info(s">$SpoolerProcessAfterName< CALLED ($result)")
    stepCount += 1
    stepCount == 1 && (spooler_task.params.value(SpoolerProcessAfterName) match {
      case KeepResult ⇒ result
      case InvertResult ⇒ !result
    })
  }

  private def run(name: String): Boolean = {
    spooler_log.info(s">$name< CALLED")
    spooler_task.params.value(name).toBoolean
  }
}

private[jobmethods] object TestMonitor {
  val SpoolerTaskBeforeName = "spooler_task_before"
  val SpoolerTaskAfterName = "spooler_task_after"
  val SpoolerProcessBeforeName = "spooler_process_before"
  val SpoolerProcessAfterName = "spooler_process_after"
  val AllMethodNames = Set(SpoolerTaskBeforeName, SpoolerTaskAfterName, SpoolerProcessBeforeName, SpoolerProcessAfterName)
  val KeepResult = "keep"
  val InvertResult = "invert"
}
