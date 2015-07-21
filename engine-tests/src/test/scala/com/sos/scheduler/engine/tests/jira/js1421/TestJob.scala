package com.sos.scheduler.engine.tests.jira.js1421

import com.google.common.io.Files.touch
import java.io.File

/**
 * @author Joacim Zschimmer
 */
final class TestJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    touch(new File(spooler_task.params.value("SIGNALFILE")))
    Thread.sleep(120*1000)
    sys.error("NOT KILLED")
  }
}
