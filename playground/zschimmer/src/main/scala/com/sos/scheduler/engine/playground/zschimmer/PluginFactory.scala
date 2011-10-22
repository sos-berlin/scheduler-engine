package com.sos.scheduler.engine.playground.zschimmer

import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.plugin.Plugin
import com.sos.scheduler.engine.kernel.plugin.{PluginFactory => JavaPlugInFactory}
import org.w3c.dom.Element
import scala.xml.Elem


trait PlugInFactory extends JavaPlugInFactory {
    def newInstance(scheduler: Scheduler, configurationOption: Option[Elem]): Plugin

    override def newInstance(scheduler: Scheduler, configurationOrNull: Element) = {
        val elem = Option(configurationOrNull) map { XMLs.fromJavaDom(_).asInstanceOf[xml.Elem] }
        newInstance(scheduler, elem)
    }
}
