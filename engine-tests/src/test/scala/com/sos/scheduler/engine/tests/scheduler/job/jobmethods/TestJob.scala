package com.sos.scheduler.engine.tests.scheduler.job.jobmethods

import com.sos.scheduler.engine.tests.scheduler.job.jobmethods.TestJob._

/**
 * @author Joacim Zschimmer
 */
final class TestJob extends sos.spooler.Job_impl {

  private var stepCount = 0

  override def spooler_init() = run(SpoolerInitName)

  override def spooler_exit() = run(SpoolerExitName)

  override def spooler_open() = run(SpoolerOpenName)

  override def spooler_close() = run(SpoolerCloseName)

  override def spooler_process() = {
    stepCount += 1
    run(SpoolerProcessName) && stepCount == 1
  }

  override def spooler_on_error() = run(SpoolerOnErrorName)

  override def spooler_on_success() = run(SpoolerOnSuccessName)

  private def run(name: String): Boolean = {
    spooler_log.info(s">$name< CALLED")
    spooler_task.params.value(name).toBoolean
  }
}

object TestJob {
  val SpoolerInitName = "spooler_init"
  val SpoolerExitName = "spooler_exit"
  val SpoolerOpenName = "spooler_open"
  val SpoolerCloseName = "spooler_close"
  val SpoolerProcessName = "spooler_process"
  val SpoolerOnErrorName = "spooler_on_error"
  val SpoolerOnSuccessName = "spooler_on_success"
}
