package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.api.{FileBasedClient, OrderClient, SchedulerOverviewClient}
import com.sos.scheduler.engine.client.web.common.QueryHttp.{jobChainNodeQuery, orderQuery}
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.event.{AnyEvent, EventId}
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat.anyEventJsonFormat
import com.sos.scheduler.engine.data.order.{OrderDetailed, OrderEvent, OrderKey, OrderOverview, Orders}
import com.sos.scheduler.engine.data.queries.OrderQuery
import com.sos.scheduler.engine.kernel.event.DirectEventClient
import com.sos.scheduler.engine.kernel.order.OrderSubsystemClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.OrderRoute._
import com.sos.scheduler.engine.plugins.newwebservice.routes.SchedulerDirectives.typedPath
import com.sos.scheduler.engine.plugins.newwebservice.routes.log.LogRoute
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.jsonToYamlHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.{OrdersHtmlPage, SingleKeyEventHtmlPage}
import scala.concurrent.ExecutionContext
import spray.httpx.marshalling.ToResponseMarshallable.isMarshallable
import spray.json.DefaultJsonProtocol._
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
    (testSlash(webServiceContext) | pass) {
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

        case "Event" ⇒
          parameter("after" ? EventId.BeforeFirst) { afterEventId ⇒
            implicit val toHtmlPage = SingleKeyEventHtmlPage.singleKeyEventToHtmlPage(orderKey)
            completeTryHtml(client.eventsForKey[AnyEvent](orderKey, after = afterEventId))
          }

        case "OrderEvent" ⇒
          parameter("after" ? EventId.BeforeFirst) { afterEventId ⇒
            completeTryHtml(client.eventsForKey[OrderEvent](orderKey, after = afterEventId))
          }

        case o ⇒
          reject(ValidationRejection(s"Invalid parameter return=$o"))
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

      case Some(o) ⇒
        reject(ValidationRejection(s"Unknown value for parameter return=$o"))

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
