package com.sos.scheduler.engine.kernel.extrascheduler

import com.sos.scheduler.engine.data.base.IsString

final case class SchedulerAddress(interface: String, port: Int) extends IsString {
  def string = s"$interface:$port"
}
