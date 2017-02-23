package com.sos.scheduler.engine.tests.scheduler.messagecode

import com.sos.jobscheduler.common.guice.GuiceImplicits._
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.kernel.messagecode.MessageCodeHandler
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class MessageCodeIT extends FreeSpec with ScalaSchedulerTest {

  "MessageCode" in {
    val messageCodeHandler = injector.instance[MessageCodeHandler]
    assert(messageCodeHandler(MessageCode("SCHEDULER-900"), "A", "B", "C") == "SCHEDULER-900 Scheduler A is starting with B, pid=C")
  }
}
