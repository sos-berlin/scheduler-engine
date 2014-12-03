package com.sos.scheduler.engine.plugins.jetty

import org.eclipse.jetty.servlet.ServletContextHandler

/**
 * @author Joacim Zschimmer
 */
final case class JettyPluginExtension(modifyServletContextHandler: ServletContextHandler ⇒ Unit)
