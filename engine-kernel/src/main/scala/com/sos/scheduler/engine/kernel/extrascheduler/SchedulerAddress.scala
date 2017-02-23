package com.sos.scheduler.engine.kernel.extrascheduler

import com.sos.jobscheduler.base.generic.IsString

final case class SchedulerAddress(interface: String, port: Int) extends IsString {
  def string = s"$interface:$port"
}
