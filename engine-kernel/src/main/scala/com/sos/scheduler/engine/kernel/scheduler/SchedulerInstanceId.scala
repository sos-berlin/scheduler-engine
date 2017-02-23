package com.sos.scheduler.engine.kernel.scheduler

import com.sos.jobscheduler.base.generic.IsString

/**
  * @author Joacim Zschimmer
  */
final case class SchedulerInstanceId(string: String) extends IsString
