package com.sos.scheduler.engine.kernel.plugin

import org.w3c.dom

/**
 * A JobScheduler object, which XML configuration accepts foreign namespaces processed by plugins.
 * @author Joacim Zschimmer
 */
trait PluginXmlConfigurable {
  def processConfigurationDomElement(element: dom.Element): Unit
}
