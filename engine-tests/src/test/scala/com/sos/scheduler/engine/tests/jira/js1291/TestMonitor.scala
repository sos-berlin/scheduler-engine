package com.sos.scheduler.engine.tests.jira.js1291

import com.sos.scheduler.engine.common.scalautil.Collections.emptyToNone
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.tests.jira.js1291.JS1291AgentIT.SignalName
import java.nio.file.Paths

final class TestMonitor extends sos.spooler.Monitor_impl {

  override def spooler_process_before() = {
    spooler_log.info("SPOOLER_PROCESS_BEFORE")
    for (o <- emptyToNone(spooler_task.params.value(SignalName))) {
      Paths.get(o).append("x")
    }
    Thread.sleep(2000)
    true
  }

  override def spooler_process_after(returnCode: Boolean) = {
    spooler_log.info("SPOOLER_PROCESS_AFTER")
    returnCode
  }

  override def spooler_task_before(): Boolean = {
    spooler_log.info("SPOOLER_TASK_BEFORE")
    true
  }

  override def spooler_task_after(): Unit = {
    spooler_log.info("SPOOLER_TASK_AFTER")
  }
}
