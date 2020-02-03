package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import com.google.common.io.Files.touch
import com.sos.scheduler.engine.base.generic.SecretString
import com.sos.scheduler.engine.client.agent.SchedulerAgentClientFactoryTest._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.sprayutils.https.KeystoreReference
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.JavaResource
import com.sos.scheduler.engine.data.scheduler.SchedulerId
import com.typesafe.config.{Config, ConfigFactory}
import java.nio.file.Files.{createTempFile, delete}
import java.nio.file.StandardCopyOption.REPLACE_EXISTING
import org.scalatest.{BeforeAndAfterAll, FreeSpec}
import spray.http.Uri

/**
 * @author Joacim Zschimmer
 */
final class SchedulerAgentClientFactoryTest extends FreeSpec with BeforeAndAfterAll
{
  private lazy val actorSystem = ActorSystem("SchedulerAgentClientFactoryTest")
  private lazy val keystoreFile = {
    val file = createTempFile("SchedulerAgentClientFactoryTest-", ".jks")
    jksResource.copyToFile(file, REPLACE_EXISTING)
    file
  }

  override def afterAll() = {
    actorSystem.terminate() await 99.s
    delete(keystoreFile)
    super.afterAll()
  }

  "test" in {
    val factory = new SchedulerAgentClientFactory(
      actorSystem,
      SchedulerId("SCHEDULER-ID"),
      Some(KeystoreReference(keystoreFile.toUri.toURL, Some(SecretString("jobscheduler")), Some(SecretString("jobscheduler")))),
      Nil,
      ConfigFactory.empty: Config)
    val uri = Uri("https://127.0.0.1:1")
    val agentClient1 = factory.apply(uri)
    val agentClient2 = factory.apply(uri)
    // Reused setup
    assert(agentClient2.hostConnectorSetupOption.get eq agentClient1.hostConnectorSetupOption.get)

    touch(keystoreFile)
    val agentClient3 = factory.apply(uri)
    // Updated setup
    assert(agentClient3.hostConnectorSetupOption.get ne agentClient1.hostConnectorSetupOption.get)
    val agentClient4 = factory.apply(uri)
    // Reused setup
    assert(agentClient4.hostConnectorSetupOption.get eq agentClient3.hostConnectorSetupOption.get)
  }
}

private object SchedulerAgentClientFactoryTest
{
  private val jksResource = JavaResource("com/sos/scheduler/engine/client/agent/test-https.jks")
}
