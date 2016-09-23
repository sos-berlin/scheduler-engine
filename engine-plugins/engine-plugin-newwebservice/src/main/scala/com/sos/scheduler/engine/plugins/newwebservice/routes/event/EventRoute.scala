package com.sos.scheduler.engine.plugins.newwebservice.routes.event

import com.sos.scheduler.engine.base.convert.As._
import com.sos.scheduler.engine.base.convert.ConvertiblePartialFunctions.ImplicitConvertablePF
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, Event, EventId, Snapshot}
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.event.OrderStatisticsChangedSource
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.completeTryHtml
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.event.EventRoutes._
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.KeyedEventsHtmlPage.implicits.keyedEventsToHtmlPage
import scala.collection.immutable
import scala.concurrent.ExecutionContext
import scala.math.abs
import scala.reflect.ClassTag
import shapeless.{::, HNil}
import spray.json.DefaultJsonProtocol._
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
trait EventRoute extends HasCloser with OrderEventRoute {

  protected def orderStatisticsChangedSource: OrderStatisticsChangedSource
  protected implicit def client: DirectSchedulerClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  def eventRoute: Route =
    parameter("return".?) {
      case Some("OrderStatisticsChanged") ⇒ orderPathEventRoute
      case _ ⇒ otherEventRoute
    }

  private def otherEventRoute: Route =
    withEventParameters { case EventParameters(returnType, afterEventId, limit) ⇒
      pathSingleSlash {
        completeTryHtml[immutable.Seq[Snapshot[AnyKeyedEvent]]] {
          val classTag = ClassTag[Event](SchedulerAnyKeyedEventJsonFormat.typeToClass(returnType))
          client.events[Event](after = afterEventId, limit = abs(limit), reverse = limit < 0)(classTag)
        }
      }
    }

  private object withEventParameters extends Directive1[EventParameters] {
    def happly(inner: EventParameters :: HNil ⇒ Route) =
      parameterMap { parameters ⇒
        val returnType = parameters.as[String]("return", classOf[Event].getSimpleName)
        val afterEventId = parameters.as[Long]("after", EventId.BeforeFirst)
        val limit = parameters.as[Int]("limit", Int.MaxValue)
        //val timeout = parameters.optionAs[Int]("timeout") map { o ⇒ Duration.ofSeconds(o) }
        if (limit == 0)
          reject(ValidationRejection(s"Invalid limit=$limit"))
        else
          inner(EventParameters(returnType, afterEventId, limit) :: HNil)
      }
    }
}
