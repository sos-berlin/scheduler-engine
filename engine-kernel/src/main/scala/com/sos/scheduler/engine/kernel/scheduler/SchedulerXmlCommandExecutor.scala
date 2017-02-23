package com.sos.scheduler.engine.kernel.scheduler

import com.google.inject.ImplementedBy
import com.sos.jobscheduler.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.data.xmlcommands.XmlCommand
import com.sos.scheduler.engine.kernel.scheduler.SchedulerXmlCommandExecutor.Result
import com.sos.scheduler.engine.kernel.security.SchedulerSecurityLevel
import com.sos.scheduler.engine.kernel.Scheduler

@ImplementedBy(classOf[Scheduler])
trait SchedulerXmlCommandExecutor {
  def uncheckedExecuteXml(xml: String): String

  /** @param clientHostName Hostname, IP-Nummer oder "" */
  def uncheckedExecuteXml(xml: String, securityLevel: SchedulerSecurityLevel, clientHostName: String): String

  def executeXml(o: XmlCommand): Result

  def executeXml(xml: String): String
}

object SchedulerXmlCommandExecutor {
  final case class Result(string: String) {
    lazy val elem: scala.xml.Elem = SafeXML.loadString(string)
    lazy val answer = elem \ "answer"
  }
}
