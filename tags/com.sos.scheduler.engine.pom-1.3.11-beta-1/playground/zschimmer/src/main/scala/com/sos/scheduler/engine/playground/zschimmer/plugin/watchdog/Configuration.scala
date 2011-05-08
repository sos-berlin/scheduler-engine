package com.sos.scheduler.engine.playground.zschimmer.plugin.watchdog

import com.sos.scheduler.engine.kernel.util.Time
import scala.xml.Elem


case class Configuration(checkEvery: Time, timeout: Time, warnEvery: Time)


object Configuration {
    private val defaultElem = <plugin.config checkEvery="1" timeout="10" warnEvery="1"/>

    def apply(e: Option[Elem]): Configuration = apply(e getOrElse defaultElem)
    
    def apply(e: Elem): Configuration = {
        def attribute(name: String) = (e.attribute(name) getOrElse defaultElem.attribute(name).get).text
        def timeAttribute(name: String) = Time.of(java.lang.Double.parseDouble(attribute(name)))

        Configuration(
            checkEvery = timeAttribute("checkEvery"),
            timeout = timeAttribute("timeout"),
            warnEvery = timeAttribute("warnEvery")
        )
    }
}
