package com.sos.scheduler.engine.http.server.heartbeat

import akka.actor.{ActorRef, ActorSystem, Props}
import akka.io.{IO, Tcp}
import akka.pattern.ask
import akka.util.Timeout
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.timer.TimerService
import com.sos.scheduler.engine.common.utils.Exceptions.repeatUntilNoException
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.http.client.heartbeat.HeartbeatRequestor.HttpRequestTimeoutException
import com.sos.scheduler.engine.http.client.heartbeat.{HeartbeatId, HeartbeatRequestor, HttpHeartbeatTiming}
import com.sos.scheduler.engine.http.server.heartbeat.HeartbeatTest._
import java.time.Duration
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.{BeforeAndAfterAll, FreeSpec}
import scala.collection.{immutable, mutable}
import scala.concurrent._
import scala.concurrent.duration.DurationInt
import spray.can.Http
import spray.client.pipelining._
import spray.http.HttpHeaders.Accept
import spray.http.MediaTypes._
import spray.http.StatusCodes.{BadRequest, OK}
import spray.http._
import spray.httpx.SprayJsonSupport._
import spray.httpx.unmarshalling._
import spray.json.DefaultJsonProtocol._
import spray.routing.{HttpServiceActor, Route}

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class HeartbeatTest extends FreeSpec with BeforeAndAfterAll {

  private implicit val askTimeout = AskTimeout
  private implicit lazy val actorSystem = ActorSystem("TEST")
  private implicit val timerService = new TimerService(1.ms, idleTimeout = Some(10.s))
  private implicit val dataJsonFormat = Data.jsonFormat
  private lazy val (baseUri, webService) = startWebServer()
  private val idempotenceScopes = Iterator from 1

  import actorSystem.dispatcher

  override protected def beforeAll() = {
    super.beforeAll()
    // Warm-up
    autoClosing(new HeartbeatRequestor(HttpHeartbeatTiming(period = 50.ms, timeout = 150.ms), testWithHeartbeatDelay = 1.ms)) { heartbeatRequestor ⇒
      val runId = idempotenceScopes.next()
      val uri = s"$baseUri/test/" + runId
      Await.ready(heartbeatRequestor.apply(addHeader(Accept(`application/json`)) ~> sendReceive, Post(uri, Data(100.ms.toString))), 10.seconds)
    }
  }

  override protected def afterAll() = {
    timerService.close()
    actorSystem.shutdown()
    super.afterAll()
  }

  "Requests with heartbeat" - {
    addHeartbeatTests(HttpHeartbeatTiming(period = 50.ms, timeout = 1000.ms))
  }

  "onHeartbeatTimeout is called when client has timed out" in {
    Await.result(webService ? "GET-TIMEOUTS", AskTimeout.duration)  // Clear data ???
    val timing = HttpHeartbeatTiming(100.ms, 500.ms)
    val duration = 150.ms
    autoClosing(new HeartbeatRequestor(timing, testWithHeartbeatDelay = timing.timeout + 1.s)) { heartbeatRequestor ⇒
      val runId = idempotenceScopes.next()
      val uri = s"$baseUri/test/" + runId
      val request = Data(duration.toString)
      val responseFuture = heartbeatRequestor.apply(sendReceive, Post(uri, request))
      val response = awaitResult(responseFuture, 10.s)
      assert(response.status == BadRequest)
      assert(response.entity.asString startsWith "Unknown or expired HeartbeatId")
      assert(heartbeatRequestor.serverHeartbeatCount == 1)
      assert(heartbeatRequestor.clientHeartbeatCount == 0)
      val timedOutRequests = Await.result((webService ? "GET-TIMEOUTS").mapTo[immutable.Seq[String]], AskTimeout.duration)
      assert(timedOutRequests == List(request))
      assertServerIsClean(runId)
    }
  }

  "Client-side heartbeat" in {
    val timing = HttpHeartbeatTiming(period = 100.ms, timeout = 1000.ms)
    val duration = timing.period + timing.period / 2
    val heartbeatRequestor = new HeartbeatRequestor(timing)
    val runId = idempotenceScopes.next()
    val uri = s"$baseUri/test/" + runId
    val request = Data(duration.toString)
    val responseFuture = heartbeatRequestor.apply(addHeader(Accept(`application/json`)) ~> sendReceive, Post(uri, request))
    val response = awaitResult(responseFuture, 10.s)
    assert((response.status, response.as[Data]) == (OK, Right(request.toResponse)))
    assert(heartbeatRequestor.serverHeartbeatCount == 1)
    assert(heartbeatRequestor.clientHeartbeatCount == 0)
    sleep(duration max HeartbeatRequestor.ClientHeartbeatMinimumDelay * 1.5 )
    assert(heartbeatRequestor.clientHeartbeatCount == 1)
    sleep(duration max HeartbeatRequestor.ClientHeartbeatMinimumDelay * 1.5 )
    assert(Set(2, 3) contains heartbeatRequestor.clientHeartbeatCount)
    heartbeatRequestor.close()  // Stop sending client-side heartbeats
    sleep(3 * duration)
    assert(Set(2, 3) contains heartbeatRequestor.clientHeartbeatCount)
    assertServerIsClean(runId)
  }

  "HttpRequestTimeoutException" in {
    val times = HttpHeartbeatTiming(period = 300.ms, timeout = 1000.ms)
    val debug = new HeartbeatRequestor.Debug
    debug.clientTimeout = Some(100.ms)
    autoClosing(new HeartbeatRequestor(times, debug = debug)) { heartbeatRequestor ⇒
      val runId = idempotenceScopes.next()
      val uri = s"$baseUri/test/" + runId
      val request = Data(200.ms.toString)
      val responseFuture = heartbeatRequestor.apply(addHeader(Accept(`application/json`)) ~> sendReceive, Post(uri, request))
      intercept[HttpRequestTimeoutException] { awaitResult(responseFuture, 1.s) }
      assert(heartbeatRequestor.serverHeartbeatCount == 0)
      assert(heartbeatRequestor.clientHeartbeatCount == 0)
      assertServerIsClean(runId)
    }
  }

  if (false) "Endurance" - {
    val times = HttpHeartbeatTiming(period = 4.ms, timeout = 100.ms)
    for (i ← 1 to 100000) s"$i" - {
      addHeartbeatTests(times, highestCurrentOperationsMaximum = 10)
    }
  }

  private def addHeartbeatTests(times: HttpHeartbeatTiming, highestCurrentOperationsMaximum: Int = 2) {
    for ((duration, delay, heartbeatCountPredicate) ← List[(Duration, Duration, Int ⇒ Unit)](
      (0.s, 0.s, o ⇒ assert(o == 0)),
      (times.period + times.period * 3/4, 2 * times.period, o ⇒ assert(o == 1)),
      (50 * times.period, 0.s, o ⇒ assert(o >= 5)),
      (50 * times.period, randomDuration(2 * times.period), o ⇒ assert(o >= 3))))
    s"operation = ${duration.pretty}, own delay = ${delay.pretty}" in {
      autoClosing(new HeartbeatRequestor(times, testWithHeartbeatDelay = delay)) { heartbeatRequestor ⇒
        val runId = idempotenceScopes.next()
        val uri = s"$baseUri/test/" + runId
        val request = Data(duration.toString)
        val responseFuture = heartbeatRequestor.apply(addHeader(Accept(`application/json`)) ~> sendReceive, Post(uri, request))
        val response = awaitResult(responseFuture, 10.s)
        withClue(response.entity.asString) {
          assert(response.status == OK)
        }
        assert(response.as[Data] == Right(request.toResponse))
        heartbeatCountPredicate(heartbeatRequestor.serverHeartbeatCount)
        assert(heartbeatRequestor.clientHeartbeatCount == 0)
        assertServerIsClean(runId, highestCurrentOperationsMaximum)
      }
    }
  }

  private def assertServerIsClean(runId: Int, highestCurrentOperationsMaximum: Int = 2): Unit = {
    repeatUntilNoException(1000.ms, 10.ms) {
      val (currentOperationsMaximum, currentHeartbeatIds) =
        Await.result((webService ? ("GET" → s"$runId")).mapTo[(Int, Set[HeartbeatId])], AskTimeout.duration)
      assert (currentOperationsMaximum <= highestCurrentOperationsMaximum && currentHeartbeatIds.isEmpty)
    }
  }
}

object HeartbeatTest {
  private implicit val AskTimeout = Timeout(10.seconds)

  private case class Data(string: String) {
    def toResponse = Data(s"$string RESPONSE")
  }
  private object Data {
    implicit val jsonFormat = jsonFormat1(apply)
  }


  private def startWebServer()(implicit actorSystem: ActorSystem, timerService: TimerService): (Uri, ActorRef) = {
    val port = findRandomFreeTcpPort()
    val webService = actorSystem.actorOf(Props { new WebActor })
    Await.result(IO(Http) ? Http.Bind(webService, interface = "127.0.0.1", port = port), AskTimeout.duration) match {
      case _: Http.Bound ⇒
      case o: Tcp.CommandFailed ⇒ throw new RuntimeException(o.toString)
    }
    (Uri(s"http://127.0.0.1:$port"), webService)
  }

  private class WebActor(implicit timerService: TimerService) extends HttpServiceActor {
    private val logger = Logger(getClass)
    private implicit val executionContext: ExecutionContext = context.system.dispatcher
    private val timedOutRequests = mutable.Buffer[Data]()
    private val idToHeartbeatService = mutable.Map[String, HeartbeatService]()

    def receive = myReceive orElse runRoute(route)

    private def myReceive: Receive = {
      case ("GET", id: String) ⇒ sender() ! (idToHeartbeatService(id).pendingOperationsMaximum, idToHeartbeatService(id).pendingHeartbeatIds)
      case "GET-TIMEOUTS" ⇒ sender() ! {
        val r = timedOutRequests.toVector
        timedOutRequests.clear()
        r
      }
    }

    private def route: Route =
      (pathPrefix("test") & path(Segment)) { id: String ⇒
        post {
          val heartbeatService = idToHeartbeatService.getOrElseUpdate(id, new HeartbeatService)
          heartbeatService.continueHeartbeat(timeout ⇒ logger.info(s"Client-side heartbeat ${timeout.pretty}")) ~
            entity(as[Data]) { data ⇒
            heartbeatService.startHeartbeat(
              onHeartbeatTimeout = Some(onHeartbeatTimeout(data))) {
                timeout ⇒ operation(timeout, data)
              }
          }
        }
      }

    private def operation(timeout: Option[Duration], data: Data) = Future {
      logger.info(s"operation timeout=$timeout $data")
      sleep(Duration parse data.string)
      data.toResponse
    }

    private def onHeartbeatTimeout(data: Data)(timeout: HeartbeatTimeout): Unit = {
      logger.info(s"$timeout")
      timedOutRequests += data
    }
  }
}
