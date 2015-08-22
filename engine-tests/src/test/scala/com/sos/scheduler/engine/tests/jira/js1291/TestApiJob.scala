package com.sos.scheduler.engine.tests.jira.js1291

import java.io.File
import java.nio.file.Paths
import org.scalatest.Matchers._

/**
 * @author Joacim Zschimmer
 */
final class TestApiJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    assert(spooler.directory == (Paths.get(".").toAbsolutePath.toString stripSuffix "."))
    checkUnsupportedMethod("sos.spooler.Spooler.include_path") { spooler.include_path }
    checkUnsupportedMethod("sos.spooler.Spooler.ini_path") { spooler.ini_path }
    checkUnsupportedMethod("sos.spooler.Spooler.log_dir") { spooler.log_dir }
    checkUnsupportedMethod("sos.spooler.Spooler.create_xslt_stylesheet") { spooler.create_xslt_stylesheet() }
    checkUnsupportedMethod("sos.spooler.Spooler.create_xslt_stylesheet") { spooler.create_xslt_stylesheet("x") }
    checkUnsupportedMethod("sos.spooler.Spooler.create_xslt_stylesheet") { spooler.create_xslt_stylesheet(new File("x")) }
    checkUnsupportedMethod("sos.spooler.Task.create_subprocess") { spooler_task.create_subprocess() }
    checkUnsupportedMethod("sos.spooler.Task.create_subprocess") { spooler_task.create_subprocess("x") }
    checkUnsupportedMethod("sos.spooler.Task.create_subprocess") { spooler_task.create_subprocess(Array("x")) }
    checkUnsupportedMethod("sos.spooler.Task.priority") { spooler_task.priority }
    spooler_task.set_priority(999)  // Ignored, warning is logged
    checkUnsupportedMethod("sos.spooler.Task.priority_class") { spooler_task.priority_class }
    spooler_task.set_priority_class("TEST-PRIORITY-CLASS")  // Ignored, warning is logged
    false
  }

  private def checkUnsupportedMethod(name: String)(body: ⇒ Unit): Unit =
    intercept[UnsupportedOperationException] { body } .getMessage shouldEqual s"Universal Agent does not support method '$name'"
}
