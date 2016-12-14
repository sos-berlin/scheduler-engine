package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.api.{OrderClient, SchedulerOverviewClient}
import com.sos.scheduler.engine.client.web.common.QueryHttp._
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.common.sprayutils.SprayUtils.asFromStringOptionDeserializer
import com.sos.scheduler.engine.data.event._
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{JocOrderStatisticsChanged, OrderDetailed, OrderKey, OrderOverview, Orders}
import com.sos.scheduler.engine.data.queries.{OrderQuery, PathQuery}
import com.sos.scheduler.engine.kernel.event.{DirectEventClient, JocOrderStatisticsChangedSource}
import com.sos.scheduler.engine.kernel.order.OrderSubsystemClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.OrderRoute._
import com.sos.scheduler.engine.plugins.newwebservice.routes.SchedulerDirectives.typedPath
import com.sos.scheduler.engine.plugins.newwebservice.routes.event.EventRoutes._
import com.sos.scheduler.engine.plugins.newwebservice.routes.log.LogRoute
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.OrdersHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.jsonToYamlHtmlPage
import scala.collection.immutable.Seq
import scala.concurrent.ExecutionContext
import spray.httpx.marshalling.ToResponseMarshallable.isMarshallable
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
trait OrderRoute extends LogRoute {

  protected def orderSubsystem: OrderSubsystemClient
  protected def orderStatisticsChangedSource: JocOrderStatisticsChangedSource
  protected implicit def client: OrderClient with SchedulerOverviewClient with DirectEventClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  protected final def orderRoute: Route =
    (getRequiresSlash(webServiceContext) | pass) {
      parameter("return".?) {
        case Some("JocOrderStatistics") ⇒
          jobChainNodeQuery { query ⇒
            completeTryHtml(client.jocOrderStatistics(query))
          }
        case Some("JocOrderStatisticsChanged") ⇒
          pathQuery(JobChainPath)(orderStatisticsChanged)
        case returnTypeOption ⇒
          typedPath(OrderKey) { query ⇒
            singleOrder(returnTypeOption, query)
          } ~
          orderQuery { query ⇒
            queriedOrders(returnTypeOption, query)
          }
      }
    }

  private def singleOrder(returnTypeOption: Option[String], orderKey: OrderKey): Route =
    returnTypeOption getOrElse "OrderDetailed" match {
      case "log" ⇒
        logRoute(orderSubsystem.order(orderKey).log)

      case "OrderOverview" ⇒
        completeTryHtml(client.order[OrderOverview](orderKey))

      case "OrderDetailed" ⇒
        completeTryHtml(client.order[OrderDetailed](orderKey))

      case _ ⇒
        singleKeyEvents[AnyEvent](orderKey)
    }

  private def queriedOrders(returnTypeOption: Option[String], query: OrderQuery): Route =
    returnTypeOption match {
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

      case Some(_) ⇒
        query.orderKeyOption match {
          case Some(orderKey) ⇒
            singleKeyEvents[AnyEvent](orderKey)
          case None ⇒
            orderEvents(query.jobChainQuery.pathQuery)  // Events are only selected by JobChainPath !!!
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

  private def orderEvents(query: PathQuery): Route =
    events[Event](
      predicate = {
        case KeyedEvent(OrderKey(jobChainPath, _), _) ⇒ query.matches(jobChainPath)
        case _ ⇒ false
      })

  private def orderStatisticsChanged(query: PathQuery): Route =
    eventRequest(classOf[Event]) {
      case EventRequest(eventClass, afterEventId, timeout, limit) if eventClass == classOf[JocOrderStatisticsChanged] ⇒
        completeTryHtml[EventSeq[Seq, AnyKeyedEvent]] {
          for (snapshot ← orderStatisticsChangedSource.whenJocOrderStatisticsChanged(after = afterEventId, timeout, query))
            yield nestIntoSeqSnapshot(snapshot)
        }
      case _ ⇒
        reject
    }
}

object OrderRoute {
  private val OrderTreeComplementedName = "OrderTreeComplemented"
  private val OrdersComplementedName = "OrdersComplemented"
  private val ReturnTypeRegex = {
    val word = "([A-Za-z]+)"
    s"$word(?:/$word)?".r
  }
}
