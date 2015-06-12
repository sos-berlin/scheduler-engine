package com.sos.scheduler.engine.agent.data.responses

/**
 * @author Joacim Zschimmer
 */
trait XmlResponse extends Response {
  def toXmlElem: xml.Elem
}
