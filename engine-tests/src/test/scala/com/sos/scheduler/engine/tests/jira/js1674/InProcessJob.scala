package com.sos.scheduler.engine.tests.jira.js1674

import com.sos.jobscheduler.base.generic.SecretString
import com.sos.jobscheduler.common.auth.{UserAndPassword, UserId}
import com.sos.jobscheduler.common.scalautil.Futures.implicits._
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.event.Snapshot
import com.sos.jobscheduler.data.scheduler.SchedulerId
import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.scheduler.engine.common.javautils.ScalaInJava.toJavaOptional
import com.sos.scheduler.engine.data.scheduler.{SchedulerOverview, SupervisorUri}
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import java.time.ZoneId
import java.util.Optional
import javax.inject.{Inject, Provider}
import org.scalatest.Assertions._

/**
  * @author Joacim Zschimmer
  */
final class InProcessJob @Inject private(
  schedulerId: SchedulerId,
  zoneId: ZoneId,
  directClient: DirectSchedulerClient,
  supervisorUriOptionProvider: Provider[Option[SupervisorUri]])
extends sos.spooler.Job_impl {

  private lazy val webClient = {
    val credentials = UserAndPassword(UserId.Empty, SecretString(spooler_task.web_service_access_token))
    new StandardWebSchedulerClient(spooler.uri, credentials = Some(credentials))
  }
  private var step = 0

  override def spooler_init() = {
    assert(supervisorUriOptionProvider.get == Some(SupervisorUri("tcp://0.0.0.1:1")))
    assert(toJavaOptional(supervisorUriOptionProvider.get) == Optional.of(SupervisorUri("tcp://0.0.0.1:1")))  // Java example
    assert(SchedulerId(spooler.id) == schedulerId)
    val overview = readOverview(directClient)
    spooler_log.info(s"zoneId=$zoneId, overview=$overview")
    true
  }

  override def spooler_exit() = {
    webClient.close()
    assert(step == 2)
  }

  override def spooler_process() = {
    val overview = readOverview(webClient)
    spooler_log.info(s"overview=$overview")
    sleep(1.s)
    step += 1
    step < 2 && {
      spooler_task.set_delay_spooler_process(1)
      true
    }
  }

  private def readOverview(client: SchedulerClient): SchedulerOverview = {
    val Snapshot(_, overview) = client.overview await 10.s
    assert(overview.schedulerId == SchedulerId(spooler.id))
    overview
  }
}
