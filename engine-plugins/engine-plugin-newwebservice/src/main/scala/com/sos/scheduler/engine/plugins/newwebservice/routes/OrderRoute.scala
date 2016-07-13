package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.web.order.OrderQueryHttp.directives.orderQuery
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.order.OrderQuery
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.{completeTryHtml, htmlPreferred}
import com.sos.scheduler.engine.plugins.newwebservice.html.TextHtmlPage.implicits._
import com.sos.scheduler.engine.plugins.newwebservice.html.{OrdersFullOverviewHtmlPage, WebServiceContext}
import com.sos.scheduler.engine.plugins.newwebservice.json.JsonProtocol._
import scala.concurrent.ExecutionContext
import spray.routing.Directives._
import spray.routing.{Route, ValidationRejection}

/**
  * @author Joacim Zschimmer
  */
trait OrderRoute {

  protected implicit def client: DirectSchedulerClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  protected final def orderRoute: Route =
    // unmatchedPath is eaten by orderQuery
    get {
      parameterMap { parameters ⇒
        val returnType = parameters.getOrElse("return", "OrdersFullOverview")
        orderQuery(parameters - "return") { query ⇒
          returnType match {
            case "OrdersFullOverview" ⇒ ordersFullOverviewRoute(query)
            case "OrderOverview" ⇒ completeTryHtml(client.orderOverviews(query))
            case o ⇒ reject(ValidationRejection(s"Invalid parameter return=$o"))
          }
        }
      }
    }

  private def ordersFullOverviewRoute(query: OrderQuery) = {
    val future = client.ordersFullOverview(query)
    htmlPreferred(webServiceContext) {
      complete(future flatMap OrdersFullOverviewHtmlPage.toHtml(query))
    } ~
      complete(future)
  }
}
