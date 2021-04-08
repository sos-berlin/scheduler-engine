package com.sos.scheduler.engine.tests.jira.js1926

import com.sos.scheduler.engine.tests.jira.js1926.TestJob._

final class TestJob extends sos.spooler.Job_impl
{
  override def spooler_process() = {
    spooler_task.set_history_field("SHORTEXTRA", shortString)
    spooler_task.set_history_field("LONGEXTRA", longString)
    false
  }
}

private object TestJob
{
  def shortString = "-" * 1024
  def longString = "+" * 1024*1024
}
