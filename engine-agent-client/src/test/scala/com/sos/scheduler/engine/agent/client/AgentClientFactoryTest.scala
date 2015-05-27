package com.sos.scheduler.engine.agent.client

import akka.actor.ActorSystem
import akka.util.Timeout
import com.google.common.io.Closer
import com.google.common.io.Files._
import com.google.inject.Guice
import com.sos.scheduler.engine.agent.Agent
import com.sos.scheduler.engine.agent.client.AgentClientFactory._
import com.sos.scheduler.engine.agent.data.commands.{FileOrderSourceContent, RequestFileOrderSourceContent}
import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.common.guice.ScalaAbstractModule
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import java.nio.file.Files._
import java.nio.file.Paths
import java.nio.file.attribute.FileTime
import java.util.concurrent.TimeUnit.MILLISECONDS
import org.junit.runner.RunWith
import org.scalatest.concurrent.ScalaFutures
import org.scalatest.junit.JUnitRunner
import org.scalatest.{BeforeAndAfterAll, FreeSpec}
import scala.concurrent.Await
import scala.concurrent.duration._

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class AgentClientFactoryTest extends FreeSpec with ScalaFutures with BeforeAndAfterAll {

  override implicit val patienceConfig = PatienceConfig(timeout = 10.seconds)
  private implicit val closer = Closer.create()
  private lazy val agent = Agent.forTest().closeWithCloser
  private val injector = Guice.createInjector(new ScalaAbstractModule {
    def configure() = bindInstance[ActorSystem] { ActorSystem() }
  })
  private lazy val client = injector.instance[AgentClientFactory].apply(agentUri = agent.localUri)

  override def beforeAll() = Await.result(agent.start(), 10.seconds)

  override def afterAll() = {
    closer.close()
    super.afterAll()
  }

  "commandMillisToRequestTimeout" in {
    val upperBound = RequestFileOrderSourceContent.MaxDuration  // The upper bound depends on Akka tick length (Int.MaxValue ticks, a tick can be as short as 1ms)
    for (millis ← List[Long](0, 1, upperBound.toMillis)) {
      Logger(getClass).debug(s"millis=$millis")
      assert(commandMillisToRequestTimeout(millis) == Timeout(RequestTimeout.toMillis + millis, MILLISECONDS))
    }
  }

  "readFiles" in {
      val dir = createTempDirectory("agent-")
      val aTime = 1000L * 1000 * 1000 * 1000
      val xTime = aTime + 2000
      val cTime = aTime + 4000
      closer.onClose { delete(dir) }
      val expectedResult = FileOrderSourceContent(List(
        FileOrderSourceContent.Entry((dir / "a").toString, aTime),
        FileOrderSourceContent.Entry((dir / "x").toString, xTime),
        FileOrderSourceContent.Entry((dir / "c").toString, cTime)))
      for (entry ← expectedResult.files) {
        val path = Paths.get(entry.path)
        touch(path)
        setLastModifiedTime(path, FileTime.fromMillis(entry.lastModifiedTime))
        closer.onClose { delete(path) }
      }
      val command = RequestFileOrderSourceContent(
        directory = dir.toString,
        regex = "",
        durationMillis = RequestFileOrderSourceContent.MaxDuration.toMillis,
        knownFiles = Set())   // TODO regex und knownFiles
      whenReady(client.executeCommand(command)) { o ⇒
        assert(o == expectedResult)
      }
  }
  }
}
