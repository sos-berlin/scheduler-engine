package com.sos.scheduler.engine.tests.jira.js1670

import akka.actor.ActorSystem
import com.sos.scheduler.engine.base.generic.SecretString
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAny
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.sprayutils.https.Https._
import com.sos.scheduler.engine.common.sprayutils.https.KeystoreReference
import com.sos.scheduler.engine.common.sprayutils.web.auth.GateKeeper
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPorts
import com.sos.scheduler.engine.common.utils.JavaResource
import com.sos.scheduler.engine.data.scheduler.{SchedulerId, SchedulerOverview}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1670.JS1670IT._
import java.time.Instant.now
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future
import spray.client.pipelining._
import spray.http.CacheDirectives.{`no-cache`, `no-store`}
import spray.http.HttpHeaders.{Accept, `Cache-Control`}
import spray.http.MediaTypes._
import spray.http.StatusCodes.Unauthorized
import spray.http.{BasicHttpCredentials, HttpRequest, HttpResponse}
import spray.httpx.SprayJsonSupport._
import spray.httpx.UnsuccessfulResponseException
import spray.httpx.encoding.Gzip
import spray.httpx.unmarshalling.FromResponseUnmarshaller

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1670IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val List(httpsPort, httpPort) = findRandomFreeTcpPorts(2)
  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(
      s"-http-port=127.0.0.1:$httpPort",
      s"-https-port=127.0.0.1:$httpsPort"))
  private lazy val httpsUri = s"https://127.0.0.1:$httpsPort/jobscheduler/master/api"
  private implicit lazy val actorSystem = ActorSystem("JS1670IT") withCloser { _.shutdown() }

  override protected def checkedBeforeAll() = {
    acceptTlsCertificateFor(ClientKeystoreRef, httpsUri)
    super.checkedBeforeAll()
  }

  private def pipeline[A: FromResponseUnmarshaller](password: Option[String]): HttpRequest ⇒ Future[A] =
    (password map { o ⇒ addCredentials(BasicHttpCredentials("TEST-USER", o)) } getOrElse identity[HttpRequest] _) ~>
    addHeader(Accept(`application/json`)) ~>
    addHeader(`Cache-Control`(`no-cache`, `no-store`)) ~>
    encode(Gzip) ~>
    sendReceive ~>
    decode(Gzip) ~>
    unmarshal[A]

  "HTTPS" - {
    lazy val uri = httpsUri

    "Unauthorized due to missing credentials" in {
      intercept[UnsuccessfulResponseException] {
        pipeline[HttpResponse](password = None).apply(Get(uri)) await 10.s
      }
      .response.status shouldEqual Unauthorized
    }

    "Unauthorized due to wrong credentials" in {
      val t = now
      val e = intercept[UnsuccessfulResponseException] {
        pipeline[SchedulerOverview](Some("WRONG-PASSWORD")).apply(Get(uri)) await 10.s
      }
      assert(now - t > instance[GateKeeper.Configuration].invalidAuthenticationDelay - 50.ms)  // Allow for timer rounding
      e.response.status shouldEqual Unauthorized
    }

    "Authorized" in {
      val overview = pipeline[SchedulerOverview](Some("TEST-PASSWORD")).apply(Get(uri)) await 10.s
      assert(overview.schedulerId == SchedulerId("test"))
    }
  }

  "HTTP" - {
    lazy val uri = s"http://127.0.0.1:$httpPort/jobscheduler/master/api"

    "Without credentials" in {
      val overview = pipeline[SchedulerOverview](password = None).apply(Get(uri)) await 10.s
      assert(overview.schedulerId == SchedulerId("test"))
    }

    "Credentials are ignored" in {
      val overview = pipeline[SchedulerOverview](Some("WRONG-PASSWORD")).apply(Get(uri)) await 10.s
      assert(overview.schedulerId == SchedulerId("test"))
    }
  }
}

private object JS1670IT {
  private val ClientKeystoreRef = KeystoreReference(
    JavaResource("com/sos/scheduler/engine/tests/jira/js1670/public-https.jks").url,
    Some(SecretString("jobscheduler")))
}
