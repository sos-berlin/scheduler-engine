package com.sos.scheduler.engine.tests.scheduler.scheduler

import com.sos.scheduler.engine.common.xml.XmlUtils.loadXml
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class SchedulerSelfIT extends FreeSpec with ScalaSchedulerTest {

  "Valid XML command" in {
    loadXml(scheduler.executeXml("<show_state/>"))
  }

  "Unknown XML command" in {
    intercept[SchedulerException] {
      controller.toleratingErrorCodes(Set(MessageCode("Z-JAVA-105"))) {
        scheduler.executeXml("<UNKNOWN/>")
      }
    }
  }
}
