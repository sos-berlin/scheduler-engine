package com.sos.scheduler.engine.kernel.processclass

import com.sos.scheduler.engine.common.scalautil.xmls.ScalaStax.domElementToStaxSource
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.kernel.processclass.agent.Agent
import java.net.URI
import org.w3c.dom
import scala.collection.immutable


/**
 * @author Joacim Zschimmer
 */
private[processclass] case class Configuration(agentOption: Option[Agent], moreAgents: immutable.IndexedSeq[Agent]) {

  val agents: immutable.IndexedSeq[Agent] = agentOption.toVector ++ moreAgents

  if (agents.size > 1) {
    val addresses = agents map { _.address }
    addresses foreach { o ⇒ new URI(o) }   // URI syntax required
  }
}

private[processclass] object Configuration {
  def parse(element: dom.Element) =
    ScalaXMLEventReader.parseDocument(domElementToStaxSource(element)) { eventReader ⇒
      import eventReader._
      parseElement("process_class") {
        case class Agents(agents: immutable.IndexedSeq[Agent])
        val nextId: () ⇒ Int = (Iterator from 0).next
        val attributeAgentOption = attributeMap.get("remote_scheduler") filter { _.nonEmpty } map { o ⇒ Agent(nextId(), o) }
        attributeMap.ignoreUnread()
        val children = forEachStartElement {
          case "remote_schedulers" ⇒ parseElement() {
            val children = forEachStartElement {
              case "remote_scheduler" ⇒ parseElement() {
                val uri = new URI(attributeMap("remote_scheduler")) // Should be an URI
                Agent(nextId(), uri.toString)
              }
              case _ ⇒ ()
            }
            Agents(children.byClass[Agent])
          }
        }
        Configuration(attributeAgentOption, children.option[Agents].toVector flatMap { _.agents })
      }
    }
}
