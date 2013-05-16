package com.sos.scheduler.engine.kernel.plugin

import PluginConfiguration._
import com.google.inject.Module
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.xml.XmlUtils._
import com.sos.scheduler.engine.kernel.plugin.ActivationMode.activateOnStart
import com.sos.scheduler.engine.kernel.plugin.ActivationMode.dontActivate
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException
import org.w3c.dom.Element
import scala.collection.immutable

private[plugin] final case class PluginConfiguration(className: String, activationMode: ActivationMode, configElementOption: Option[Element]) {

  val pluginClass = {
    val c = Class.forName(className)
    require(classOf[Plugin] isAssignableFrom c, s"Plugin $c does not implement ${classOf[Plugin]}")
    c.asInstanceOf[Class[_ <: Plugin]]
  }

  val guiceModuleOption: Option[Module] =
    Option(pluginClass.getAnnotation(classOf[UseGuiceModule])) map { _.value.newInstance() }

  val configElement = configElementOption getOrElse loadXml(s"<$configElementName/>").getDocumentElement
}

private [plugin] object PluginConfiguration {
  private val logger = Logger(getClass)
  private val configElementName = "plugin.config"

  private [plugin] def readXml(configurationXml: String): immutable.Seq[PluginConfiguration] = {
    val root = loadXml(configurationXml).getDocumentElement
    for (pluginsElement <- Option(elementXPathOrNull(root, "config/plugins")).to[immutable.Seq];
         e <- elementsXPath(pluginsElement, "plugin"))
    yield read(e)
  }

  private[plugin] def read(e: Element): PluginConfiguration = {
    val className = e.getAttribute("java_class")
    if (className.isEmpty) throw new SchedulerException("Missing attribute java_class in <plugin>")
    val activationMode = if (booleanXmlAttribute(e, "dont_activate", false)) dontActivate else activateOnStart
    val configElementOption = Option(elementXPathOrNull(e, configElementName))
    val result = PluginConfiguration(className, activationMode, configElementOption)
    logger.debug(s"Configuration for ${result.className} read")
    result
  }
}
