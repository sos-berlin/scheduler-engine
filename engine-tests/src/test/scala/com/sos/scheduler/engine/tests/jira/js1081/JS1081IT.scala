package com.sos.scheduler.engine.tests.jira.js1081

import com.sos.scheduler.engine.common.time.JodaJavaTimeConversions.implicits.asJavaInstant
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderKey}
import com.sos.scheduler.engine.test.SchedulerTestUtils.order
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1081.JS1081IT._
import org.joda.time.Instant.now
import org.joda.time.format.DateTimeFormat
import org.joda.time.{Duration, Instant, LocalTime}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/** JS-1081 */
@RunWith(classOf[JUnitRunner])
final class JS1081IT extends FreeSpec with ScalaSchedulerTest {

  s"period with begin and absolute_repeat=$AbsoluteRepeat" in {
    withEventPipe { eventPipe ⇒
      val instants = absoluteRepeatedInstants(now() + AbsoluteRepeat, AbsoluteRepeat)
      val myOrderElem = orderElem(AOrderKey, begin = instants.head.toDateTime.toLocalTime, absoluteRepeat = AbsoluteRepeat)
      scheduler executeXml myOrderElem
      for (i ← 0 to 2) {
        withClue(s"Instant $i, $myOrderElem") {
          assert(order(AOrderKey).nextInstantOption contains asJavaInstant(instants(i)))
          eventPipe.nextKeyed[OrderFinished](AOrderKey)
        }
      }
    }
  }
}

private object JS1081IT {
  private val AbsoluteRepeat = 2.s
  private val TestJobChainPath = JobChainPath("/test")
  private val AOrderKey = TestJobChainPath orderKey "A"
  private val hhmmssFormatter = DateTimeFormat forPattern "HH:mm:ss"

  private def absoluteRepeatedInstants(start: Instant, absoluteRepeat: Duration): Stream[Instant] =
    Stream from 0 map { i ⇒ new Instant(((start.toDateTime + 999.ms) withMillisOfSecond 0) + i * absoluteRepeat ) }

  private def orderElem(orderKey: OrderKey, begin: LocalTime, absoluteRepeat: Duration) =
    <order job_chain={orderKey.jobChainPath.string} id={orderKey.id.string}>
      <run_time>
         <period absolute_repeat={absoluteRepeat.getStandardSeconds.toString} begin={hhmmssFormatter print begin}/>
      </run_time>
    </order>
}
