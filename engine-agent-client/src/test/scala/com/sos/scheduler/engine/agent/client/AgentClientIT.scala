package com.sos.scheduler.engine.agent.client

import akka.actor.ActorSystem
import akka.util.Timeout
import com.google.common.io.Closer
import com.google.common.io.Files._
import com.google.inject.Guice
import com.sos.scheduler.engine.agent.Agent
import com.sos.scheduler.engine.agent.client.AgentClient.{RequestTimeout, commandDurationToRequestTimeout}
import com.sos.scheduler.engine.agent.data.commands.{DeleteFile, MoveFile, RequestFileOrderSourceContent}
import com.sos.scheduler.engine.agent.data.responses.{EmptyResponse, FileOrderSourceContent}
import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.common.guice.ScalaAbstractModule
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.touchAndDeleteWithCloser
import com.sos.scheduler.engine.common.scalautil.Futures.awaitResult
import com.sos.scheduler.engine.common.time.ScalaTime._
import java.nio.file.Files
import java.nio.file.Files._
import java.nio.file.attribute.FileTime
import java.time.{Duration, Instant}
import java.util.concurrent.TimeUnit.MILLISECONDS
import org.junit.runner.RunWith
import org.scalatest.concurrent.ScalaFutures
import org.scalatest.junit.JUnitRunner
import org.scalatest.{BeforeAndAfterAll, FreeSpec}
import scala.concurrent.Await
import scala.concurrent.duration._
import scala.util.matching.Regex

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class AgentClientIT extends FreeSpec with ScalaFutures with BeforeAndAfterAll {

  override implicit val patienceConfig = PatienceConfig(timeout = 10.s.toConcurrent)
  private implicit val closer = Closer.create()
  private lazy val agent = Agent.forTest().closeWithCloser
  private val injector = Guice.createInjector(new ScalaAbstractModule {
    def configure() = bindInstance[ActorSystem] { ActorSystem() }
  })
  private lazy val client = injector.instance[AgentClientFactory].apply(agentUri = agent.localUri)

  override def beforeAll() = awaitResult(agent.start(), 10.s)

  override def afterAll() = {
    closer.close()
    super.afterAll()
  }

  "commandMillisToRequestTimeout" in {
    val upperBound = RequestFileOrderSourceContent.MaxDuration  // The upper bound depends on Akka tick length (Int.MaxValue ticks, a tick can be as short as 1ms)
    for (duration ← List[Duration](0.s, 1.s, upperBound)) {
      assert(commandDurationToRequestTimeout(duration) == Timeout((RequestTimeout + duration).toMillis, MILLISECONDS))
    }
  }

  "Commands" - {
    "RequestFileOrderSourceContent" in {
      val dir = createTempDirectory("agent-") withCloser delete
      val knownFile = dir / "x-known"
      val instant = Instant.parse("2015-01-01T12:00:00Z")
      val expectedFiles = List(
        (dir / "x-1", instant),
        (dir / "prefix-x-3", instant + 2.s),
        (dir / "x-2", instant + 4.s))
      val expectedResult = FileOrderSourceContent(expectedFiles map { case (file, t) ⇒ FileOrderSourceContent.Entry(file.toString, t.toEpochMilli) })
      val ignoredFiles = List(
        (knownFile, instant),
        (dir / "ignore-4", instant))
      for ((file, t) ← expectedFiles ++ ignoredFiles) {
        touchAndDeleteWithCloser(file)
        setLastModifiedTime(file, FileTime.from(t))
      }
      val regex = "x-"
      assert(new Regex(regex).findFirstIn(knownFile.toString).isDefined)
      val command = RequestFileOrderSourceContent(
        directory = dir.toString,
        regex = regex,
        duration = RequestFileOrderSourceContent.MaxDuration,
        knownFiles = Set(knownFile.toString))
      whenReady(client.executeCommand(command)) { o ⇒
        assert(o == expectedResult)
      }
    }

    "DeleteFile" in {
      val file = Files.createTempFile("TEST-", ".tmp")
      assert(Files.exists(file))
      whenReady(client.executeCommand(DeleteFile(file.toString))) { case EmptyResponse ⇒
        assert(!Files.exists(file))
      }
    }

    "MoveFile" in {
      val file = Files.createTempFile("TEST-", ".tmp")
      val dir = Files.createTempDirectory("TEST-")
      assert(Files.exists(file))
      val movedPath = dir resolve file.getFileName
      whenReady(client.executeCommand(MoveFile(file.toString, dir.toString))) { case EmptyResponse ⇒
        assert(!Files.exists(file))
        assert(Files.exists(movedPath))
      }
      Files.delete(movedPath)
      Files.delete(dir)
    }

    "MoveFile to file fails" in {
      val file = Files.createTempFile("TEST-", ".tmp")
      val destination = file.getParent resolve s"NEW-${file.getFileName}"
      assert(Files.exists(file))
      assert(Await.ready(client.executeCommand(MoveFile(file.toString, destination.toString)), 10.seconds).value.get.failed.get.getMessage
        contains "directory")
    }
  }

  "fileExists" in {
    val file = createTempFile("AgentClientIT with blank", ".tmp")
    closer.onClose { deleteIfExists(file) }
    for (i ← 1 to 3) {   // Check no-cache
      touch(file)
      whenReady(client.fileExists(file.toString)) { exists ⇒ assert(exists) }
      delete(file)
      whenReady(client.fileExists(file.toString)) { exists ⇒ assert(!exists) }
    }
  }
}
