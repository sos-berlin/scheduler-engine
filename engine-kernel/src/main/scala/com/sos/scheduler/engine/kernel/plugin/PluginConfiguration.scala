package com.sos.scheduler.engine.kernel.plugin

import com.google.inject.Module
import com.sos.jobscheduler.base.utils.ScalaUtils.cast
import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.scheduler.engine.common.xml.CppXmlUtils._
import com.sos.scheduler.engine.kernel.plugin.PluginConfiguration._
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException
import org.w3c.dom.Element
import scala.collection.immutable

private[plugin] final case class PluginConfiguration(className: String, configElementOption: Option[Element], testInhibitsActivateOnStart: Boolean = false) {

  val pluginClass: Class[Plugin] = cast[Class[Plugin]](Class forName className)

  val guiceModuleOption: Option[Module] =
    Option(pluginClass.getAnnotation(classOf[UseGuiceModule])) map { _.value.newInstance() }

  val configElement = configElementOption getOrElse loadXml(s"<$configElementName/>").getDocumentElement
}

private [plugin] object PluginConfiguration {
  private val logger = Logger(getClass)
  private val configElementName = "plugin.config"

  private [plugin] def readXml(configurationXml: String): immutable.Seq[PluginConfiguration] = {
    val root = loadXml(configurationXml).getDocumentElement
    for (pluginsElement ← Option(elementXPathOrNull(root, "config/plugins")).to[immutable.Seq];
         e ← elementsXPath(pluginsElement, "plugin"))
    yield read(e)
  }

  private[plugin] def read(e: Element): PluginConfiguration = {
    val className = e.getAttribute("java_class")
    if (className.isEmpty) throw new SchedulerException("Missing attribute java_class in <plugin>")
    val configElementOption = Option(elementXPathOrNull(e, configElementName))
    val result = PluginConfiguration(className, configElementOption, testInhibitsActivateOnStart = booleanXmlAttribute(e, "dont_activate", false))
    logger.debug(s"Configuration for ${result.className} read")
    result
  }
}
