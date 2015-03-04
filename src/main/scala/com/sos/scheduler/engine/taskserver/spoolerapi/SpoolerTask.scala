package com.sos.scheduler.engine.taskserver.spoolerapi

/**
 * @author Joacim Zschimmer
 */
trait SpoolerTask {

  def paramsXml: String

  def paramsXml_=(o: String): Unit

  def orderParamsXml: String

  def orderParamsXml_=(o: String): Unit
}
