package com.sos.scheduler.engine.tests.jira.js1776

import java.io.File.pathSeparator

/**
  * @author Joacim Zschimmer
  */
final class TestJob extends sos.spooler.Job_impl
{
  override def spooler_process() = {
    spooler_log.info("java.class.path=" + sys.props("java.class.path").stripSuffix(pathSeparator) + "<--")
    false
  }
}
