package com.sos.scheduler.engine.tests.jira.js846

final class TestJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    val order = spooler_task.order
    order.title match {
      case "" => order.set_title("x" * 200)
      case o => order.set_title(o +"+")
    }
    true
  }
}
