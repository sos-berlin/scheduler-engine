package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginJerseyTester
import com.sos.scheduler.engine.plugins.webservice.services.EventsServiceIT._
import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import java.io.{BufferedReader, IOException, Reader}
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class EventsServiceIT extends FunSuite with ScalaSchedulerTest with JettyPluginJerseyTester {

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    testPackage = Some(Tests.testPackage))

  test("Read some events") {
    val testString = s"+++ EventsServiceIT +++"
    val range = 1 to 5
    val thread = new ResponsePrinter(get[Reader]("/jobscheduler/engine/TESTONLY/events"))
    thread.start()
    try {
      Thread.sleep(1000)  // Warten, bis Reader liest
      for (i <- range) {
        instance[PrefixLog].info(s"$testString $i")
        instance[SchedulerEventBus].dispatchEvents()
        Thread.sleep(100)
      }
      controller.terminateScheduler()
      controller.waitForTermination()
      eventBus.dispatchEvents()
      Thread.sleep(100)
    } finally {
      thread.interrupt()
    }
    val result = thread.result.toString()
    for (i <- range) result should include (s"$testString $i")
    // Das Event ist manchmal nicht drin: result should include (classOf[SchedulerCloseEvent].getSimpleName)
  }
}

object EventsServiceIT {
  private val logger = Logger(getClass)

  private class ResponsePrinter(reader: ⇒ Reader) extends Thread {
    val result = new StringBuilder
    override def run(): Unit = {
      try {
        val bufferedReader = new BufferedReader(reader)
        while (true) {
          val line = bufferedReader.readLine()
          if (line == null)  return
          result.append(line)
          logger.debug(line)
        }
      }
      catch {
        case x: IOException ⇒ if (x.getMessage != "Premature EOF") logger.error(x.toString)
      }
    }
  }
}
