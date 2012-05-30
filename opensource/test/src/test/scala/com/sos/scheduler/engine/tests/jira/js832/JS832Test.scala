package com.sos.scheduler.engine.tests.jira.js832

import com.google.common.io.Files
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderKey}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerEncoding
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import org.slf4j.LoggerFactory
import java.io.File
import scala.collection.mutable

@RunWith(classOf[JUnitRunner])
final class JS832Test extends ScalaSchedulerTest {
  import JS832Test._

  test("When order is finished, Order log should be closed and reopened for next repetition") {
    def logFile(o: OrderKey) = scheduler.getOrderSubsystem.order(o).getLog.getFile
    val eventPipe = controller.newEventPipe
    val firstLines = new mutable.HashSet[String]
    for (i <- 1 to 3) {
      scheduler executeXml <modify_order job_chain={orderKey.jobChainPathString} order={orderKey.idString} at="now"/>
      eventPipe.nextWithCondition[OrderFinishedEvent] { _.getKey == orderKey }
      val line = firstLine(logFile(orderKey))
      firstLines should not contain (line)    // Erste Zeile hat jedesmal einen neuen Zeitstempel
      firstLines += line
    }
  }
}

object JS832Test {
  private val logger = LoggerFactory.getLogger(classOf[JS832Test])
  private val orderKey = OrderKey.of("/test", "1")

  private def firstLine(f: File) = {
    val r = Files.newReader(f, schedulerEncoding)
    try r.readLine
    finally r.close()
  }
}
