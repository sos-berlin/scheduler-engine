package com.sos.scheduler.engine.playground.zschimmer.plugin.watchdog

import com.sos.jobscheduler.common.time.ScalaTime._
import java.time.Duration

final case class Configuration(checkEvery: Duration, timeout: Duration, warnEvery: Duration)


object Configuration {
    private val defaultElem = <plugin.config checkEvery="1" timeout="10" warnEvery="1"/>

    def apply(e: Option[xml.Elem]): Configuration =
      apply(e getOrElse defaultElem)

    def apply(e: xml.Elem): Configuration = {
        def attribute(name: String) = (e.attribute(name) getOrElse defaultElem.attribute(name).get).text
        def durationAttribute(name: String) = java.lang.Integer.parseInt(attribute(name)).s

        Configuration(
            checkEvery = durationAttribute("checkEvery"),
            timeout = durationAttribute("timeout"),
            warnEvery = durationAttribute("warnEvery")
        )
    }
}
