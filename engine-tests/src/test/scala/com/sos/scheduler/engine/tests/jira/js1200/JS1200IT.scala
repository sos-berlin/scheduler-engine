package com.sos.scheduler.engine.tests.jira.js1200

import akka.actor.ActorSystem
import com.sos.jobscheduler.common.scalautil.Futures._
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.utils.FreeTcpPortFinder
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future
import spray.client.pipelining._
import spray.http.HttpHeaders.{Accept, `Content-Type`}
import spray.http.MediaTypes._
import spray.http.{HttpRequest, HttpResponse, MediaType}

/**
 * JS-1200
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1200IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val tcpPort = FreeTcpPortFinder.findRandomFreeTcpPort()
  override protected lazy val testConfiguration = TestConfiguration(getClass, mainArguments = List(s"-tcp-port=$tcpPort"))
  implicit private lazy val actorSystem = instance[ActorSystem]

  "Webservice /job_description" - {
    "Accept: text/html" in {
      val responseString = getJobDescription(`text/html`)
      assert(responseString contains "<html>")
      assert(responseString contains "DESCRIPTION")
    }

    "Accept: text/plain" in {
      val responseString = getJobDescription(`text/plain`)
      assert(!(responseString contains "<html>"))
      assertResult("DESCRIPTION") { responseString }
    }
  }

  private def getJobDescription(mediaType: MediaType): String = {
    val pipeline: HttpRequest ⇒ Future[HttpResponse] = addHeader(Accept(mediaType)) ~> (sendReceive ~> unmarshal[HttpResponse])
    val future = pipeline(Get(s"http://127.0.0.1:$tcpPort/job_description?job=/test-a"))
    val response = awaitResult(future, 5.s)
    assertResult(Some(`Content-Type`(mediaType))) { response.header[`Content-Type`] }
    response.entity.asString
  }
}
