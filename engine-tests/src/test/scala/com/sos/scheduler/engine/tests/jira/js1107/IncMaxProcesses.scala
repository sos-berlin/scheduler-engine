package com.sos.scheduler.engine.tests.jira.js1107

/**
 * Created by fs on 19.05.2014.
 */
class IncMaxProcesses extends sos.spooler.Job_impl {
  override def spooler_process() = {

    val process_classes = spooler.process_classes
    process_classes.process_class("pc1").set_max_processes(1)
    process_classes.process_class("pc2").set_max_processes(1)

    false
  }
}
