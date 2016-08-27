package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.client.api.SchedulerOverviewClient
import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.data.event.{Event, Snapshot}
import com.sos.scheduler.engine.data.events.AnyEvent
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
final class SingleKeyEventHtmlPage private(
  key: Any,
  protected val snapshot: Snapshot[immutable.Seq[Snapshot[Event]]],
  protected val pageUri: Uri,
  implicit protected val uris: SchedulerUris,
  protected val schedulerOverview: SchedulerOverview)
extends SchedulerHtmlPage {

  import scala.language.implicitConversions

  private val eventSnapshot = snapshot.value

  private implicit def nodeIdToHtml(nodeId: NodeId): Frag = stringFrag(nodeId.toString)

  private implicit def taskIdToHtml(taskId: TaskId): Frag = a(cls := "inherit-markup", href := uris.task.overview(taskId))(taskId.toString)

  private val midnightInstant = Instant.ofEpochSecond(LocalDate.now(SchedulerHtmlPage.OurZoneId).toEpochDay * 24*3600)

  def wholePage = htmlPage(
    div(cls := "ContentBox ContentBox-single Padded")(
      h3(key.toString),
      table(cls := "SimpleTable")(
        thead(
          tr(
            th("Timestamp"),
            th("Event"),
            th, th, th
          )
        ),
        tbody(
          (eventSnapshot map eventToTr).toVector))))

  private def eventToTr(eventSnapshot: Snapshot[Event]): Frag =
    tr(
      td(whiteSpace.nowrap)(eventIdToLocalHtml(eventSnapshot.eventId, withDateBefore = midnightInstant)),
      eventToTds(eventSnapshot.value))

  private def eventToTds(event: Event): List[Frag] = {
    val eventName = event.getClass.getSimpleName stripSuffix "$"

    def unknownEventToTds(event: Event): List[Frag] = {
      val withoutEventName = event.toString match {
        case `eventName` ⇒ ""
        case string ⇒ string stripPrefix s"$eventName(" stripSuffix ")"
      }
      td(colspan := 4, withoutEventName) :: Nil
    }

    event match {
      case event: OrderEvent ⇒
        td(eventName) :: (
          event match {
            case OrderFinished(nodeId: NodeId)        ⇒ td(nodeId) :: Nil
            case OrderNodeChanged(nodeId, fromNodeId) ⇒ td(nodeId) :: td("← ", fromNodeId) :: Nil
            case OrderSetBack(nodeId)                 ⇒ td(nodeId) :: Nil
            case OrderStepEnded(stateTransition)      ⇒ td(stateTransition.toString) :: Nil
            case OrderStepStarted(nodeId, taskId)     ⇒ td(nodeId) :: td(taskId) :: Nil
            case _ ⇒ unknownEventToTds(event)
          })
      case event: LogEvent ⇒
        td() :: td(event.level.toString) :: td(colspan := 4)(event.message) :: Nil
      case _ ⇒
        td(eventName) :: unknownEventToTds(event)
    }
  }

}

object SingleKeyEventHtmlPage {

  import scala.language.implicitConversions

  def singleKeyEventToHtmlPage(key: Any)(implicit client: SchedulerOverviewClient, webServiceContext: WebServiceContext, ec: ExecutionContext) =
    ToHtmlPage[Snapshot[immutable.Seq[Snapshot[AnyEvent]]]] { (snapshot, pageUri) ⇒
      for (schedulerOverviewSnapshot ← client.overview) yield
        new SingleKeyEventHtmlPage(key, snapshot, pageUri, webServiceContext.uris, schedulerOverviewSnapshot.value)
    }
}
