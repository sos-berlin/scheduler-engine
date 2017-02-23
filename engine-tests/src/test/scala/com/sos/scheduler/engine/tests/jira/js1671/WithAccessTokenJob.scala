package com.sos.scheduler.engine.tests.jira.js1671

import com.sos.jobscheduler.base.generic.SecretString
import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.jobscheduler.common.auth.{UserAndPassword, UserId}
import com.sos.jobscheduler.common.scalautil.AutoClosing.autoClosing
import com.sos.jobscheduler.common.scalautil.Futures.implicits._
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.event.Snapshot
import com.sos.scheduler.engine.test.TestEnvironment
import com.sos.scheduler.engine.tests.jira.js1671.WithAccessTokenJob._

/**
  * @author Joacim Zschimmer
  */
private[js1671] final class WithAccessTokenJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    val credentials = UserAndPassword(UserId.Empty, SecretString(spooler_task.web_service_access_token))
    autoClosing(new StandardWebSchedulerClient(spooler.uri, credentials = Some(credentials))) { client ⇒
      val Snapshot(_, overview) = client.overview await 10.s
      assert(overview.schedulerId == TestEnvironment.TestSchedulerId)
    }

    spooler.variables.set_value(AccessTokenName, spooler_task.web_service_access_token)  // For next test
    false
  }
}

private[js1671] object WithAccessTokenJob {
  val AccessTokenName = "accessToken"
}
