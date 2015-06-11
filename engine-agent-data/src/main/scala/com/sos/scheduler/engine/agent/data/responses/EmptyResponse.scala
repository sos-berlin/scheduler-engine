package com.sos.scheduler.engine.agent.data.responses

import spray.json.DefaultJsonProtocol.jsonFormat0

/**
 * @author Joacim Zschimmer
 */
case object EmptyResponse extends Response {

  def toXmlElem = <ok/>

  implicit def MyJsonFormat = jsonFormat0(() ⇒ EmptyResponse)
}
