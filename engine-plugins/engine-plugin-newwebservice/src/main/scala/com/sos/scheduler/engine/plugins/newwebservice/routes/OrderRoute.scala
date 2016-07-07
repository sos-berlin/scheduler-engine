package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.client.web.order.OrderQueryHttp.directives.orderQuery
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.html.OrdersFullOverviewHtmlPage._
import com.sos.scheduler.engine.plugins.newwebservice.json.JsonProtocol._
import scala.concurrent.ExecutionContext
import spray.http.StatusCodes.TemporaryRedirect
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
trait OrderRoute {

  protected implicit def client: DirectSchedulerClient
  protected implicit def executionContext: ExecutionContext

  protected final def orderRoute: Route =
    (pathSingleSlash & get) {
       redirect("OrdersFullOverview", TemporaryRedirect)
    } ~
    (pathPrefix("OrderOverview") & pathSingleSlash) {
      get {
        orderQuery { query ⇒
          complete(client.orderOverviews(query))
        }
      }
    } ~
    (path("OrdersFullOverview") & pathEnd) {
      get {
        orderQuery { query ⇒
          completeAsHtmlPageOrOther(client.ordersFullOverview(query))
        }
      }
    }
}
