package com.sos.scheduler.agent.command

/**
 * @author Joacim Zschimmer
 */
trait Response {
  def toElem: xml.Elem
}
