package com.sos.scheduler.engine.tests.jira.js1291

import com.sos.jobscheduler.common.scalautil.Collections.emptyToNone
import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.tests.jira.js1291.JS1291AgentIT.SignalName
import java.nio.charset.StandardCharsets.ISO_8859_1
import java.nio.file.Paths
import org.scalatest.Assertions._

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
    // JS-1371 Test locally executed proxy code
    assert(spooler_task.stdout_path.nonEmpty)
    assert(spooler_task.stderr_path.nonEmpty)
    assert(spooler_task.stdout_text == Paths.get(spooler_task.stdout_path).contentString(ISO_8859_1))
    assert(spooler_task.stderr_text == Paths.get(spooler_task.stderr_path).contentString(ISO_8859_1))
    assert(spooler_task.stdout_text contains "TEXT FOR STDOUT")
    assert(spooler_task.stderr_text contains "TEXT FOR STDERR")
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
