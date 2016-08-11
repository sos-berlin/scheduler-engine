package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.data.event.{Event, IdAndEvent, Snapshot}
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.jobchain.NodeId
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.ToHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SchedulerHtmlPage.eventIdToLocalHtml
import scala.collection.immutable
import scala.concurrent.ExecutionContext
import scalatags.Text.all._
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class EventsHtmlPage private(
  protected val snapshot: Snapshot[immutable.Seq[IdAndEvent]],
  protected val pageUri: Uri,
  implicit protected val uris: SchedulerUris,
  protected val schedulerOverview: SchedulerOverview)
extends SchedulerHtmlPage {

  import scala.language.implicitConversions

  private val idAndEvents = snapshot.value

  private implicit def orderKeyToHtml(orderKey: OrderKey): Frag = stringFrag(orderKey.toString) // a(cls := "inherit-markup", href := uris.order.details(orderKey))

  private implicit def nodeIdToHtml(nodeId: NodeId): Frag = stringFrag(nodeId.toString)

  private implicit def taskIdToHtml(taskId: TaskId): Frag = a(cls := "inherit-markup", href := uris.task.overview(taskId))(taskId.toString)

  def wholePage = htmlPage(
    div(cls := "ContentBox ContentBox-single")(
      table(cls := "SimpleTable")(
        thead(
          tr(
            th("Timestamp"),
            th("Event"),
            th("Object"),
            th, th, th
          )
        ),
        tbody(
          (idAndEvents map eventToTr).toVector))))

  private def eventToTr(idAndEvent: IdAndEvent): Frag =
    tr(
      td(eventIdToLocalHtml(idAndEvent.eventId)),
      eventToTds(idAndEvent.event))

  private def eventToTds(event: Event): List[Frag] = {
    val name = event.getClass.getSimpleName
    td(name stripSuffix "Event") :: (
      event match {
        case OrderFinished(orderKey, nodeId: NodeId)           ⇒ td(orderKey) :: td(nodeId.toString) :: Nil
        case OrderNestedFinished(orderKey)                     ⇒ td(orderKey) :: Nil
        case OrderNestedStarted(orderKey)                      ⇒ td(orderKey) :: Nil
        case OrderNodeChanged(orderKey, nodeId, fromNodeId)    ⇒ td(orderKey) :: td(nodeId) :: td("← ", fromNodeId) :: Nil
        case OrderResumed(orderKey)                            ⇒ td(orderKey) :: Nil
        case OrderSetBack(orderKey, nodeId)                    ⇒ td(orderKey) :: td(nodeId) :: Nil
        case OrderStepEnded(orderKey, stateTransition)         ⇒ td(orderKey) :: td(stateTransition.toString) :: Nil
        case OrderStepStarted(orderKey, nodeId, taskId)        ⇒ td(orderKey) :: td(nodeId) :: td(taskId) :: Nil
        case OrderSuspended(orderKey)                          ⇒ td(orderKey) :: Nil
        case OrderStarted(orderKey)                            ⇒ td(orderKey) :: Nil
        case _                                                 ⇒ td(event.toString stripPrefix s"$name(" stripSuffix ")") :: Nil
      })
  }
}

object EventsHtmlPage {

  object implicits {
    import scala.language.implicitConversions

    implicit def eventsToHtmlPage(implicit client: SchedulerClient, webServiceContext: WebServiceContext, ec: ExecutionContext) =
      ToHtmlPage[Snapshot[immutable.Seq[IdAndEvent]]] { (snapshot, pageUri) ⇒
        for (schedulerOverviewSnapshot ← client.overview) yield
          new EventsHtmlPage(snapshot, pageUri, webServiceContext.uris, schedulerOverviewSnapshot.value)
      }
  }
}
