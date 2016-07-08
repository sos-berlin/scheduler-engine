package com.sos.scheduler.engine.kernel

import akka.util.ByteString
import com.sos.scheduler.engine.client.api.CommandClient
import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.kernel.DirectCommandClient._
import java.io.ByteArrayInputStream
import scala.concurrent.{ExecutionContext, Future}
import scala.xml.Elem

/**
  * @author Joacim Zschimmer
  */
trait DirectCommandClient extends CommandClient {

  protected def scheduler: Scheduler
  protected implicit def executionContext: ExecutionContext

  /**
    * Like uncheckedExecuteXml, but fails when response contains an ERROR element.
    *
    * @return XML response
    */
  def executeXml(string: String) = scheduler.executeXmlFuture(string)

  /**
    * Like uncheckedExecuteXml, but fails when response contains an ERROR element.
    *
    * @return XML response
    */
  def executeXml(byteString: ByteString) =
    Future { bytesToXmlString(byteString) } flatMap scheduler.executeXmlFuture

  /**
    * Like uncheckedExecuteXml, but fails when response contains an ERROR element.
    *
    * @return XML response
    */
  def execute(elem: Elem) = scheduler.executeXmlFuture(elem.toString)

  /**
    * @return XML response
    */
  def uncheckedExecuteXml(string: String) = scheduler.uncheckedExecuteXmlFuture(string)

  /**
    * @return XML response
    */
  def uncheckedExecuteXml(byteString: ByteString) =
    Future { bytesToXmlString(byteString) } flatMap scheduler.uncheckedExecuteXmlFuture

  /**
    * @return XML response
    */
  def uncheckedExecute(elem: Elem) = scheduler.uncheckedExecuteXmlFuture(elem.toString)
}

object DirectCommandClient {
  private def bytesToXmlString(byteString: ByteString) = SafeXML.load(new ByteArrayInputStream(byteString.toArray)).toString()
}
