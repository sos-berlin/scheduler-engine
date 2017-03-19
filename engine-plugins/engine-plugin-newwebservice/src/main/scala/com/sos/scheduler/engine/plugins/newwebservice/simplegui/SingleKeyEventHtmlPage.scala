package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.jobscheduler.common.sprayutils.html.HtmlDirectives.ToHtmlPage
import com.sos.jobscheduler.common.sprayutils.html.HtmlPage.seqFrag
import com.sos.jobscheduler.data.event.{Event, EventId, EventSeq, Stamped, TearableEventSeq}
import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.client.api.SchedulerOverviewClient
import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.data.jobchain.NodeId
import com.sos.scheduler.engine.data.log.Logged
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerWebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SchedulerHtmlPage.eventIdToLocalHtml
import java.time.{Instant, LocalDate}
import scala.collection.immutable.Seq
import scala.concurrent.ExecutionContext
import scalatags.Text.all._
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class SingleKeyEventHtmlPage private(
  key: Any,
  stampedEventSeq: Stamped[TearableEventSeq[Seq, Event]],
  protected val pageUri: Uri,
  protected val webServiceContext: SchedulerWebServiceContext,
  protected val schedulerOverview: SchedulerOverview)
extends SchedulerHtmlPage {

  import scala.language.implicitConversions

  protected def eventId = stampedEventSeq.eventId

  private implicit val uris = webServiceContext.uris
  private val eventSeq = stampedEventSeq.value
  private val midnightInstant = Instant.ofEpochSecond(LocalDate.now(SchedulerHtmlPage.OurZoneId).toEpochDay * 24*3600)

  private implicit def nodeIdToHtml(nodeId: NodeId): Frag = stringFrag(nodeId.toString)

  private implicit def taskIdToHtml(taskId: TaskId): Frag = a(cls := "inherit-markup", href := uris.task.overview(taskId))(taskId.toString)


  def wholePage = htmlPage(
    div(cls := "ContentBox ContentBox-single Padded")(
      h3(key.toString),
      eventSeq match {
        case EventSeq.Torn ⇒ p("Event stream is torn. Try a newer EventId (parameter after=", stampedEventSeq.eventId, ")")
        case EventSeq.Empty(eventId) ⇒ p("No events until " + EventId.toString(eventId))
        case EventSeq.NonEmpty(eventStampeds) ⇒
          table(cls := "SimpleTable")(
            thead(
              tr(
                th("Timestamp"),
                th("Event"),
                th,
                th,
                th)),
            tbody(
              eventStampeds map eventToTr))
      }))

  private def eventToTr(eventStamped: Stamped[Event]): Frag =
    tr(
      td(whiteSpace.nowrap)(
        eventIdToLocalHtml(eventStamped.eventId, withDateBefore = midnightInstant)),
      eventToTds(eventStamped.value))

  private def eventToTds(event: Event): Frag = {
    val eventName = event.getClass.getSimpleName stripSuffix "$"

    def unknownEventToTds(event: Event): Frag = {
      val withoutEventName = event.toString match {
        case `eventName` ⇒ ""
        case string ⇒ string stripPrefix s"$eventName(" stripSuffix ")"
      }
      td(colspan := 3, withoutEventName)
    }

    event match {
      case event: OrderEvent ⇒
        seqFrag(
          td(eventName),
          event match {
            case OrderFinished(nodeId: NodeId)        ⇒ td(nodeId)
            case OrderNodeChanged(nodeId, fromNodeId) ⇒ td(nodeId) :: td("← ", fromNodeId) :: Nil
            case OrderSetBack(nodeId)                 ⇒ td(nodeId)
            case OrderStepEnded(stateTransition)      ⇒ td(colspan := 3)(stateTransition.toString)
            case OrderStepStarted(nodeId, taskId)     ⇒ td(nodeId) :: td(taskId) :: Nil
            case _ ⇒ unknownEventToTds(event)
          })
      case event: Logged ⇒
        td :: td(event.level.toString) :: td(colspan := 2)(event.message) :: Nil
      case _ ⇒
        td(eventName) :: unknownEventToTds(event) :: Nil
    }
  }
}

object SingleKeyEventHtmlPage {
  import scala.language.implicitConversions

  def singleKeyEventToHtmlPage[E <: Event](key: Any)(implicit client: SchedulerOverviewClient, webServiceContext: SchedulerWebServiceContext, ec: ExecutionContext) =
    ToHtmlPage[Stamped[TearableEventSeq[Seq, E]]] { (stampedEventSeq, pageUri) ⇒
      for (stampedOverview ← client.overview) yield
        new SingleKeyEventHtmlPage(key, stampedEventSeq, pageUri, webServiceContext, stampedOverview.value)
    }
}
