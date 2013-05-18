package com.sos.scheduler.engine.tests.scheduler.config.schema

import SchemaIT._
import com.sos.scheduler.engine.common.time.Time
import com.sos.scheduler.engine.test.{TestConfiguration, TestSchedulerController}
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import java.io.File

@RunWith(classOf[JUnitRunner])
final class SchemaIT extends FunSuite {

  test("XML Schema validation detects invalid scheduler.xml") {
    runScheduler { controller =>
      val x = intercept[RuntimeException] { controller.activateScheduler() }
      x.getMessage should (include ("LIBXML2-007") or include ("SAXParseException"))
    }
  }

  test("XML Schema validation suppressed") {
    runScheduler { controller =>
      controller.activateScheduler("-validate-xml-")
    }
  }

  test("-use-xml-schema") {
    runScheduler { controller =>
      val e = intercept[Exception] { controller.activateScheduler("-use-xml-schema="+ new File(controller.environment.configDirectory, "test-xsd.xml")) }
      e.getMessage should include ("cvc-elt.1: Cannot find the declaration")   // Das ist wegen Spooler::configuration_for_single_job_script
    }
  }
}

object SchemaIT {
  private def runScheduler(f: TestSchedulerController => Unit) {
    val controller = new TestSchedulerController(classOf[SchemaIT], TestConfiguration())
    try f(controller)
    finally if (controller.isStarted) {
      controller.terminateScheduler()
      try controller.waitForTermination(Time.eternal)
      finally controller.close()
    }
  }
}
