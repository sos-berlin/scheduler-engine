package com.sos.scheduler.engine.kernel

import com.sos.scheduler.engine.client.api.CommandClient
import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.kernel.DirectCommandClient._
import java.io.ByteArrayInputStream
import javax.inject.{Inject, Singleton}
import scala.concurrent.{ExecutionContext, Future}
import scala.xml.Elem

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class DirectCommandClient @Inject private(scheduler: Scheduler)(implicit executionContext: ExecutionContext)
extends CommandClient {

  /**
    * Like uncheckedExecuteXml, but fails when response contains an ERROR element.
    *
    * @return XML response
    */
  def executeXml(xmlBytes: Array[Byte]) =
    Future { bytesToXmlString(xmlBytes) } flatMap scheduler.executeXmlFuture

  /**
    * @return XML response
    */
  def uncheckedExecuteXml(xmlBytes: Array[Byte]) =
    Future { bytesToXmlString(xmlBytes) } flatMap scheduler.uncheckedExecuteXmlFuture

  /**
    * Like uncheckedExecuteXml, but fails when response contains an ERROR element.
    *
    * @return XML response
    */
  def execute(elem: Elem) = scheduler.executeXmlFuture(elem.toString)

  /**
    * @return XML response
    */
  def uncheckedExecute(elem: Elem) = scheduler.uncheckedExecuteXmlFuture(elem.toString)
}

object DirectCommandClient {
  private def bytesToXmlString(bytes: Array[Byte]) = SafeXML.load(new ByteArrayInputStream(bytes)).toString()
}
