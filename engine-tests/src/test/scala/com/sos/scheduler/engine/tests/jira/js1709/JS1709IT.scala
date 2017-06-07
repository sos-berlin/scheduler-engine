package com.sos.scheduler.engine.tests.jira.js1709

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import java.time.format.DateTimeFormatterBuilder
import java.time.temporal.ChronoField.{HOUR_OF_DAY, MINUTE_OF_HOUR, SECOND_OF_MINUTE}
import java.time.{ZoneId, ZonedDateTime}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * JS-1709 Passed &lt;period single_start> inhibits following &lt;period absolute_repeat>.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1709IT extends FreeSpec with ScalaSchedulerTest {

  "absolute_repeat after passed single_start is respected" in {
    val zoneId = ZoneId.of(Scheduler.defaultTimezoneId)
    val format = new DateTimeFormatterBuilder()
      .appendValue(HOUR_OF_DAY, 2).appendLiteral(':')
      .appendValue(MINUTE_OF_HOUR, 2).appendLiteral(':')
      .appendValue(SECOND_OF_MINUTE, 2)
      .toFormatter.withZone(zoneId)
    val t1 = ZonedDateTime.now(zoneId) plusSeconds 2
    val t2 = t1 plusSeconds 2
    val absoluteRepeat = 10
    val t3 = t2 plusSeconds 1 + absoluteRepeat   // The fix works only if second <period> (with absolute_repeat) calculates more than one start !!!
    val orderKey = JobChainPath("/test") orderKey "1"
    controller.withEventPipe { eventPipe ⇒
      runOrder(OrderCommand(orderKey, xmlChildren =
        <run_time>
          <period single_start={t1.format(format)}/>
          <period begin={t2.format(format)} end={t3.format(format)} absolute_repeat={absoluteRepeat.toString}/>
        </run_time>))
      eventPipe.next[OrderFinished](orderKey)
      eventPipe.next[OrderFinished](orderKey)
    }
  }
}
