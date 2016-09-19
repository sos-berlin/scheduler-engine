package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.common.convert.As._
import com.sos.scheduler.engine.common.convert.ConvertiblePartialFunctions.ImplicitConvertablePF
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, Event, EventId, KeyedEvent, NoKeyEvent, Snapshot}
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.event.OrderStatisticsChangedSource
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.completeTryHtml
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.EventRoute._
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.KeyedEventsHtmlPage.implicits.keyedEventsToHtmlPage
import scala.collection.immutable
import scala.concurrent.ExecutionContext
import scala.math.abs
import scala.reflect.ClassTag
import spray.json.DefaultJsonProtocol._
import spray.routing.Directives._
import spray.routing.{Route, ValidationRejection}

/**
  * @author Joacim Zschimmer
  */
trait EventRoute extends HasCloser {

  protected def orderStatisticsChangedSource: OrderStatisticsChangedSource
  protected implicit def client: DirectSchedulerClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  def eventRoute: Route =
    pathSingleSlash {
      parameterMap { parameters ⇒
        val afterEventId = parameters.as[Long]("after", EventId.BeforeFirst)
        val limit = parameters.as[Int]("limit", Int.MaxValue)
        val returnType = parameters.as[String]("return", classOf[Event].getSimpleName)
        //val timeout = parameters.optionAs[Int]("timeout") map { o ⇒ Duration.ofSeconds(o) }
        if (limit == 0)
          reject(ValidationRejection(s"Invalid limit=$limit"))
        else
          completeTryHtml[immutable.Seq[Snapshot[AnyKeyedEvent]]] {
            returnType match {
              case "OrderStatisticsChanged" ⇒
                for (snapshot ← orderStatisticsChangedSource.whenOrderStatisticsChanged(after = afterEventId))
                  yield nestIntoSeqSnapshot(snapshot)
              case _ ⇒
                val classTag = ClassTag[Event](SchedulerAnyKeyedEventJsonFormat.typeToClass(returnType))
                client.events[Event](after = afterEventId, limit = abs(limit), reverse = limit < 0)(classTag)
            }
          }
      }
    }
}

object EventRoute {
  // Nests a simple Snapshot[NoKeyEvent] into the expected nested type for the event web service.
  private def nestIntoSeqSnapshot[E <: NoKeyEvent](snapshot: Snapshot[E]): Snapshot[immutable.Seq[Snapshot[AnyKeyedEvent]]] = {
    val eventId = snapshot.eventId
    val anyKeyedEvent = KeyedEvent(snapshot.value).asInstanceOf[AnyKeyedEvent]
    Snapshot(eventId, List(Snapshot(eventId, anyKeyedEvent)))
  }
}
