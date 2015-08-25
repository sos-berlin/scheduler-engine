package com.sos.scheduler.engine.agent.client

import com.sos.scheduler.engine.agent.client.TextAgentClientIT._
import com.sos.scheduler.engine.agent.command.CommandExecutor
import com.sos.scheduler.engine.agent.data.commandresponses.EmptyResponse
import com.sos.scheduler.engine.agent.data.commands.{Command, Terminate}
import com.sos.scheduler.engine.agent.test.AgentTest
import com.sos.scheduler.engine.common.guice.ScalaAbstractModule
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.soslicense.LicenseKey
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
final class TextAgentClientIT extends FreeSpec with BeforeAndAfterAll with HasCloser with AgentTest {

  override def afterAll(): Unit = {
    onClose { super.afterAll() }
    close()
  }

  override protected def extraAgentModule = new ScalaAbstractModule {
    def configure() = {
      bindInstance[CommandExecutor](new CommandExecutor {
        def executeCommand(command: Command, licenseKey: Option[LicenseKey]): Future[command.Response] = {
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
    autoClosing(new TextAgentClient(agentUri = agent.localUri, o ⇒ output += o)) { client ⇒
      client.executeCommand("""{ $TYPE: Terminate, sigtermProcesses: true, sigkillProcessesAfter: 10 }""")
      client.get("")
    }
    assert(output.size == 3)
    assert(output(0) == "{}")
    assert(output(1) == "---")
    assert(output(2) contains "startedAt: '2")
    assert(output(2) contains "isTerminating: false")
    assert(output(2) contains "totalTaskCount: 0")
    assert(output(2) contains "currentTaskCount: 0")
  }

  "requireIsResponding" in {
    val output = mutable.Buffer[String]()
    autoClosing(new TextAgentClient(agentUri = agent.localUri, o ⇒ output += o)) { client ⇒
      client.requireIsResponding()
    }
    assert(output == List("JobScheduler Agent is responding"))
    autoClosing(new TextAgentClient(agentUri = s"http:127.0.0.1:${findRandomFreeTcpPort()}", _ ⇒ Unit)) { client ⇒
      intercept[Exception] { client.requireIsResponding() }
    }
  }
}

private object TextAgentClientIT {
  private val ExpectedTerminate = Terminate(sigtermProcesses = true, sigkillProcessesAfter = Some(10.s))
}
