package com.sos.scheduler.engine.agent.data.commandresponses

import spray.json.DefaultJsonProtocol.jsonFormat0

/**
 * @author Joacim Zschimmer
 */
case object EmptyResponse extends XmlResponse {

  def toXmlElem = <ok/>

  implicit def MyJsonFormat = jsonFormat0(() ⇒ EmptyResponse)
}
