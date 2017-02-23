package com.sos.scheduler.engine.tests.jira.js1671

import com.sos.jobscheduler.base.generic.SecretString
import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.jobscheduler.common.auth.{UserAndPassword, UserId}
import com.sos.jobscheduler.common.scalautil.AutoClosing.autoClosing
import com.sos.jobscheduler.common.scalautil.Futures.implicits._
import com.sos.jobscheduler.common.time.ScalaTime._
import org.scalatest.Assertions._
import spray.httpx.UnsuccessfulResponseException

/**
  * @author Joacim Zschimmer
  */
private[js1671] final class WithoutAccessTokenJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    checkWithoutAccessToken()
    checkWithOldTaskAccessToken(spooler.variables.value(WithAccessTokenJob.AccessTokenName))
    false
  }

  private def checkWithoutAccessToken(): Unit = {
    autoClosing(new StandardWebSchedulerClient(spooler.uri)) { client ⇒
      intercept[UnsuccessfulResponseException] {
        client.overview await 10.s
      }
    }
  }

  private def checkWithOldTaskAccessToken(accessToken: String): Unit = {
    assert(accessToken.nonEmpty)
    val credentials = UserAndPassword(UserId.Empty, SecretString(accessToken))
    autoClosing(new StandardWebSchedulerClient(spooler.uri, credentials = Some(credentials))) { client ⇒
      intercept[UnsuccessfulResponseException] {
        client.overview await 10.s
      }
    }
  }
}
