package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.api.{FileBasedClient, OrderClient, SchedulerOverviewClient}
import com.sos.scheduler.engine.client.web.common.QueryHttp._
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.common.sprayutils.SprayUtils.{asFromStringOptionDeserializer, completeWithError}
import com.sos.scheduler.engine.data.event._
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat.anyEventJsonFormat
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderDetailed, OrderKey, OrderOverview, Orders}
import com.sos.scheduler.engine.data.queries.{OrderQuery, PathQuery}
import com.sos.scheduler.engine.kernel.event.{DirectEventClient, OrderStatisticsChangedSource}
import com.sos.scheduler.engine.kernel.order.OrderSubsystemClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.OrderRoute._
import com.sos.scheduler.engine.plugins.newwebservice.routes.SchedulerDirectives.typedPath
import com.sos.scheduler.engine.plugins.newwebservice.routes.event.EventRoutes._
import com.sos.scheduler.engine.plugins.newwebservice.routes.log.LogRoute
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.jsonToYamlHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.{OrdersHtmlPage, SingleKeyEventHtmlPage}
import scala.collection.immutable.Seq
import scala.concurrent.ExecutionContext
import scala.reflect.ClassTag
import spray.http.StatusCodes.NotImplemented
import spray.httpx.marshalling.ToResponseMarshallable.isMarshallable
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
trait OrderRoute extends LogRoute {

  protected def orderSubsystem: OrderSubsystemClient
  protected def orderStatisticsChangedSource: OrderStatisticsChangedSource
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
        case Some("OrderStatisticsChanged") ⇒
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

      case "FileBasedDetailed" ⇒
        completeTryHtml(client.fileBasedDetailed(orderKey))

      case returnType ⇒
        anyEventJsonFormat.typeNameToClass.get(returnType) match {
          case Some(eventClass) ⇒ orderEvents(eventClass, orderKey)
          case None ⇒ rejectReturnType(returnType)
        }
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

      case Some(returnType) ⇒
        anyEventJsonFormat.typeNameToClass.get(returnType) match {
          case Some(eventClass) ⇒
            query.orderKeyOption match {
              case Some(orderKey) ⇒ orderEvents(eventClass, orderKey)
              case None ⇒ completeWithError(NotImplemented, s"return=$returnType is supported only for single OrderKey queries")
            }
          case _ ⇒ rejectReturnType(returnType)
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

  private def orderEvents(eventClass: Class[_ <: Event], orderKey: OrderKey): Route =
    withEventParameters { case EventParameters(_, afterEventId, timeout, limit) ⇒
      implicit val toHtmlPage = SingleKeyEventHtmlPage.singleKeyEventToHtmlPage[AnyEvent](orderKey)
      completeTryHtml {
        if (limit >= 0)
          client.eventsForKey[AnyEvent](orderKey, after = afterEventId, timeout, limit = limit)(ClassTag(eventClass))
        else
          for (responseSnapshot ← client.eventsReverseForKey[AnyEvent](orderKey, after = afterEventId, limit = -limit)(ClassTag(eventClass))) yield
            for (events ← responseSnapshot) yield
              EventSeq.NonEmpty(events)
      }
    }

  private def orderStatisticsChanged(query: PathQuery): Route =
    withEventParameters {
      case EventParameters("OrderStatisticsChanged", afterEventId, timeout, limit) ⇒
        completeTryHtml[EventSeq[Seq, AnyKeyedEvent]] {
          for (snapshot ← orderStatisticsChangedSource.whenOrderStatisticsChanged(after = afterEventId, timeout, query))
            yield nestIntoSeqSnapshot(snapshot)
        }
      case _ ⇒ reject
  }

  private def rejectReturnType(returnType: String) =
    reject(ValidationRejection(s"Unknown value for parameter return=$returnType"))
}

object OrderRoute {
  private val OrderTreeComplementedName = "OrderTreeComplemented"
  private val OrdersComplementedName = "OrdersComplemented"
  private val ReturnTypeRegex = {
    val word = "([A-Za-z]+)"
    s"$word(?:/$word)?".r
  }
}
