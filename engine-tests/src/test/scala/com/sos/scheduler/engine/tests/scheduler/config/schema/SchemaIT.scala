package com.sos.scheduler.engine.tests.scheduler.config.schema

import com.sos.scheduler.engine.test.ProvidesTestEnvironment
import com.sos.scheduler.engine.test.scalatest.HasCloserBeforeAndAfterAll
import java.io.File
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import com.sos.scheduler.engine.test.configuration.TestConfiguration

@RunWith(classOf[JUnitRunner])
final class SchemaIT
    extends FunSuite
    with HasCloserBeforeAndAfterAll
    with ProvidesTestEnvironment {

  protected lazy val testConfiguration = TestConfiguration(testClass = getClass)

  test("XML Schema validation detects invalid scheduler.xml") {
    runScheduler(activate = false) { controller =>
      val x = intercept[RuntimeException] { controller.activateScheduler() }
      x.getMessage should (include ("LIBXML2-007") or include ("SAXParseException"))
    }
  }

  test("XML Schema validation suppressed") {
    runScheduler(activate = false) { controller =>
      controller.activateScheduler("-validate-xml-")
    }
  }

  test("-use-xml-schema") {
    runScheduler(activate = false) { controller =>
      val e = intercept[Exception] {
        controller.activateScheduler("-use-xml-schema="+ new File(controller.environment.configDirectory, "test-xsd.xml"))
      }
      assert(e.getMessage contains "org.xml.sax.SAXParseException")
      // English only: e.getMessage should include ("Cannot find the declaration")   // Das ist wegen Spooler::configuration_for_single_job_script
    }
  }
}
