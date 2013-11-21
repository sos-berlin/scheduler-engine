package com.sos.scheduler.engine.kernel.scheduler

import com.google.inject.ImplementedBy
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.security.SchedulerSecurityLevel

@ImplementedBy(classOf[Scheduler])
trait SchedulerXmlCommandExecutor {

  def uncheckedExecuteXml(xml: String): String

  def uncheckedExecuteXml(xml: String, securityLevel: SchedulerSecurityLevel): String

  def executeXml(xml: String): String
}

