package com.sos.scheduler.engine.tests.jira.js832

import com.google.common.io.Files
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderKey}
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerEncoding
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import java.io.File
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import scala.collection.mutable

@RunWith(classOf[JUnitRunner])
final class JS832IT extends ScalaSchedulerTest {

  import JS832IT._

  test("When order is finished, Order log should be closed and reopened for next repetition") {
    def logFile(o: OrderKey) = instance[OrderSubsystem].order(o).getLog.getFile
    val eventPipe = controller.newEventPipe
    val firstLines = new mutable.HashSet[String]
    for (i <- 1 to 3) {
      scheduler executeXml <modify_order job_chain={orderKey.jobChainPathString} order={orderKey.idString} at="now"/>
      eventPipe.nextWithCondition[OrderFinishedEvent] { _.orderKey == orderKey }
      val line = firstLine(logFile(orderKey))
      firstLines should not contain (line)    // Erste Zeile hat jedesmal einen neuen Zeitstempel
      firstLines += line
    }
  }
}

object JS832IT {
  private val orderKey = OrderKey.of("/test", "1")

  private def firstLine(f: File) = {
    val r = Files.newReader(f, schedulerEncoding)
    try r.readLine
    finally r.close()
  }
}
