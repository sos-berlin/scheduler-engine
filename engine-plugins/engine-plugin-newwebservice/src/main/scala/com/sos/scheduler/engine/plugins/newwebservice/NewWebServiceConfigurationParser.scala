package com.sos.scheduler.engine.plugins.newwebservice

import com.sos.jobscheduler.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.plugins.newwebservice.configuration.NewWebServicePluginConfiguration
import javax.xml.transform.Source

/**
 * @author Joacim Zschimmer
 */
object NewWebServiceConfigurationParser {

  def parseString(xml: String): NewWebServicePluginConfiguration =
    ScalaXMLEventReader.parseString(xml)(parseEvents)

  def parse(source: Source): NewWebServicePluginConfiguration = {
    ScalaXMLEventReader.parseDocument(source)(parseEvents)
  }

  private def parseEvents(eventReader: ScalaXMLEventReader): NewWebServicePluginConfiguration = {
    import eventReader._
    parseElement("plugin.config") {
      val testMode = attributeMap.as[Boolean]("test", default = false)
      new NewWebServicePluginConfiguration(testMode = testMode)
    }
  }
}
