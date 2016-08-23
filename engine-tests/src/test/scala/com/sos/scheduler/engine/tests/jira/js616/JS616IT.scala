package com.sos.scheduler.engine.tests.jira.js616

import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderKey}
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js616.JS616IT._
import org.joda.time.DateTimeZone
import org.joda.time.Instant.now
import org.joda.time.format.ISODateTimeFormat
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS616IT extends FunSuite with ScalaSchedulerTest {
  test("JS-616 Bug fix: Order Job does not start when having a single start schedule") {
    withEventPipe { eventPipe ⇒
      val t = now + 3.s
      scheduler executeXml
        <job name="test-shell">
          <script language="shell">exit 0</script>
          <run_time>
            <at at={ISODateTimeFormat.dateHourMinuteSecond.withZone(DateTimeZone.getDefault).print(t)}/>
          </run_time>
        </job>
      eventPipe.nextKeyed[OrderFinished](shellOrderKey)
    }
  }
}


private object JS616IT {
  private val shellOrderKey = OrderKey("/test-shell", "1")
}
