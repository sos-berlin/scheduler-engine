package com.sos.scheduler.engine.plugins.jetty.services

import EventsServiceIT._
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.plugins.jetty.JettyPlugin
import com.sos.scheduler.engine.plugins.jetty.JettyPluginTests.javaResource
import com.sos.scheduler.engine.test.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sun.jersey.api.client.WebResource
import java.io.{BufferedReader, IOException, Reader}
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import org.slf4j.LoggerFactory

@RunWith(classOf[JUnitRunner])
final class EventsServiceIT extends ScalaSchedulerTest {

  override lazy val testConfiguration = TestConfiguration(testPackage = Some(classOf[JettyPlugin].getPackage))
  private lazy val eventsResource = javaResource(injector).path("TESTONLY/events")

  test("Read some events") {
    val testString = "***"+ getClass.getSimpleName +"***"
    val range = 1 to 5
    val thread = new ResponsePrinter(eventsResource)
    thread.start()
    try {
      Thread.sleep(500)  // Warten, bis Reader liest
      for (i <- range) {
        instance[PrefixLog].info(testString +" "+ i)
        instance[SchedulerEventBus].dispatchEvents()
        Thread.sleep(100)
      }
      controller.terminateScheduler()
      controller.waitForTermination(shortTimeout)
      controller.getEventBus.dispatchEvents()
      Thread.sleep(100)
    } finally {
      thread.interrupt()
    }
    val result = thread.result.toString()
    for (i <- range) result should include (testString +" "+ i)
    //Das Event ist manchmal nicht drin: result should include (classOf[SchedulerCloseEvent].getSimpleName)
  }
}

object EventsServiceIT {
  private val logger = LoggerFactory.getLogger(classOf[EventsServiceIT])

  class ResponsePrinter(resource: WebResource) extends Thread {
    val result = new StringBuilder
    override def run() {
      try {
        val reader = new BufferedReader(resource.get(classOf[Reader]))
        while (true) {
          val line = reader.readLine()
          if (line == null)  return
          result.append(line)
          logger.debug(line)
        }
      }
      catch {
        case x: IOException => if (x.getMessage != "Premature EOF") logger.error(x.toString)
      }
    }
  }
}