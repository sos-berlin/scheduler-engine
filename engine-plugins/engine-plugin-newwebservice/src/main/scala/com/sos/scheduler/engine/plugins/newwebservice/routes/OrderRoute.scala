package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.web.order.OrderQueryHttp.directives.extendedOrderQuery
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.order.OrderSubsystemClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.{completeTryHtml, htmlPreferred}
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.json.JsonProtocol._
import com.sos.scheduler.engine.plugins.newwebservice.routes.SchedulerDirectives.typedPath
import com.sos.scheduler.engine.plugins.newwebservice.routes.log.LogRoute
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.OrdersHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.toYamlHtmlPage
import scala.concurrent.ExecutionContext
import spray.routing.Directives._
import spray.routing.{Route, ValidationRejection}

/**
  * @author Joacim Zschimmer
  */
trait OrderRoute extends LogRoute {

  protected def orderSubsystem: OrderSubsystemClient
  protected implicit def client: DirectSchedulerClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  protected final def orderRoute: Route =
    get {
      parameterMap { parameters ⇒
        parameters.get("return") match {
          case Some("log") ⇒
            typedPath(OrderKey) { orderKey ⇒
              logRoute(orderSubsystem.order(orderKey).log)
            }
          case returnType ⇒
            extendedOrderQuery { implicit query ⇒
              returnType match {
                case Some("OrderTreeComplemented") ⇒ completeTryHtml(client.orderTreeComplementedBy(query))
                case Some("OrdersComplemented") ⇒ completeTryHtml(client.ordersComplementedBy(query))
                case Some("OrderOverview") ⇒ completeTryHtml(client.orderOverviewsBy(query))
                case Some(o) ⇒ reject(ValidationRejection(s"Invalid parameter return=$o"))
                case None ⇒
                  htmlPreferred(webServiceContext) {
                    requestUri { uri ⇒
                      complete(
                        for (o ← client.ordersComplementedBy(query)) yield
                          OrdersHtmlPage.toHtmlPage(o, uri, query, client, webServiceContext))
                    }
                  } ~
                    complete(client.orderTreeComplementedBy(query))
              }
            }
        }
      }
    }
}
