package com.sos.scheduler.engine.scalajs.frontend.data

import scala.scalajs.js
/**
  * @author Joacim Zschimmer
  */
final case class SchedulerOverview(
  version: String,
  startedAt: String,//js.Date,
  schedulerId: SchedulerId,
  httpPort: Option[Int],
  udpPort: Option[Int],
  pid: Int,
  state: SchedulerState)

object SchedulerOverview {
  def apply(dyn: js.Dynamic): SchedulerOverview =
    SchedulerOverview(
      version = dyn.version.asInstanceOf[String],
      startedAt = dyn.startedAt.asInstanceOf[String],
      schedulerId = SchedulerId(dyn.schedulerId.asInstanceOf[String]),
      httpPort = None,
      udpPort = None,
      pid = dyn.pid.asInstanceOf[Int],
      state = SchedulerState(dyn.state.asInstanceOf[String]))
}
