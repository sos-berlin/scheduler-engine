package com.sos.scheduler.engine.http.server.heartbeat

import akka.actor.{ActorRef, ActorSystem, Props}
import akka.io.{IO, Tcp}
import akka.pattern.ask
import akka.util.Timeout
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
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
  private lazy val heartbeatService = new HeartbeatService
  private implicit lazy val actorSystem = ActorSystem("TEST")
  private implicit val dataJsonFormat = Data.jsonFormat
  import actorSystem.dispatcher
  private lazy val (baseUri, webService) = startWebServer(heartbeatService)

  override protected def afterAll() = {
    actorSystem.shutdown()
    super.afterAll()
  }

  "Requests with heartbeat" - {
    addHeartbeatTests(HttpHeartbeatTiming(period = 20.ms, timeout = 100.ms))
  }

  "onHeartbeatTimeout is called when client has timed out" in {
    val timing = HttpHeartbeatTiming(100.ms, 200.ms)
    val duration = timing.period + timing.period / 2
    val heartbeatRequestor = new HeartbeatRequestor(timing, delayHeartbeat = timing.timeout + 1.s)
    val request = Data(duration.toString)
    val responseFuture = heartbeatRequestor.apply(sendReceive, Post(s"$baseUri/test", request))
    val response = awaitResult(responseFuture, 10.s)
    assert(response.status == BadRequest)
    assert(response.entity.asString == "Unknown heartbeat ID (HTTP request is too late?)")
    assert(heartbeatRequestor.heartbeatCount == 1)
    assertServerIsClean()
    val timedOutRequests = Await.result((webService ? "GET-TIMEOUTS").mapTo[immutable.Seq[String]], AskTimeout.duration)
    assert(timedOutRequests == List(request))
  }

  "HttpRequestTimeoutException" in {
    val times = HttpHeartbeatTiming(period = 300.ms, timeout = 300.ms)
    val heartbeatRequestor = new HeartbeatRequestor(times)
    val request = Data(200.ms.toString)
    val responseFuture = heartbeatRequestor.apply(addHeader(Accept(`application/json`)) ~> sendReceive(actorSystem, actorSystem.dispatcher, 100.ms.toFiniteDuration), Post(s"$baseUri/test", request))
    intercept[HttpRequestTimeoutException] { awaitResult(responseFuture, 1.s) }
    assert(heartbeatRequestor.heartbeatCount == 0)
    assertServerIsClean()
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
      val heartbeatRequestor = new HeartbeatRequestor(times, delayHeartbeat = delay)
      val request = Data(duration.toString)
      val responseFuture = heartbeatRequestor.apply(addHeader(Accept(`application/json`)) ~> sendReceive, Post(s"$baseUri/test", request))
      val response = awaitResult(responseFuture, 10.s)
      assert((response.status, response.as[Data].right.get) == (OK, request.toResponse))
      heartbeatCountPredicate(heartbeatRequestor.heartbeatCount)
      assertServerIsClean(highestCurrentOperationsMaximum)
    }
  }

  private def assertServerIsClean(highestCurrentOperationsMaximum: Int = 2): Unit = {
    val timeout = 1.s
    repeatUntilNoException(1000.ms, 10.ms) {
      val (currentOperationsMaximum, currentHeartbeatIds) = awaitResult(webService.ask("GET")(timeout.toFiniteDuration).mapTo[(Int, Set[HeartbeatId])], timeout)
      assert (currentOperationsMaximum <= highestCurrentOperationsMaximum && currentHeartbeatIds.isEmpty)
    }
  }
}

object HeartbeatTest {
  private implicit val AskTimeout = Timeout(2.seconds)

  private case class Data(string: String) {
    def toResponse = Data(s"$string RESPONSE")
  }
  private object Data {
    implicit val jsonFormat = jsonFormat1(apply)
  }


  private def startWebServer(heartbeatService: HeartbeatService)(implicit actorSystem: ActorSystem): (Uri, ActorRef) = {
    val port = findRandomFreeTcpPort()
    val webService = actorSystem.actorOf(Props { new WebActor(heartbeatService) })
    Await.result(IO(Http) ? Http.Bind(webService, interface = "127.0.0.1", port = port), AskTimeout.duration) match {
      case _: Http.Bound ⇒
      case o: Tcp.CommandFailed ⇒ throw new RuntimeException(o.toString)
    }
    (Uri(s"http://127.0.0.1:$port"), webService)
  }

  final class WebActor(heartbeatService: HeartbeatService) extends HttpServiceActor {
    private val logger = Logger(getClass)
    private implicit val executionContext: ExecutionContext = context.system.dispatcher
    private val timedOutRequests = mutable.Buffer[Data]()

    def receive = myReceive orElse runRoute(route)

    private def myReceive: Receive = {
      case "GET" ⇒ sender() ! (heartbeatService.pendingOperationsMaximum, heartbeatService.pendingHeartbeatIds)
      case "GET-TIMEOUTS" ⇒ sender() ! {
        val r = timedOutRequests.toVector
        timedOutRequests.clear()
        r
      }
    }

    private def route: Route =
      path("test") {
        post {
          heartbeatService.continueHeartbeat ~
          entity(as[Data]) { data ⇒
            val resultFuture = operation(data)
            heartbeatService.startHeartbeat(
              resultFuture,
              onHeartbeatTimeout = {
                case t: HeartbeatTimeout ⇒
                  logger.warn(s"No client heartbeat: $t")
                  timedOutRequests += data
              })
          }
        }
      }

    private def operation(data: Data) = Future {
      sleep(Duration parse data.string)
      data.toResponse
    }
  }
}
