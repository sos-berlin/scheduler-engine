package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.jobscheduler.data.event.{AnyKeyedEvent, EventId, EventSeq, KeyedEvent, Stamped}
import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.client.api.SchedulerOverviewClient
import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.data.jobchain.NodeId
import com.sos.scheduler.engine.data.log.Logged
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.ToHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage.{EmptyFrag, seqFrag}
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SchedulerHtmlPage.eventIdToLocalHtml
import scala.collection.immutable.Seq
import scala.concurrent.ExecutionContext
import scalatags.Text.all._
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class KeyedEventsHtmlPage private(
  protected val snapshot: Stamped[EventSeq[Seq, AnyKeyedEvent]],
  protected val pageUri: Uri,
  implicit protected val uris: SchedulerUris,
  protected val schedulerOverview: SchedulerOverview)
extends SchedulerHtmlPage {

  import scala.language.implicitConversions

  private val eventSeq = snapshot.value

  private implicit def orderKeyToHtml(orderKey: OrderKey): Frag = stringFrag(orderKey.toString) // a(cls := "inherit-markup", href := uris.order.detailed(orderKey))

  private implicit def nodeIdToHtml(nodeId: NodeId): Frag = stringFrag(nodeId.toString)

  private implicit def taskIdToHtml(taskId: TaskId): Frag = a(cls := "inherit-markup", href := uris.task.overview(taskId))(taskId.toString)

  private val midnightInstant = SchedulerHtmlPage.midnightInstant

  def wholePage = htmlPage(
    div(cls := "ContentBox ContentBox-single Padded")(
      eventSeq match {
        case EventSeq.Torn ⇒ p("Event stream is torn. Try a newer EventId (parameter after=", snapshot.eventId, ")")
        case EventSeq.Empty(eventId) ⇒ p("No events until " + EventId.toString(eventId))
        case EventSeq.NonEmpty(eventStampeds) ⇒
          table(cls := "SimpleTable")(
            thead(
              tr(
                th("Timestamp"),
                th("Object"),
                th("Event"),
                th,
                th,
                th
              )
            ),
            tbody(
              (eventStampeds map eventToTr).toVector))
      }))

  private def eventToTr(eventStamped: Stamped[AnyKeyedEvent]): Frag =
    tr(
      td(whiteSpace.nowrap)(eventIdToLocalHtml(eventStamped.eventId, withDateBefore = midnightInstant)),
      eventToTds(eventStamped.value))

  private def eventToTds(keyedEvent: AnyKeyedEvent): Frag = {
    val eventName = keyedEvent.event.getClass.getSimpleName stripSuffix "$"
    keyedEvent match {
      case KeyedEvent(orderKey: OrderKey, event: OrderEvent) ⇒
        seqFrag(
          td(orderKey.string),
          td(eventName),
          event match {
            case OrderFinished(nodeId: NodeId) ⇒ td(nodeId)
            case OrderNestedFinished ⇒ EmptyFrag
            case OrderNestedStarted ⇒ EmptyFrag
            case OrderNodeChanged(nodeId, fromNodeId) ⇒ seqFrag(td(nodeId), td("← ", fromNodeId))
            case OrderResumed ⇒ EmptyFrag
            case OrderSetBack(nodeId) ⇒ td(nodeId)
            case OrderStepEnded(stateTransition) ⇒ td(stateTransition.toString)
            case OrderStepStarted(nodeId, taskId) ⇒ seqFrag(td(nodeId), td(taskId))
            case OrderSuspended ⇒ EmptyFrag
            case OrderStarted ⇒ EmptyFrag
            case _ ⇒ td(colspan := 3, event.toString)
          })
      case KeyedEvent(_, e: Logged) ⇒
        seqFrag(td, td(e.level.toString), td(e.message))
      case KeyedEvent(key, e) ⇒
        seqFrag(td(key.toString), td(eventName), td(keyedEvent.event.toString stripPrefix eventName stripPrefix "(" stripSuffix ")"))
    }
  }
}

object KeyedEventsHtmlPage {

  object implicits {
    import scala.language.implicitConversions

    implicit def keyedEventsToHtmlPage(implicit client: SchedulerOverviewClient, webServiceContext: WebServiceContext, ec: ExecutionContext) =
      ToHtmlPage[Stamped[EventSeq[Seq, AnyKeyedEvent]]] { (snapshot, pageUri) ⇒
        for (stamped ← client.overview) yield
          new KeyedEventsHtmlPage(snapshot, pageUri, webServiceContext.uris, stamped.value)
      }
  }
}
