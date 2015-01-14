package com.sos.scheduler.engine.agent.web

import akka.actor.ActorSystem
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import scala.concurrent.Future
import spray.http.StatusCodes.InternalServerError
import spray.testkit.ScalatestRouteTest

/**
 * @author Joacim Zschimmer
 */
final class AgentWebServiceTest extends FreeSpec with ScalatestRouteTest with AgentWebService {

  implicit val actorRefFactory = ActorSystem("TEST")

  "Good command" in {
    Post("/jobscheduler/engine/command", "test") ~> route ~> check {
      responseAs[String] shouldEqual "<ok/>"
    }
  }

  "Exception" in {
    Post("/jobscheduler/engine/command", "error") ~> route ~> check {
      assert(status == InternalServerError)
    }
  }

  protected def executeCommand(command: String) = Future.successful[xml.Elem](
    command match {
      case "test" ⇒ <ok/>
      case _ ⇒ throw new Exception
    })
}
