package com.sos.scheduler.engine.taskserver.spoolerapi

import com.sos.scheduler.engine.taskserver.task.common.VariableSets
import SpoolerTask._

/**
 * @author Joacim Zschimmer
 */
trait SpoolerTask {

  final def parameterMap: Map[String, String] = xmlToParameterMap(paramsXml)

  def paramsXml: String

  def paramsXml_=(o: String): Unit

  final def orderParameterMap: Map[String, String] = xmlToParameterMap(orderParamsXml)

  def orderParamsXml: String

  def orderParamsXml_=(o: String): Unit
}

private object SpoolerTask {
  final def xmlToParameterMap(xmlString: String): Map[String, String] =
    xmlString match {
      case "" ⇒ Map()
      case o ⇒ VariableSets.parseXml(o)
    }
}
