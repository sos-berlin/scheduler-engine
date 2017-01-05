package com.sos.scheduler.engine.master.order.agent

import com.sos.scheduler.engine.common.scalautil.xmls.XmlSources._
import com.sos.scheduler.engine.data.engine2.agent.AgentPath
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class AgentParserTest extends FreeSpec {

  "AgentParser" in {
    val xml = <agent uri="http://localhost"/>
    val path = AgentPath("/FOLDER/AGENT")
    assert(AgentParser.parseXml(path, xml) == Agent(path, uri = "http://localhost"))
  }
}
