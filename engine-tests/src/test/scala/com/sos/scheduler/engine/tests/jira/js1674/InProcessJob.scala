package com.sos.scheduler.engine.tests.jira.js1674

import com.google.common.io.Closer
import com.sos.scheduler.engine.base.generic.SecretString
import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.scheduler.engine.common.auth.{UserAndPassword, UserId}
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event.Snapshot
import com.sos.scheduler.engine.data.scheduler.{SchedulerId, SchedulerOverview}

/**
  * @author Joacim Zschimmer
  */
final class InProcessJob extends sos.spooler.Job_impl {

  private implicit val closer = Closer.create()
  private lazy val client = {
    val credentials = UserAndPassword(UserId.Empty, SecretString(spooler_task.web_service_access_token))
    new StandardWebSchedulerClient(spooler.uri, credentials = Some(credentials)).closeWithCloser
  }
  private var step = 0

  override def spooler_exit() = closer.close()

  override def spooler_process() = {
    step += 1
    val overview = readOverview()
    spooler_log.info(overview.toString)
    sleep(1.s)
    step < 2 && {
      spooler_task.set_delay_spooler_process(1)
      true
    }
  }

  private def readOverview(): SchedulerOverview = {
    val Snapshot(_, overview) = client.overview await 10.s
    assert(overview.schedulerId == SchedulerId(spooler.id))
    overview
  }
}
