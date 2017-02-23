package com.sos.scheduler.engine.tests.jira.js1670

import akka.actor.ActorSystem
import com.sos.jobscheduler.base.generic.SecretString
import com.sos.jobscheduler.common.scalautil.Closers.implicits.RichClosersAny
import com.sos.jobscheduler.common.scalautil.Futures.implicits._
import com.sos.jobscheduler.common.sprayutils.https.{Https, KeystoreReference}
import com.sos.jobscheduler.common.sprayutils.sprayclient.ExtendedPipelining.extendedSendReceive
import com.sos.jobscheduler.common.sprayutils.web.auth.GateKeeper
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.utils.FreeTcpPortFinder.findRandomFreeTcpPorts
import com.sos.jobscheduler.common.utils.JavaResource
import com.sos.jobscheduler.data.scheduler.SchedulerId
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC
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
import spray.can.Http.HostConnectorSetup
import spray.client.pipelining._
import spray.http.CacheDirectives.{`no-cache`, `no-store`}
import spray.http.HttpHeaders.{Accept, `Cache-Control`}
import spray.http.MediaTypes._
import spray.http.StatusCodes.{Forbidden, Unauthorized}
import spray.http.{BasicHttpCredentials, HttpRequest, HttpResponse, Uri}
import spray.httpx.SprayJsonSupport._
import spray.httpx.UnsuccessfulResponseException
import spray.httpx.encoding.Gzip
import spray.httpx.unmarshalling.{FromResponseUnmarshaller, PimpedHttpResponse}

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
  private val httpsSetup = Some(Https.toHostConnectorSetup(ClientKeystoreRef, httpsUri))

  private def pipeline[A: FromResponseUnmarshaller](password: Option[String], setupOption: Option[HostConnectorSetup] = None): HttpRequest ⇒ Future[A] =
    (password map { o ⇒ addCredentials(BasicHttpCredentials("TEST-USER", o)) } getOrElse identity[HttpRequest] _) ~>
    addHeader(Accept(`application/json`)) ~>
    addHeader(`Cache-Control`(`no-cache`, `no-store`)) ~>
    encode(Gzip) ~>
    extendedSendReceive(60.s.toFiniteDuration, setupOption) ~>
    decode(Gzip) ~>
    unmarshal[A]

  "show_state http_port" in {
    assert(((scheduler executeXml <show_state/>).elem \ "answer" \ "state" \ "@http_port").toString == s"127.0.0.1:$httpPort")
  }

  "show_state https_port" in {
    assert(((scheduler executeXml <show_state/>).elem \ "answer" \ "state" \ "@https_port").toString == s"127.0.0.1:$httpsPort")
  }

  "http_url" in {
    assert(inSchedulerThread { instance[SpoolerC].http_url } == s"https://127.0.0.1:$httpsPort")
  }

  "HTTPS" - {
    lazy val uri = httpsUri

    "Unauthorized request is rejected" - {
      "due to missing credentials" in {
        intercept[UnsuccessfulResponseException] {
          pipeline[HttpResponse](password = None, httpsSetup).apply(Get(uri)) await 10.s
        }
        .response.status shouldEqual Unauthorized
      }

      "due to wrong credentials" in {
        val t = now
        val e = intercept[UnsuccessfulResponseException] {
          pipeline[SchedulerOverview](Some("WRONG-PASSWORD"), httpsSetup).apply(Get(uri)) await 10.s
        }
        assert(now - t > instance[GateKeeper.Configuration].invalidAuthenticationDelay - 50.ms)  // Allow for timer rounding
        e.response.status shouldEqual Unauthorized
      }

      addPostTextPlainText(uri)
    }

    "Authorized request" - {
      val password = Some("TEST-PASSWORD")

      "is accepted" in {
        val overview = pipeline[SchedulerOverview](password, httpsSetup).apply(Get(uri)) await 10.s
        assert(overview.schedulerId == SchedulerId("test"))
      }

      addPostTextPlainText(uri, password, httpsSetup)
    }
  }

  "HTTP" - {
    lazy val uri = s"http://127.0.0.1:$httpPort/jobscheduler/master/api"

    "Without credentials" in {
      val overview = pipeline[SchedulerOverview](password = None).apply(Get(uri)) await 10.s
      assert(overview.schedulerId == SchedulerId("test"))
    }

    addPostTextPlainText(uri)

    "Credentials are ignored" in {
      val overview = pipeline[SchedulerOverview](Some("WRONG-PASSWORD")).apply(Get(uri)) await 10.s
      assert(overview.schedulerId == SchedulerId("test"))
    }
  }

  private def addPostTextPlainText(uri: Uri, password: Option[String] = None, setupOption: Option[HostConnectorSetup] = None): Unit =
    "POST plain/text is rejected due to CSRF" in {
      val response = intercept[UnsuccessfulResponseException] {
        pipeline[HttpResponse](password, setupOption).apply(Post(uri, "TEXT")) await 10.s
      } .response
      assert(response.status == Forbidden)
      assert(response.as[String].right.get == "HTML form POST is forbidden")
    }
}

private object JS1670IT {
  private val ClientKeystoreRef = KeystoreReference(
    JavaResource("com/sos/scheduler/engine/tests/jira/js1670/public-https.jks").url,
    Some(SecretString("jobscheduler")))
}
