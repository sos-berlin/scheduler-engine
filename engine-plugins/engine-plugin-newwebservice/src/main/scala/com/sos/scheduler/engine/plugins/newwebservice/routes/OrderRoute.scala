package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.api.{FileBasedClient, OrderClient, SchedulerOverviewClient}
import com.sos.scheduler.engine.client.web.common.QueryHttp.{jobChainNodeQuery, orderQuery}
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.common.sprayutils.SprayUtils.asFromStringOptionDeserializer
import com.sos.scheduler.engine.data.event._
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat.anyEventJsonFormat
import com.sos.scheduler.engine.data.order.{OrderDetailed, OrderKey, OrderOverview, Orders}
import com.sos.scheduler.engine.data.queries.OrderQuery
import com.sos.scheduler.engine.kernel.event.DirectEventClient
import com.sos.scheduler.engine.kernel.order.OrderSubsystemClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.OrderRoute._
import com.sos.scheduler.engine.plugins.newwebservice.routes.SchedulerDirectives.typedPath
import com.sos.scheduler.engine.plugins.newwebservice.routes.event.EventRoutes.{EventParameters, withEventParameters}
import com.sos.scheduler.engine.plugins.newwebservice.routes.log.LogRoute
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.toHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.{KeyedEventsHtmlPage, OrdersHtmlPage, SingleKeyEventHtmlPage}
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.KeyedEventsHtmlPage
import scala.collection.immutable.Seq
import scala.concurrent.ExecutionContext
import scala.reflect.ClassTag
import spray.httpx.marshalling.ToResponseMarshallable.isMarshallable
import spray.routing.Directives._
import spray.routing.{Route, ValidationRejection}

/**
  * @author Joacim Zschimmer
  */
trait OrderRoute extends LogRoute {

  protected def orderSubsystem: OrderSubsystemClient
  protected implicit def client: OrderClient with SchedulerOverviewClient with FileBasedClient with DirectEventClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  protected final def orderRoute: Route =
    (getRequiresSlash(webServiceContext) | pass) {
      parameter("return".?) {
        case Some("OrderStatistics") ⇒
          jobChainNodeQuery { query ⇒
            completeTryHtml(client.orderStatistics(query))
          }
        case _ ⇒
          singleOrder ~
          orderQuery { query ⇒
            queriedOrders(query)
          }
      }
    }

  private def singleOrder: Route =
    typedPath(OrderKey) { orderKey ⇒
      parameter("return" ? "OrderDetailed") {
        case "log" ⇒
          logRoute(orderSubsystem.order(orderKey).log)

        case "OrderOverview" ⇒
          completeTryHtml(client.order[OrderOverview](orderKey))

        case "OrderDetailed" ⇒
          completeTryHtml(client.order[OrderDetailed](orderKey))

        case "FileBasedDetailed" ⇒
          completeTryHtml(client.fileBasedDetailed(orderKey))

        case returnType ⇒
          anyEventJsonFormat.typeNameToClass.get(returnType) match {
            case Some(eventClass) ⇒
              withEventParameters { case EventParameters(`returnType`, afterEventId, timeout, limit) ⇒
                completeTryHtml {
                  if (limit >= 0)
                    client.eventsForKey[AnyEvent](orderKey, after = afterEventId, timeout, limit = limit)(ClassTag(eventClass))
                  else
                    for (responseSnapshot ← client.eventsReverseForKey[AnyEvent](orderKey, after = afterEventId, limit = -limit)(ClassTag(eventClass))) yield
                      for (events ← responseSnapshot) yield
                        EventSeq.NonEmpty(events)
                }
              }
            case None ⇒
              reject(ValidationRejection(s"Invalid parameter return=$returnType"))
          }
      }
    }

  private def queriedOrders(implicit query: OrderQuery): Route =
    parameter("return".?) {
      case Some(ReturnTypeRegex(OrderTreeComplementedName, OrderOverview.name)
                | OrderTreeComplementedName) ⇒
        completeTryHtml(client.orderTreeComplementedBy[OrderOverview](query))

      case Some(ReturnTypeRegex(OrderTreeComplementedName, OrderDetailed.name)) ⇒
        completeTryHtml(client.orderTreeComplementedBy[OrderDetailed](query))

      case Some(ReturnTypeRegex(OrdersComplementedName, OrderOverview.name)
                | OrdersComplementedName)  ⇒
        completeTryHtml(client.ordersComplementedBy[OrderOverview](query))

      case Some(ReturnTypeRegex(OrdersComplementedName, OrderDetailed.name)) ⇒
        completeTryHtml(client.ordersComplementedBy[OrderDetailed](query))

      case Some(OrderOverview.name) ⇒
        completeTryHtml(client.ordersBy[OrderOverview](query) map { _ map Orders.apply })

      case Some(OrderDetailed.name) ⇒
        completeTryHtml(client.ordersBy[OrderDetailed](query) map { _ map Orders.apply })

      case Some(returnType) ⇒
        anyEventJsonFormat.typeNameToClass.get(returnType) match {
          case Some(eventClass) ⇒
            withEventParameters { case EventParameters(`returnType`, afterEventId, timeout, limit) ⇒
              import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat
              implicit val toHtmlPage: ToHtmlPage[Snapshot[EventSeq[Seq, AnyKeyedEvent]]] = KeyedEventsHtmlPage.implicits.toHtmlPage
              completeTryHtml {
              //complete {
                if (limit >= 0)
                  client.events[Event](after = afterEventId, timeout, limit = limit)(ClassTag(eventClass))
                else
                  for (responseSnapshot ← client.eventsReverse[Event](after = afterEventId, limit = -limit)(ClassTag(eventClass))) yield
                    for (events ← responseSnapshot) yield
                      EventSeq.NonEmpty(events)
              }
            }
          case None ⇒
            reject(ValidationRejection(s"Invalid parameter return=$returnType"))
        }

      case None ⇒
        htmlPreferred(webServiceContext) {
          requestUri { uri ⇒
            complete(
              for (o ← client.ordersComplementedBy[OrderOverview](query)) yield
                OrdersHtmlPage.toHtmlPage(o, uri, query, client, webServiceContext))
          }
        } ~
          complete(client.orderTreeComplementedBy[OrderOverview](query))
    }
}

object OrderRoute {
  private val OrderTreeComplementedName = "OrderTreeComplemented"
  private val OrdersComplementedName = "OrdersComplemented"
  private val ReturnTypeRegex = {
    val word = "([A-Za-z]+)"
    s"$word(?:/$word)?".r
  }

//  private def toOrderViewCompanion(string: String) =
//    string match {
//      case "" ⇒ OrderOverview
//      case _ ⇒ OrderView.Companion.option(string) getOrElse {
//        throw new RejectionError(ValidationRejection(s"Unknown OrderView: $string"))
//      }
//    }
}
