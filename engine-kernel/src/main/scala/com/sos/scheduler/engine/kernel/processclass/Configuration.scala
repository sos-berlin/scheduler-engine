package com.sos.scheduler.engine.kernel.processclass

import com.sos.scheduler.engine.common.scalautil.xmls.ScalaStax.domElementToStaxSource
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.http.client.heartbeat.HttpHeartbeatTiming
import com.sos.scheduler.engine.kernel.processclass.agent.Agent
import java.net.URI
import java.time.Duration
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
  def parse(element: dom.Element): Configuration =
    ScalaXMLEventReader.parseDocument(domElementToStaxSource(element)) { eventReader ⇒
      import eventReader._
      parseElement("process_class") {
        case class Agents(agents: immutable.IndexedSeq[Agent])
        val nextId: () ⇒ Int = (Iterator from 0).next
        val attributeAgentOption = attributeMap.get("remote_scheduler") filter { _.nonEmpty } map { o ⇒ Agent(nextId(), o, httpHeartbeatTiming = None) }
        attributeMap.ignoreUnread()
        val children = forEachStartElement {
          case "remote_schedulers" ⇒ parseElement() {
            val children = forEachStartElement {
              case "remote_scheduler" ⇒ parseElement() {
                val uri = new URI(attributeMap("remote_scheduler")) // Should be an URI
                val httpHeartbeatTimingOption =
                  for (timeout ← attributeMap.get("http_heartbeat_timeout") map { o ⇒ Duration ofSeconds o.toInt };
                       period = attributeMap.get("http_heartbeat_period") map { o ⇒ Duration ofSeconds o.toInt } getOrElse timeout / 2)
                  yield HttpHeartbeatTiming(period = period, timeout = timeout)
                Agent(nextId(), uri.toString, httpHeartbeatTimingOption)
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
