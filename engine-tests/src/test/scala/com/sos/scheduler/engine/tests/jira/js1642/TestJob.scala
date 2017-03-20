package com.sos.scheduler.engine.tests.jira.js1642

import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.tests.jira.js1642.TestJob._
import java.nio.file.Files.exists
import java.nio.file.Paths

/**
  * @author Joacim Zschimmer
  */
private[js1642] final class TestJob extends sos.spooler.Job_impl {

  override def spooler_process = {
    spooler_task.order.set_state_text("TestJob")
    val barrierFile = Paths.get(spooler.variables.value(BarrierFileVariableName))
    require(barrierFile.toString.nonEmpty)
    while (exists(barrierFile)) {
      sleep(100.ms)
    }
    true
  }
}

private[js1642] object TestJob {
  val BarrierFileVariableName = "TEST-BARRIER"
}
