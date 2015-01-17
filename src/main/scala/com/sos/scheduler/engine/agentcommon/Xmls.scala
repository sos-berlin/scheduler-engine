package com.sos.scheduler.engine.agentcommon

/**
 * @author Joacim Zschimmer
 */
object Xmls {

  def xmlStringToBoolean(o: String) = o match {
    case "true" | "yes" | "1" ⇒ true
    case "false" | "no" | "0" ⇒ false
    case _ ⇒ throw new IllegalArgumentException(s"Boolean value expected instead of '$o'")
  }
}
