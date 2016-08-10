package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.data.event.{Event, IdAndEvent}
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderKey, OrderNestedFinishedEvent, OrderNestedTouchedEvent, OrderResumedEvent, OrderSetBackEvent, OrderState, OrderStateChangedEvent, OrderStepEndedEvent, OrderStepStartedEvent, OrderSuspendedEvent, OrderTouchedEvent}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.ToHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SchedulerHtmlPage.eventInstantToLocalHtml
import scala.concurrent.ExecutionContext
import scalatags.Text.all._
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class EventsHtmlPage private(
  idAndEvents: Vector[IdAndEvent],
  protected val pageUri: Uri,
  implicit protected val webServiceContext: WebServiceContext,
  protected val schedulerOverview: SchedulerOverview)
extends SchedulerHtmlPage {

  import scala.language.implicitConversions
  import webServiceContext.uris

  private implicit def orderKeyToHtml(orderKey: OrderKey): Frag = stringFrag(orderKey.toString) // a(cls := "inherit-markup", href := uris.order.details(orderKey))

  private implicit def nodeIdToHtml(nodeId: OrderState): Frag = stringFrag(nodeId.toString)

  private implicit def taskIdToHtml(taskId: TaskId): Frag = a(cls := "inherit-markup", href := uris.task.overview(taskId))(taskId.toString)

  def wholePage = htmlPage(
    div(cls := "ContentBox")(
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
      td(eventInstantToLocalHtml(idAndEvent.eventInstant)),
      eventToTds(idAndEvent.event))

  private def eventToTds(event: Event): List[Frag] = {
    val name = event.getClass.getSimpleName
    td(name stripSuffix "Event") :: (
      event match {
        case OrderFinishedEvent(orderKey, nodeId: OrderState)       ⇒ td(orderKey) :: td(nodeId.toString) :: Nil
        case OrderNestedFinishedEvent(orderKey)                     ⇒ td(orderKey) :: Nil
        case OrderNestedTouchedEvent(orderKey)                      ⇒ td(orderKey) :: Nil
        case OrderResumedEvent(orderKey)                            ⇒ td(orderKey) :: Nil
        case OrderSetBackEvent(orderKey, nodeId)                    ⇒ td(orderKey) :: td(nodeId) :: Nil
        case OrderStateChangedEvent(orderKey, fromNodeId, toNodeId) ⇒ td(orderKey) :: td(toNodeId) :: td("← ", fromNodeId) :: Nil
        case OrderStepEndedEvent(orderKey, stateTransition)         ⇒ td(orderKey) :: td(stateTransition.toString) :: Nil
        case OrderStepStartedEvent(orderKey, nodeId, taskId)        ⇒ td(orderKey) :: td(nodeId) :: td(taskId) :: Nil
        case OrderSuspendedEvent(orderKey)                          ⇒ td(orderKey) :: Nil
        case OrderTouchedEvent(orderKey)                            ⇒ td(orderKey) :: Nil
        case _                                                      ⇒ td(event.toString stripPrefix s"$name(" stripSuffix ")") :: Nil
      })
  }
}

object EventsHtmlPage {

  object implicits {
    import scala.language.implicitConversions

    implicit def eventsToHtmlPage(implicit client: SchedulerClient): ToHtmlPage[Vector[IdAndEvent]] =
      new ToHtmlPage[Vector[IdAndEvent]] {
        def apply(pageUri: Uri, webServiceContext: WebServiceContext)(events: Vector[IdAndEvent])
          (implicit executionContext: ExecutionContext)
        =
          for (schedulerOverview ← client.overview) yield
            new EventsHtmlPage(events, pageUri, webServiceContext, schedulerOverview)
      }
  }
}
