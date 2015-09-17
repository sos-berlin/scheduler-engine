package com.sos.scheduler.engine.tests.jira.js1454.short

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits.RichFile
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.log.InfoLogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1454.short.JS1454IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1454 Keep-alive packets for classic Agent TCP communication.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1454IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val tcpPort = findRandomFreeTcpPort()
  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-tcp-port=$tcpPort"),
    logCategories = s"$KeepAliveLogCategory")

  "Busy communcation may suppress keep-alive" in {
    writeConfigurationFile(ProcessClassPath("/test-agent"), <process_class remote_scheduler={s"127.0.0.1:$tcpPort"}/>)
    autoClosing(controller.newEventPipe()) { events ⇒
      runJobAndWaitForEnd(JobPath("/test-busy"))
      val keepAliveCount = events.queued[InfoLogEvent] count { _.codeOption contains MessageCode("SCHEDULER-727") }  // scheduler.agent.keep_alive=TEST floods the line with keep-alive spaces
      assert(keepAliveCount >= 20)
    }
    val schedulerLog = testEnvironment.schedulerLog.contentString
    // How to test this automatically??? Manual test was successful.
    assert(schedulerLog contains s"{$KeepAliveLogCategory} Stopped")
  }

  "Keep-alives when there is no communcation" in {
    runJobAndWaitForEnd(JobPath("/test-sleep"))
    val schedulerLog = testEnvironment.schedulerLog.contentString
    assert(schedulerLog contains s"{$KeepAliveLogCategory} Received")
    assert(schedulerLog contains s"{$KeepAliveLogCategory} Stopped")
  }
}

private object JS1454IT {
  private val KeepAliveLogCategory = "object_server.keep_alive"
}
