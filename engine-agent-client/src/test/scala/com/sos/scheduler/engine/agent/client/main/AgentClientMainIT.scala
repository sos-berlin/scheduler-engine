package com.sos.scheduler.engine.agent.client.main

import com.sos.scheduler.engine.agent.client.main.AgentClientMainIT._
import com.sos.scheduler.engine.agent.command.{CommandExecutor, CommandMeta}
import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.agent.configuration.AgentConfiguration.Https
import com.sos.scheduler.engine.agent.data.commandresponses.EmptyResponse
import com.sos.scheduler.engine.agent.data.commands.{Command, Terminate}
import com.sos.scheduler.engine.agent.test.AgentTest
import com.sos.scheduler.engine.base.generic.SecretString
import com.sos.scheduler.engine.common.guice.ScalaAbstractModule
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.sprayutils.https.KeystoreReference
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.common.utils.JavaResource
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

  override protected def agentConfiguration = AgentConfiguration(
    httpInterfaceRestriction = Some("127.0.0.1"),
    https = Some(Https(findRandomFreeTcpPort(), TestKeyStoreRef)))

  override def afterAll() = {
    onClose { super.afterAll() }
    close()
  }

  override protected def extraAgentModule = new ScalaAbstractModule {
    def configure() = {
      bindInstance[CommandExecutor](new CommandExecutor {
        def executeCommand(command: Command, meta: CommandMeta): Future[command.Response] = {
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
    AgentClientMain.run(List(agent.localUri, commandYaml, "/"), o ⇒ output += o, Some(TestKeyStoreRef))
    assert(output.size == 3)
    assert(output(0) == "{}")
    assert(output(1) == "---")
    assert(output(2) contains "startedAt: '2")
    assert(output(2) contains "isTerminating: false")
    assert(output(2) contains "totalTaskCount: 0")
    assert(output(2) contains "currentTaskCount: 0")
  }

  "main with Agent URI only checks wether Agent is responding (it is)" in {
    val output = mutable.Buffer[String]()
    assertResult(0) {
      AgentClientMain.run(List(agent.localUri), o ⇒ output += o, Some(TestKeyStoreRef))
    }
    assert(output == List("JobScheduler Agent is responding"))
  }

  "main with Agent URI only checks wether Agent is responding (it is not)" in {
    val port = findRandomFreeTcpPort()
    val output = mutable.Buffer[String]()
    assertResult(1) {
      AgentClientMain.run(List(s"http://127.0.0.1:${findRandomFreeTcpPort()}"), _ ⇒ (), Some(TestKeyStoreRef))
    }
    assert(output == List(s"JobScheduler Agent is not responding: Connection attempt to 127.0.0.1:$port failed"))
  }
}

private object AgentClientMainIT {
  private val TestKeyStoreRef = KeystoreReference(
    JavaResource("com/sos/scheduler/engine/agent/client/main/test-keystore.jks").url,
    keyPassword = SecretString("test-keystore.p12"),
    storePassword = Some(SecretString("test-keystore.jks")))
  private val ExpectedTerminate = Terminate(sigtermProcesses = true, sigkillProcessesAfter = Some(10.s))
}
