package com.sos.scheduler.engine.agent.client

import akka.actor.ActorSystem
import com.google.common.io.Files._
import com.google.inject.Guice
import com.sos.scheduler.engine.agent.Agent
import com.sos.scheduler.engine.agent.data.commands.{FileOrderSourceContent, RequestFileOrderSourceContent}
import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.common.guice.ScalaAbstractModule
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.Closers.withCloser
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import java.nio.file.Files._
import java.nio.file.attribute.FileTime
import java.nio.file.{Files, Paths}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.concurrent.ScalaFutures
import org.scalatest.junit.JUnitRunner
import scala.concurrent.duration._

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class FileOrderSourceClientTest extends FreeSpec with ScalaFutures {

  override implicit val patienceConfig = PatienceConfig(timeout = 10.seconds)

  "readFiles" in {
    withCloser { implicit closer ⇒
      val injector = Guice.createInjector(new ScalaAbstractModule {
        def configure() = bindInstance[ActorSystem] { ActorSystem() }
      })
      val client = injector.instance[FileOrderSourceClient]
      val dir = createTempDirectory("agent-")
      val aTime = 1000L * 1000 * 1000 * 1000
      val xTime = aTime + 2000
      val cTime = aTime + 4000
      closer.onClose { Files.delete(dir) }
      val expectedResult = FileOrderSourceContent(List(
        FileOrderSourceContent.Entry((dir / "a").toString, aTime),
        FileOrderSourceContent.Entry((dir / "x").toString, xTime),
        FileOrderSourceContent.Entry((dir / "c").toString, cTime)))
      for (entry ← expectedResult.files) {
        val path = Paths.get(entry.path)
        touch(path)
        setLastModifiedTime(path, FileTime.fromMillis(entry.lastModifiedTime))
        closer.onClose { Files.delete(path) }
      }
      autoClosing(Agent.forTest()) { agent ⇒
        whenReady(agent.start()) { _ ⇒
          val command = RequestFileOrderSourceContent(directory = dir.toString, regex = "", durationMillis = Long.MaxValue, knownFiles = Set())   // TODO regex und knownFiles
          whenReady(client.execute(agentUri = agent.localUri, command)) { o ⇒
            assert(o == expectedResult)
          }
        }
      }
    }
  }
}
