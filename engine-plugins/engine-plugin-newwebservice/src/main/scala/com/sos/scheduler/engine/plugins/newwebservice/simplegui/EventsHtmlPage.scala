package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, KeyedEvent, Snapshot}
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.jobchain.NodeId
import com.sos.scheduler.engine.data.log.LogEvent
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.ToHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SchedulerHtmlPage.eventIdToLocalHtml
import java.time.{Instant, LocalDate}
import scala.collection.immutable
import scala.concurrent.ExecutionContext
import scalatags.Text.all._
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class EventsHtmlPage private(
  protected val snapshot: Snapshot[immutable.Seq[Snapshot[AnyKeyedEvent]]],
  protected val pageUri: Uri,
  implicit protected val uris: SchedulerUris,
  protected val schedulerOverview: SchedulerOverview)
extends SchedulerHtmlPage {

  import scala.language.implicitConversions

  private val eventSnapshot = snapshot.value

  private implicit def orderKeyToHtml(orderKey: OrderKey): Frag = stringFrag(orderKey.toString) // a(cls := "inherit-markup", href := uris.order.detailed(orderKey))

  private implicit def nodeIdToHtml(nodeId: NodeId): Frag = stringFrag(nodeId.toString)

  private implicit def taskIdToHtml(taskId: TaskId): Frag = a(cls := "inherit-markup", href := uris.task.overview(taskId))(taskId.toString)

  private val midnightInstant = Instant.ofEpochSecond(LocalDate.now(SchedulerHtmlPage.OurZoneId).toEpochDay * 24*3600)

  def wholePage = htmlPage(
    div(cls := "ContentBox ContentBox-single Padded")(
      table(cls := "SimpleTable")(
        thead(
          tr(
            th("Timestamp"),
            th("Object"),
            th("Event"),
            th, th, th
          )
        ),
        tbody(
          (eventSnapshot map eventToTr).toVector))))

  private def eventToTr(eventSnapshot: Snapshot[AnyKeyedEvent]): Frag =
    tr(
      td(whiteSpace.nowrap)(eventIdToLocalHtml(eventSnapshot.eventId, withDateBefore = midnightInstant)),
      eventToTds(eventSnapshot.value))

  private def eventToTds(event: AnyKeyedEvent): List[Frag] = {
    val eventName = event match {
      case KeyedEvent(_, e) ⇒ e.getClass.getSimpleName stripSuffix "$"
      case _ ⇒ event.getClass.getSimpleName stripSuffix "Event"
    }
    event match {
      case KeyedEvent(orderKey: OrderKey, e: OrderEvent) ⇒
        td(orderKey.string) :: td(eventName) :: (
          e match {
            case OrderFinished(nodeId: NodeId) ⇒ td(nodeId) :: Nil
            case OrderNestedFinished ⇒ Nil
            case OrderNestedStarted ⇒ Nil
            case OrderNodeChanged(nodeId, fromNodeId) ⇒ td(nodeId) :: td("← ", fromNodeId) :: Nil
            case OrderResumed ⇒ Nil
            case OrderSetBack(nodeId) ⇒ td(nodeId) :: Nil
            case OrderStepEnded(stateTransition) ⇒ td(stateTransition.toString) :: Nil
            case OrderStepStarted(nodeId, taskId) ⇒ td(nodeId) :: td(taskId) :: Nil
            case OrderSuspended ⇒ Nil
            case OrderStarted ⇒ Nil
            case _ ⇒ td(colspan := 3, e.toString) :: Nil
          })
      case KeyedEvent(_, e: LogEvent) ⇒
        td() :: td(e.level.toString) :: td(colspan := 4)(e.message) :: Nil
      case KeyedEvent(key, e) ⇒
        td(key.toString) :: td(eventName) :: td(colspan := 4, event.toString stripPrefix s"$eventName(" stripSuffix ")") :: Nil
      case _ ⇒
        td() :: td(eventName) :: td(colspan := 4, event.toString stripPrefix s"$eventName(" stripSuffix ")") :: Nil
    }
  }
}

object EventsHtmlPage {

  object implicits {
    import scala.language.implicitConversions

    implicit def eventsToHtmlPage(implicit client: SchedulerClient, webServiceContext: WebServiceContext, ec: ExecutionContext) =
      ToHtmlPage[Snapshot[immutable.Seq[Snapshot[AnyKeyedEvent]]]] { (snapshot, pageUri) ⇒
        for (schedulerOverviewSnapshot ← client.overview) yield
          new EventsHtmlPage(snapshot, pageUri, webServiceContext.uris, schedulerOverviewSnapshot.value)
      }
  }
}
