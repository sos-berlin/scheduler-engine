package com.sos.scheduler.engine.client.api

import akka.util.ByteString
import com.sos.jobscheduler.common.scalautil.xmls.SafeXML
import java.io.ByteArrayInputStream
import scala.concurrent.{ExecutionContext, Future}
import scala.xml.Elem

/**
  * Client for JobScheduler web service.
  *
  * @author Joacim Zschimmer
  */
trait CommandClient {

  /**
    * Like uncheckedExecuteXml, but fails when response contains an ERROR element.
    *
    * @return XML response
    */
  def execute(elem: xml.Elem): Future[String]

  /**
    * Like uncheckedExecuteXml, but fails when response contains an ERROR element.
    *
    * @return XML response
    */
  def executeXml(byteString: ByteString): Future[String]

  /**
    * Like uncheckedExecuteXml, but fails when response contains an ERROR element.
    *
    * @return XML response
    */
  def executeXml(string: String): Future[String]

  /**
    * @return XML response
    */
  def uncheckedExecute(elem: Elem): Future[String]

  /**
    * @return XML response
    */
  def uncheckedExecuteXml(byteString: ByteString): Future[String]

  /**
    * @return XML response
    */
  def uncheckedExecuteXml(string: String): Future[String]
}

object CommandClient {

  private def bytesToXmlString(byteString: ByteString) = SafeXML.load(new ByteArrayInputStream(byteString.toArray)).toString()

  trait Defaults {
    this: CommandClient ⇒

    protected implicit def executionContext: ExecutionContext

    def execute(elem: xml.Elem) = executeXml(elem.toString)

    def executeXml(byteString: ByteString): Future[String] =
      Future { bytesToXmlString(byteString) } flatMap executeXml

    def uncheckedExecute(elem: Elem) = uncheckedExecuteXml(elem.toString)

    def uncheckedExecuteXml(byteString: ByteString): Future[String] =
      Future { bytesToXmlString(byteString) } flatMap uncheckedExecuteXml
  }
}
