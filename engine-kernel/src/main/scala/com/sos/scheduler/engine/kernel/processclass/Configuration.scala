package com.sos.scheduler.engine.kernel.processclass

import com.sos.jobscheduler.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.agent.AgentAddress
import com.sos.jobscheduler.http.client.heartbeat.HttpHeartbeatTiming
import com.sos.scheduler.engine.common.xml.CppXmlUtils.domElementToStaxSource
import com.sos.scheduler.engine.kernel.processclass.Configuration._
import com.sos.scheduler.engine.kernel.processclass.agent.Agent
import com.sos.scheduler.engine.kernel.processclass.common.selection.{FixedPriority, RoundRobin, SelectionMethod}
import java.net.URI
import java.time.Duration
import javax.xml.transform.Source
import org.w3c.dom
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
private[processclass] case class Configuration(
  agentOption: Option[Agent] = None,
  moreAgents: immutable.IndexedSeq[Agent] = Vector(),
  selectionMethod: SelectionMethod = DefaultSelectionMethod)
{
  val agents: immutable.IndexedSeq[Agent] = agentOption.toVector ++ moreAgents

  if (agents.size > 1) {
    for (a ← agents) a.address.requireURI()
  }
}

private[processclass] object Configuration {
  private val DefaultSelectionMethod = FixedPriority

  def parse(element: dom.Element): Configuration = parse(domElementToStaxSource(element))

  def parse(source: Source): Configuration =
    ScalaXMLEventReader.parseDocument(source) { eventReader ⇒
      import eventReader._
      parseElement("process_class") {
        val nextId: () ⇒ Int = (Iterator from 0).next
        val attributeAgentOption = for (string ← attributeMap.get("remote_scheduler") if string.nonEmpty) yield
          Agent(nextId(), AgentAddress.normalized(string), httpHeartbeatTiming = None)
        attributeMap.ignoreUnread()
        (forEachStartElement {
          case "remote_schedulers" ⇒ parseElement() {
            val selectionMethod = attributeMap.get("select") match {
              case Some("first") ⇒ FixedPriority
              case Some("next") ⇒ RoundRobin
              case None ⇒ DefaultSelectionMethod
              case _ ⇒ throw new IllegalArgumentException("Attribute 'select' is not 'first' or 'next'")
            }
            val children = forEachStartElement {
              case "remote_scheduler" ⇒ parseElement() {
                val uri = new URI(attributeMap("remote_scheduler")) // Should be an URI
                val httpHeartbeatTimingOption =
                  for (timeout ← attributeMap.get("http_heartbeat_timeout") map { o ⇒ Duration ofSeconds o.toInt };
                       period = attributeMap.get("http_heartbeat_period") map { o ⇒ Duration ofSeconds o.toInt } getOrElse timeout / 2)
                  yield HttpHeartbeatTiming(period = period, timeout = timeout)
                Agent(nextId(), AgentAddress(uri), httpHeartbeatTimingOption)
              }
              case _ ⇒ ()
            }
            Configuration(None, children.byClass[Agent], selectionMethod)
          }
        }
          .option[Configuration] getOrElse Configuration()
          copy (agentOption = attributeAgentOption))
      }
    }
}
