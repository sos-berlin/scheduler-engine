package com.sos.scheduler.engine.agent.client.main

import com.sos.scheduler.engine.agent.client.main.AgentClientMainIT._
import com.sos.scheduler.engine.agent.commandexecutor.CommandExecutor
import com.sos.scheduler.engine.agent.data.commands.{Command, Terminate}
import com.sos.scheduler.engine.agent.data.responses.EmptyResponse
import com.sos.scheduler.engine.agent.test.AgentTest
import com.sos.scheduler.engine.common.guice.ScalaAbstractModule
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.{BeforeAndAfterAll, FreeSpec}
import scala.collection.mutable
import scala.concurrent.Future

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class AgentClientMainIT extends FreeSpec with BeforeAndAfterAll with HasCloser with AgentTest {

  override def afterAll(): Unit = {
    onClose { super.afterAll() }
    close()
  }

  override protected def extraAgentModule = new ScalaAbstractModule {
    def configure() = {
      bindInstance[CommandExecutor](new CommandExecutor {
        def executeCommand(command: Command): Future[command.Response] = {
          val response = command match {
            case ExpectedTerminate ⇒ EmptyResponse
          }
          Future.successful(response.asInstanceOf[command.Response])
        }
      })
    }
  }

  "main" in {
    val output = mutable.Buffer[String]()
    val commandYaml = """{ $TYPE: Terminate, sigtermProcesses: true, sigkillProcessesAfter: 10 }"""
    AgentClientMain.run(List(agent.localUri, commandYaml, "/overview"), o ⇒ output += o)
    assert(output.size == 3)
    assert(output(0) == "{}")
    assert(output(1) == "---")
    assert(output(2) contains "startedAt: '2")
    assert(output(2) contains "isTerminating: false")
    assert(output(2) contains "totalProcessCount: 0")
    assert(output(2) contains "currentProcessCount: 0")
  }

  "main with Agent URI only checks wether Agent is responding" in {
    val output = mutable.Buffer[String]()
    AgentClientMain.run(List(agent.localUri), o ⇒ output += o)
    assert(output == List("JobScheduler Agent is responding"))
    intercept[Exception] { AgentClientMain.run(List(s"http://127.0.0.1:${findRandomFreeTcpPort()}"), _ ⇒ ()) }
  }
}

private object AgentClientMainIT {
  private val ExpectedTerminate = Terminate(sigtermProcesses = true, sigkillProcessesAfter = Some(10.s))
}
