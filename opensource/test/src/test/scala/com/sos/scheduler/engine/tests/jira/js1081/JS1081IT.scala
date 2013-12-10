package com.sos.scheduler.engine.tests.jira.js1081

import JS1081IT._
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderKey}
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.joda.time.Instant.now
import org.joda.time.format.DateTimeFormat
import org.joda.time.{Instant, LocalTime, Duration}
import org.junit.runner.RunWith
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/** JS-1081 */
@RunWith(classOf[JUnitRunner])
final class JS1081IT extends ScalaSchedulerTest {

  test("period with begin and absolute_repeat=1") {
    autoClosing(controller.newEventPipe()) { eventPipe =>
      val absoluteRepeat = 2.s
      val instants = absoluteRepeatedInstants(now() + 1.s, absoluteRepeat)
      val myOrderElem = orderElem(aOrderKey, begin = instants.head.toDateTime.toLocalTime, absoluteRepeat = absoluteRepeat)
      scheduler executeXml myOrderElem
      for (i <- 0 to 2) {
        withClue(s"Instant $i, $myOrderElem") {
          scheduler.instance[OrderSubsystem].order(aOrderKey).nextInstantOption should equal (Some(instants(i)))
          eventPipe.nextKeyed[OrderFinishedEvent](aOrderKey)
        }
      }
    }
  }
}


private object JS1081IT {
  private val testJobChainPath = JobChainPath.of("/test")
  private val aOrderKey = testJobChainPath orderKey "A"
  private val hhmmssFormatter = DateTimeFormat forPattern "HH:mm:ss"

  private def absoluteRepeatedInstants(start: Instant, absoluteRepeat: Duration): Stream[Instant] =
    Stream from 0 map { i => new Instant(((start.toDateTime + 999.ms) withMillisOfSecond 0) + i * absoluteRepeat ) }

  private def orderElem(orderKey: OrderKey, begin: LocalTime, absoluteRepeat: Duration) =
    <order job_chain={orderKey.jobChainPathString} id={orderKey.idString}>
      <run_time>
         <period absolute_repeat={absoluteRepeat.getStandardSeconds.toString} begin={hhmmssFormatter print begin}/>
      </run_time>
    </order>
}
