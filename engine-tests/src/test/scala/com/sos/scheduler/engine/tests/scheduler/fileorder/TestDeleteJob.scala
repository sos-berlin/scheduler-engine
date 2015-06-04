package com.sos.scheduler.engine.tests.scheduler.fileorder

import java.nio.file.Files.delete
import java.nio.file.Paths

/**
 * @author Joacim Zschimmer
 */
final class TestDeleteJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    val file = Paths.get(spooler_task.order.params.value("scheduler_file_path"))
    delete(file)
    true
  }
}
