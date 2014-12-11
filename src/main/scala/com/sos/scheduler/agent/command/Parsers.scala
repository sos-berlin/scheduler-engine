package com.sos.scheduler.agent.command

/**
 * @author Joacim Zschimmer
 */
private[command] object Parsers {

  def parseBoolean(o: String) = o match {
    case "true" | "yes" | "1"  ⇒ true
    case "false" | "no" | "0" ⇒ false
  }
}
