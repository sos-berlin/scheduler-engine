package com.sos.scheduler.engine.client.api

import akka.util.ByteString
import scala.concurrent.Future

/**
 * Client for JobScheduler web service.
 *
 * @author Joacim Zschimmer
 */
trait CommandClient {

  /**
   * Like uncheckedExecuteXml, but fails when response contains an ERROR element.
   * @return XML response
   */
  def executeXml(string: String): Future[String]

  /**
   * Like uncheckedExecuteXml, but fails when response contains an ERROR element.
   * @return XML response
   */
  def executeXml(byteString: ByteString): Future[String]

  /**
   * Like uncheckedExecuteXml, but fails when response contains an ERROR element.
   * @return XML response
   */
  def execute(elem: xml.Elem): Future[String]

  /**
   * @return XML response
   */
  def uncheckedExecuteXml(string: String): Future[String]

  /**
   * @return XML response
   */
  def uncheckedExecuteXml(byteString: ByteString): Future[String]

  /**
   * @return XML response
   */
  def uncheckedExecute(elem: xml.Elem): Future[String]
}
