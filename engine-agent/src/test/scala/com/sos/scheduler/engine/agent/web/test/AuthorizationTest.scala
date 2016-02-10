package com.sos.scheduler.engine.agent.web.test

import akka.actor.ActorRefFactory
import akka.util.Timeout
import com.sos.scheduler.engine.agent.web.test.AuthorizationTest._
import org.scalatest.FreeSpec
import scala.concurrent.Future
import scala.concurrent.duration._
import spray.json.DefaultJsonProtocol._
import spray.routing.AuthenticationFailedRejection
import spray.routing.AuthenticationFailedRejection.{CredentialsMissing, CredentialsRejected}
import spray.routing.Directives._
import spray.routing.authentication.Authentication
import spray.routing.directives.AuthMagnet._

/**
  * @author Joacim Zschimmer
  */
final class AuthorizationTest extends FreeSpec with WebServiceTest with AuthorizationTest.WebService {

  "Without access token" in {
    val requestString = "TEST-REQUEST"
    Post(s"/test", requestString) ~> route ~> check {
      //assert(status == Forbidden)
      assert(rejection == AuthenticationFailedRejection(CredentialsMissing, Nil))
    }
  }

  "With valid access token" in {
    val requestString = "TEST-REQUEST"
    Post("/test", requestString) ~> addHeader("X-JobScheduler-Access-Token", "SECRET-TOKEN") ~> route ~> check {
      assert(responseAs[String] == requestToResponse(requestString))
    }
  }

  "With invalid access token" in {
    val requestString = "TEST-REQUEST"
    Post("/test", requestString) ~> addHeader("X-JobScheduler-Access-Token", "INVALID-TOKEN") ~> route ~> check {
      assert(rejection == AuthenticationFailedRejection(CredentialsRejected, Nil))
    }
  }
}

object AuthorizationTest {
  private implicit val AskTimeout: Timeout = Timeout(30.seconds)

  trait WebService {
    protected val actorRefFactory: ActorRefFactory
    import actorRefFactory.dispatcher

    protected def route =
      path("test") {
        post {
          optionalHeaderValueByName("X-JobScheduler-Access-Token") {
            case None ⇒
              reject(AuthenticationFailedRejection(CredentialsMissing, Nil))
              //complete(Forbidden)
            case Some(accessToken) ⇒
               authenticate(validate(accessToken)) { _: Unit ⇒
                 entity(as[String]) { string ⇒
                   complete {
                     requestToResponse(string)
                   }
                 }
               }
          }
        }
      }

    private def validate(token: String): Future[Authentication[Unit]] =
      Future.successful(
        token match {
          case "SECRET-TOKEN" ⇒ Right(())
          case _ ⇒ Left(AuthenticationFailedRejection(CredentialsRejected, Nil))
        }
      )
  }

  private def requestToResponse(requestString: String) = s"TEST-RESPONSE FOR $requestString"
}
