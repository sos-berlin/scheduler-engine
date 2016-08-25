package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.api.{OrderClient, SchedulerOverviewClient}
import com.sos.scheduler.engine.client.web.order.OrderQueryHttp.directives.extendedOrderQuery
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.order.{OrderDetailed, OrderKey, OrderOverview}
import com.sos.scheduler.engine.kernel.order.OrderSubsystemClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.{completeTryHtml, htmlPreferred}
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.json.JsonProtocol._
import com.sos.scheduler.engine.plugins.newwebservice.routes.OrderRoute._
import com.sos.scheduler.engine.plugins.newwebservice.routes.SchedulerDirectives.typedPath
import com.sos.scheduler.engine.plugins.newwebservice.routes.log.LogRoute
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.OrdersHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.jsonToYamlHtmlPage
import scala.concurrent.ExecutionContext
import spray.routing.Directives._
import spray.routing.{Route, ValidationRejection}

/**
  * @author Joacim Zschimmer
  */
trait OrderRoute extends LogRoute {

  protected def orderSubsystem: OrderSubsystemClient
  protected implicit def client: OrderClient with SchedulerOverviewClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  protected final def orderRoute: Route =
    get {
      typedPath(OrderKey) { orderKey ⇒
        parameter("return") {
          case "log" ⇒ logRoute(orderSubsystem.order(orderKey).log)
          case o ⇒ reject(ValidationRejection(s"Invalid parameter return=$o"))
        }
      } ~
      extendedOrderQuery { implicit query ⇒
        parameter("return".?) {

          case Some("OrderTreeComplemented") ⇒
            completeTryHtml(client.orderTreeComplementedBy[OrderOverview](query))

          case Some(ReturnTypeRegex("OrderTreeComplemented", OrderOverview.name | "")) ⇒
            completeTryHtml(client.orderTreeComplementedBy[OrderOverview](query))

          case Some(ReturnTypeRegex("OrderTreeComplemented", OrderDetailed.name)) ⇒
            completeTryHtml(client.orderTreeComplementedBy[OrderDetailed](query))

          case Some(ReturnTypeRegex("OrdersComplemented", OrderOverview.name | "")) ⇒
            completeTryHtml(client.ordersComplementedBy[OrderOverview](query))

          case Some(ReturnTypeRegex("OrdersComplemented", OrderDetailed.name)) ⇒
            completeTryHtml(client.ordersComplementedBy[OrderDetailed](query))

          case Some(OrderOverview.name) ⇒
            completeTryHtml(client.ordersBy[OrderOverview](query))

          case Some(OrderDetailed.name) ⇒
            completeTryHtml(client.ordersBy[OrderDetailed](query))

          case Some(o) ⇒
            reject(ValidationRejection(s"Invalid parameter return=$o"))

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
    }
}

object OrderRoute {
  private val ReturnTypeRegex = "([A-Za-z]+)(?:/([A-Za-z]*))".r

//  private def toOrderViewCompanion(string: String) =
//    string match {
//      case "" ⇒ OrderOverview
//      case _ ⇒ OrderView.Companion.option(string) getOrElse {
//        throw new RejectionError(ValidationRejection(s"Unknown OrderView: $string"))
//      }
//    }
}
