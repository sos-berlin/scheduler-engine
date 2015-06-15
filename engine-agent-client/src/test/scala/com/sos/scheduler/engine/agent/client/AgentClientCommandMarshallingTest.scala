package com.sos.scheduler.engine.agent.client

import akka.util.Timeout
import com.sos.scheduler.engine.agent.client.AgentClient.{RequestTimeout, commandMillisToRequestTimeout}
import com.sos.scheduler.engine.agent.client.AgentClientCommandMarshallingTest._
import com.sos.scheduler.engine.agent.commandexecutor.CommandExecutor
import com.sos.scheduler.engine.agent.data.commands.{AbortImmediately, Command, RequestFileOrderSourceContent, Terminate}
import com.sos.scheduler.engine.agent.data.responses.{EmptyResponse, FileOrderSourceContent, Response}
import com.sos.scheduler.engine.agent.test.AgentTest
import com.sos.scheduler.engine.common.guice.ScalaAbstractModule
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.time.ScalaTime._
import java.util.concurrent.TimeUnit.MILLISECONDS
import org.junit.runner.RunWith
import org.scalatest.concurrent.ScalaFutures
import org.scalatest.junit.JUnitRunner
import org.scalatest.{BeforeAndAfterAll, FreeSpec}
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future
import scala.concurrent.duration._
import spray.json.JsonParser

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class AgentClientCommandMarshallingTest extends FreeSpec with BeforeAndAfterAll with ScalaFutures with HasCloser with AgentTest {

  override def afterAll(): Unit = {
    onClose { super.afterAll() }
    close()
  }

  override protected def extraAgentModule = new ScalaAbstractModule {
    def configure() = {
      bindInstance[CommandExecutor](new CommandExecutor {
        def executeCommand(command: Command): Future[command.Response] =
          Future {
            (command match {
              case ExpectedTerminate ⇒ EmptyResponse
              case xx ⇒ ExpectedFileOrderSourceContent
            })
            .asInstanceOf[command.Response]
          }
      })
    }
  }

  override implicit val patienceConfig = PatienceConfig(timeout = 10.seconds)
  private lazy val client = StandAloneAgentClient(agentUri = agent.localUri).closeWithCloser

  "commandMillisToRequestTimeout" in {
    val upperBound = RequestFileOrderSourceContent.MaxDuration  // The upper bound depends on Akka tick length (Int.MaxValue ticks, a tick can be as short as 1ms)
    for (millis ← List[Long](0, 1, upperBound.toMillis)) {
      assert(commandMillisToRequestTimeout(millis) == Timeout(RequestTimeout.toMillis + millis, MILLISECONDS))
    }
  }

  List[(Command, Response)](
    ExpectedRequestFileOrderSourceContent → ExpectedFileOrderSourceContent,
    ExpectedTerminate → EmptyResponse,
    AbortImmediately → EmptyResponse)
  .foreach { case (command, response) ⇒
    command.getClass.getSimpleName in {
      whenReady(client.executeCommand(command)) { o ⇒
        assert(o == response)
      }
    }
  }

  "syncExecuteJsonCommand" in {
    val command = """{
      "$TYPE":"Terminate",
      "sigtermProcesses": true,
      "sigkillProcessesAfter": "PT10S"
    }"""
    val expectedResponse = "{}"
    val response = client.synchronouslyExecuteJsonCommand(command)
    assert(JsonParser(response) == JsonParser(expectedResponse))
  }
}

private object AgentClientCommandMarshallingTest {
  private val ExpectedRequestFileOrderSourceContent = RequestFileOrderSourceContent(
    directory = "DIRECTORY",
    regex = "REGEX",
    durationMillis = 111222,
    knownFiles = Set("a", "b"))
  private val ExpectedFileOrderSourceContent = FileOrderSourceContent(List(
    FileOrderSourceContent.Entry("a.txt", 23334445555L),
    FileOrderSourceContent.Entry("b.txt", 20000000000L)))
  private val ExpectedTerminate = Terminate(sigtermProcesses = true, sigkillProcessesAfter = 10.s)
}
