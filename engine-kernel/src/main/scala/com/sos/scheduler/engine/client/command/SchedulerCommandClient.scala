package com.sos.scheduler.engine.client.command

import scala.concurrent.Future

/**
 * Client for JobScheduler web service.
 *
 * @author Joacim Zschimmer
 */
trait SchedulerCommandClient {
  /**
   * Like unchechedExecuteXml, but fails when response contains an ERROR element.
   * @return XML response
   */
  def executeXml(xmlBytes: Array[Byte]): Future[String]

  /**
   * Like unchechedExecuteXml, but fails when response contains an ERROR element.
   * @return XML response
   */
  def execute(elem: xml.Elem): Future[String]

  /**
   * @return XML response
   */
  def uncheckedExecuteXml(xmlBytes: Array[Byte]): Future[String]

  /**
   * @return XML response
   */
  def uncheckedExecute(elem: xml.Elem): Future[String]
}
