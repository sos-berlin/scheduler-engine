package com.sos.scheduler.engine.plugins.newwebservice.configuration

import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerEncoding
import java.nio.charset.StandardCharsets.ISO_8859_1
import spray.http.HttpCharsets.`ISO-8859-1`

/**
 * @author Joacim Zschimmer
 */
final case class NewWebServicePluginConfiguration(testMode: Boolean = false)

object NewWebServicePluginConfiguration {

  val SchedulerHttpCharset = schedulerEncoding match {
    case ISO_8859_1 ⇒ `ISO-8859-1`
  }
}
