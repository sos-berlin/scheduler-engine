package com.sos.scheduler.engine.tests.scheduler.job.start_when_directory_changed

import StartWhenDirectoryChangedIT.TriggeredFilesName

class AJob extends sos.spooler.Job_impl {
  override def spooler_process() = {
    spooler_task.params().set_value(TriggeredFilesName, spooler_task.trigger_files)
    false
  }
}
