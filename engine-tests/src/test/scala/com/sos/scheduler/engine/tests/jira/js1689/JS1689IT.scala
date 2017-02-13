package com.sos.scheduler.engine.tests.jira.js1689

import com.sos.scheduler.engine.test.TestEnvironment.TestSchedulerId
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1689IT extends FreeSpec with ScalaSchedulerTest {

  "JS-1689 Default C++ mail settings in Scala" in {
    assert((scheduler.mailDefaults - "from_name") == Map(
      "mail_on_error" → "1",
      "mail_on_warning" → "1",
      "smtp" → "TEST-SMTP",
      "mail.smtp.port" → "2525",
      "mail.smtp.user" → "TEST-USER",
      "mail.smtp.password" → "TEST-PASSWORD",
      "to" → "TEST-TO",
      "from" → "TEST-FROM",
      "bcc" → "TEST-BCC",
      "cc" → "TEST-CC",
      "subject" → "TEST-SUBJECT",
      "queue_dir" → "TEST-QUEUE-DIR",
      "queue_only" → "0"))
    assert(scheduler.mailDefaults("from_name") startsWith "Scheduler ")
    assert(scheduler.mailDefaults("from_name") contains s" -id=$TestSchedulerId")
  }
}
