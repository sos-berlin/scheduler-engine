package com.sos.scheduler.engine.kernel.plugin

import org.w3c.dom

/**
 * @author Joacim Zschimmer
 */
trait NamespaceXmlPlugin {
  this: Plugin ⇒

  def xmlNamespace: String

  def attachPluginXmlConfigurable(obj: PluginXmlConfigurable, element: dom.Element): Unit

  def detachPluginXmlConfigurable(obj: PluginXmlConfigurable): Unit
}
