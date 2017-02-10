package com.sos.scheduler.engine.tests.jira.js1689.defaults

import com.sos.scheduler.engine.test.TestEnvironment.TestSchedulerId
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1689defaultsIT extends FreeSpec with ScalaSchedulerTest {

  "JS-1689 Default C++ mail settings in Scala" in {
    assert((scheduler.mailDefaults - "from_name") == Map(
      "mail_on_error" → "0",
      "mail_on_warning" → "0",
      "smtp" → "-",
      "subject" → "",
      "to" → "",
      "from" → "",
      "bcc" → "",
      "cc" → "",
      "queue_dir" → "-",
      "queue_only" → "0"))
    assert(scheduler.mailDefaults("from_name") startsWith "Scheduler ")
    assert(scheduler.mailDefaults("from_name") contains s" -id=$TestSchedulerId")
  }
}
