package com.sos.scheduler.engine.tests.scheduler.config.schema

import com.sos.scheduler.engine.kernel.util.Time
import com.sos.scheduler.engine.test.TestSchedulerController
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.matchers.ShouldMatchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class SchemaTest extends FunSuite {
  import SchemaTest._

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

  test("-show-xml-schema") {
    pending
//    runScheduler { controller =>
//      controller.activateScheduler("-show-xml-schema")
//    }
  }

  test("-use-schema-xml") {
    pending
    //val schemaFile = ...
//    runScheduler { controller =>
//      val x = intercept[RuntimeException] { controller.activateScheduler("-use-xml-schema="+schemaFile) }
//      x.getMessage should (include ("LIBXML2-007") or include ("SAXParseException"))
//    }
  }
}

object SchemaTest {
  private def runScheduler(f: TestSchedulerController => Unit) {
    val controller = TestSchedulerController.of(classOf[SchemaTest])
    try f(controller)
    finally if (controller.isStarted) {
      controller.terminateScheduler()
      controller.waitForTermination(Time.eternal)
    }
  }
}
