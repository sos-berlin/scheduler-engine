package com.sos.scheduler.engine.taskserver.spoolerapi

/**
 * @author Joacim Zschimmer
 */
trait SpoolerTask {

  def paramsXml: String

  def orderParamsXml: String
}
