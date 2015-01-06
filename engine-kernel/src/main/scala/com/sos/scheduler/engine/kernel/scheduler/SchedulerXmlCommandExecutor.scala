package com.sos.scheduler.engine.kernel.scheduler

import com.google.inject.ImplementedBy
import com.sos.scheduler.engine.kernel.security.SchedulerSecurityLevel
import com.sos.scheduler.engine.kernel.Scheduler

@ImplementedBy(classOf[Scheduler])
trait SchedulerXmlCommandExecutor {
  def uncheckedExecuteXml(xml: String): String

  /** @param clientHostName Hostname, IP-Nummer oder "" */
  def uncheckedExecuteXml(xml: String, securityLevel: SchedulerSecurityLevel, clientHostName: String): String

  def executeXml(xml: String): String
}
