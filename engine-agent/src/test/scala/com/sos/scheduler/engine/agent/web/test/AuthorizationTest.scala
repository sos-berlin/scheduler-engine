package com.sos.scheduler.engine.agent.web.test

import akka.actor.ActorRefFactory
import akka.util.Timeout
import com.sos.scheduler.engine.agent.web.test.AuthorizationTest._
import org.scalatest.FreeSpec
import scala.concurrent.Future
import scala.concurrent.duration._
import spray.http.{BasicHttpCredentials, HttpChallenge}
import spray.json.DefaultJsonProtocol._
import spray.routing.AuthenticationFailedRejection
import spray.routing.AuthenticationFailedRejection.{CredentialsMissing, CredentialsRejected}
import spray.routing.Directives._
import spray.routing.authentication.{BasicAuth, UserPass, Authentication}
import spray.routing.directives.AuthMagnet._
import spray.http.HttpHeaders._
import spray.http.StatusCodes._
/**
  * @author Joacim Zschimmer
  */
final class AuthorizationTest extends FreeSpec with WebServiceTest with AuthorizationTest.WebService {

  "Basic authentication" - {
    "Without login" in {
      val requestString = "TEST-REQUEST"
      Post(s"/basic", requestString) ~> route ~> check {
        assert(rejection == AuthenticationFailedRejection(
          CredentialsMissing,
          `WWW-Authenticate`(HttpChallenge(scheme = "Basic", realm = Realm)) :: Nil))
      }
    }

    "With valid access token" in {
      val requestString = "TEST-REQUEST"
      Post("/basic", requestString) ~> addCredentials(BasicHttpCredentials(LoginName, Password)) ~> route ~> check {
        assert(responseAs[String] == requestToResponse(requestString))
      }
    }

    "With invalid access token" in {
      val requestString = "TEST-REQUEST"
      Post("/basic", requestString) ~> addCredentials(BasicHttpCredentials(LoginName, "INVALID-PASSWORD")) ~> route ~> check {
        assert(rejection == AuthenticationFailedRejection(
          CredentialsRejected,
          `WWW-Authenticate`(HttpChallenge(scheme = "Basic", realm = Realm)) :: Nil))
      }
    }
  }

  "Own access token" - {
    "Without access token" in {
      val requestString = "TEST-REQUEST"
      Post(s"/accessToken", requestString) ~> route ~> check {
        //assert(status == Forbidden)
        assert(rejection == AuthenticationFailedRejection(CredentialsMissing, Nil))
      }
    }

    "With valid access token" in {
      val requestString = "TEST-REQUEST"
      Post("/accessToken", requestString) ~> addHeader("X-JobScheduler-Access-Token", "SECRET-TOKEN") ~> route ~> check {
        assert(responseAs[String] == requestToResponse(requestString))
      }
    }

    "With invalid access token" in {
      val requestString = "TEST-REQUEST"
      Post("/accessToken", requestString) ~> addHeader("X-JobScheduler-Access-Token", "INVALID-TOKEN") ~> route ~> check {
        assert(rejection == AuthenticationFailedRejection(CredentialsRejected, Nil))
      }
    }
  }

  "Access token via basic authentication" - {
    "Without access token" in {
      val requestString = "TEST-REQUEST"
      Post("/basicAccessToken", requestString) ~> route ~> check {
        assert(rejection == AuthenticationFailedRejection(
          CredentialsMissing,
          `WWW-Authenticate`(HttpChallenge(scheme = "Basic", realm = Realm)) :: Nil))
      }
    }

    "With valid access token" in {
      val requestString = "TEST-REQUEST"
      Post("/basicAccessToken", requestString) ~> addCredentials(BasicHttpCredentials("access-token", "SECRET-TOKEN")) ~> route ~> check {
        assert(responseAs[String] == requestToResponse(requestString))
      }
    }

    "With invalid access token" in {
      val requestString = "TEST-REQUEST"
      Post("/basicAccessToken", requestString) ~> addCredentials(BasicHttpCredentials("access-token", "INVALID-TOKEN")) ~> route ~> check {
        assert(rejection == AuthenticationFailedRejection(
          CredentialsRejected,
          `WWW-Authenticate`(HttpChallenge(scheme = "Basic", realm = Realm)) :: Nil))
      }
    }
  }
}

object AuthorizationTest {
  private val Realm = "JobScheduler Agent"
  private val LoginName = "TEST-USER"
  private val Password = "TEST-PASSWORD"
  private implicit val AskTimeout: Timeout = Timeout(30.seconds)

  trait WebService {
    protected val actorRefFactory: ActorRefFactory
    import actorRefFactory.dispatcher

    protected def route =
      path("basic") {
        authenticate(BasicAuth(myUserPassAuthenticator _, realm = Realm)) { userName ⇒
          entity(as[String]) { string ⇒
            complete {
              requestToResponse(string)
            }
          }
        }
      } ~
      path("accessToken") {
        post {
          optionalHeaderValueByName("X-JobScheduler-Access-Token") {
            case None ⇒
              reject(AuthenticationFailedRejection(CredentialsMissing, Nil))
              //complete(Forbidden)
            case Some(accessToken) ⇒
               authenticate(validateAccessToken(accessToken)) { _: Unit ⇒
                 entity(as[String]) { string ⇒
                   complete {
                     requestToResponse(string)
                   }
                 }
               }
          }
        }
      } ~
      path("basicAccessToken") {
        post {
          authenticate(BasicAuth(myUserPassAuthenticator _, realm = Realm)) { userName ⇒
            entity(as[String]) { string ⇒
              complete {
                requestToResponse(string)
              }
            }
          }
        }
      }

    def myUserPassAuthenticator(userPass: Option[UserPass]) = Future[Option[String]] {
      userPass match {
        case Some(UserPass(LoginName, Password)) ⇒ Some(LoginName)
        case Some(UserPass("access-token", "SECRET-TOKEN")) ⇒ Some(LoginName)
        case _ ⇒ None
      }
    }

    private def validateAccessToken(token: String): Future[Authentication[Unit]] =
      Future.successful(
        token match {
          case "SECRET-TOKEN" ⇒ Right(())
          case _ ⇒ Left(AuthenticationFailedRejection(CredentialsRejected, Nil))
        }
      )
  }

  private def requestToResponse(requestString: String) = s"TEST-RESPONSE FOR $requestString"
}
