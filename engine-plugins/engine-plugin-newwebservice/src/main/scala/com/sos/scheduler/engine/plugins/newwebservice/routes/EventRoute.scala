package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, Event, EventId, KeyedEvent, NoKeyEvent, Snapshot}
import com.sos.scheduler.engine.data.events.{SchedulerAnyKeyedEventJsonFormat, schedulerKeyedEventJsonFormat}
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
      parameter("after" ? EventId.BeforeFirst) { afterEventId ⇒
        parameter("limit" ? Int.MaxValue) { limit ⇒
          if (limit < 1)
            reject(ValidationRejection(s"Invlid limit=$limit"))
          else
            parameter("return" ? classOf[Event].getSimpleName) { returnType ⇒
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
    }
}

object EventRoute {
  // Nests a simple Snapshot[NoKeyEvent] into the expected nested type for the event web service.
  private def nestIntoSeqSnapshot[E <: NoKeyEvent](snapshot: Snapshot[E]): Snapshot[immutable.Seq[Snapshot[AnyKeyedEvent]]] = {
    val anyKeyedEvent = KeyedEvent(snapshot.value).asInstanceOf[AnyKeyedEvent]
    Snapshot(List(Snapshot(anyKeyedEvent)(snapshot.eventId)))(snapshot.eventId)
  }
}
