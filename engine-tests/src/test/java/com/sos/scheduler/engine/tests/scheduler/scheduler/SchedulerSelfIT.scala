package com.sos.scheduler.engine.tests.scheduler.scheduler

import com.sos.scheduler.engine.common.xml.CppXmlUtils.loadXml
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.test.SchedulerTestUtils.interceptSchedulerError
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class SchedulerSelfIT extends FreeSpec with ScalaSchedulerTest {

  "Valid XML command" in {
    loadXml(scheduler.executeXml("<show_state/>"))
  }

  "Unknown XML command" in {
    interceptSchedulerError(MessageCode("Z-JAVA-105")) {
      scheduler.executeXml("<UNKNOWN/>")
    }
  }
}
