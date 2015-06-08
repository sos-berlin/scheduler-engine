package com.sos.scheduler.engine.tests.jira.js1291

import java.nio.file.Paths
import org.scalatest.Matchers._

/**
 * @author Joacim Zschimmer
 */
final class TestApi extends sos.spooler.Job_impl {

  override def spooler_process() = {
    assert(spooler.directory == (Paths.get(".").toAbsolutePath.toString stripSuffix "."))
    intercept[UnsupportedOperationException] { spooler.include_path } .getMessage shouldEqual "Java Agent does not support method 'sos.spooler.Spooler.include_path'"
    intercept[UnsupportedOperationException] { spooler.ini_path } .getMessage shouldEqual "Java Agent does not support method 'sos.spooler.Spooler.ini_path'"
    intercept[UnsupportedOperationException] { spooler.log_dir } .getMessage shouldEqual "Java Agent does not support method 'sos.spooler.Spooler.log_dir'"
    false
  }
}
