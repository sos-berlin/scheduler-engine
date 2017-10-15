package com.sos.scheduler.engine.tests.jira.slow

/**
  * @author Joacim Zschimmer
  */
final class SlowJob extends sos.spooler.Job_impl with ApiTimer {

  override def spooler_process() = doVariableCalls(1000)
}
