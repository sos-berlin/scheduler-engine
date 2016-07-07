package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.web.order.OrderQueryHttp.directives.orderQuery
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.html.OrdersFullOverviewHtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.json.JsonProtocol._
import scala.concurrent.ExecutionContext
import spray.routing.Directives._
import spray.routing.{Route, ValidationRejection}

/**
  * @author Joacim Zschimmer
  */
trait OrderRoute {

  protected implicit def client: DirectSchedulerClient
  protected implicit def executionContext: ExecutionContext

  protected final def orderRoute: Route =
    get {
      parameterMap { parameters ⇒
        val returnType = parameters.getOrElse("return", "OrdersFullOverview")
        orderQuery(parameters - "return") { query ⇒
          returnType match {
            case "OrdersFullOverview" ⇒ completeAsHtmlPageOrOther(client.ordersFullOverview(query))
            case "OrderOverview" ⇒ complete(client.orderOverviews(query))
            case o ⇒ reject(ValidationRejection(s"Invalid parameter return=$o"))
          }
        }
      }
    }
}
