package com.sos.scheduler.engine.tests.jira.js1858

import akka.util.ByteString
import com.google.common.io.Files.touch
import com.sos.scheduler.engine.client.web.{SchedulerUris, StandardWebSchedulerClient}
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.sprayutils.JsObjectMarshallers._
import com.sos.scheduler.engine.common.system.Files.removeDirectoryContentRecursivly
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.plugins.newwebservice.routes.LiveRoute.test.{allowedPaths, forbiddenPaths}
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import java.nio.file.Files.{exists, isDirectory}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.util.Random
import spray.http.ContentTypes.`application/octet-stream`
import spray.http.HttpHeaders.`Content-Type`
import spray.http.StatusCodes.{BadRequest, Created, Forbidden, NotFound, OK}
import spray.http.{ContentTypeRange, HttpEntity, MediaRange, MediaType, MediaTypes}
import spray.json.{JsArray, JsString}

@RunWith(classOf[JUnitRunner])
final class JS1858IT extends FreeSpec with ScalaSchedulerTest
{
  private lazy val httpPort = findRandomFreeTcpPort()
  override protected lazy val testConfiguration = TestConfiguration(getClass, mainArguments = List(s"-http-port=$httpPort"))
  private lazy val client = new StandardWebSchedulerClient(s"http://127.0.0.1:$httpPort").closeWithCloser
  private lazy val live = controller.environment.liveDirectory

  "GET /api/live returns BadRequest" in {
    val response = client.getHttpResponse(uri("/master/api/live"), ContentTypeRange.`*`.mediaRange) await TestTimeout
    assert(response.status == BadRequest)
  }

  "GET /api/live/MISSING returns NotFound" in {
    val response = client.getHttpResponse(uri("/master/api/live/MISSING"), ContentTypeRange.`*`.mediaRange) await TestTimeout
    assert(response.status == NotFound)
  }

  "GET /api/live/MISSING/ returns NotFound" in {
    val response = client.getHttpResponse(uri("/master/api/live/MISSING/"), ContentTypeRange.`*`.mediaRange) await TestTimeout
    assert(response.status == NotFound)
  }

  "GET /api/live/test.job.xml returns content" in {
    val response = client.getHttpResponse(uri("/master/api/live/test.job.xml"), `application/octet-stream`.mediaType) await TestTimeout
    assert(response.status == OK)
    assert(response.entity.data.toByteString == ByteString((controller.environment.liveDirectory / "test.job.xml").contentBytes))
    assert(response.header[`Content-Type`] == Some(`Content-Type`(`application/octet-stream`)))
  }

  "PUT /api/live/test.job.xml overwrites existing file" in {
    val msg = "LIVE TEST OVERWRITTEN"
    val job = <job><script language="shell">echo {msg}</script></job>
    val response = client.putRaw(uri("/master/api/live/test.job.xml"), job.toString) await TestTimeout
    assert(response.status == OK)

    assert(runJob(JobPath("/test")).logString contains msg)
  }

  "PUT /api/live/a/b/added.job.xml add directories and a file " in {
    val file = "a/b/added.job.xml"
    assert(!exists(live / file))
    val msg = "LIVE TEST ADDED"
    val job = <job><script language="shell">echo {msg}</script></job>
    val response = client.putRaw(uri("/master/api/live/a/b/added.job.xml"), job.toString) await TestTimeout
    assert(response.status == Created)
    assert(exists(live / file))

    assert(runJob(JobPath("/a/b/added")).logString contains msg)
  }

  "GET /api/live/ returns directory content" in {
    val jsArray = client.get[JsArray](uri("/master/api/live/")) await TestTimeout
    assert(jsArray.elements.toSet == Set(
      JsString("a/"),
      JsString("test.job.xml")))
  }

  "GET /api/live/a/b/ returns directory content" in {
    val jsArray = client.get[JsArray](uri("/master/api/live/a/b/")) await TestTimeout
    assert(jsArray.elements.toSet == Set(
      JsString("added.job.xml")))
  }

  "DELETE /api/live/a/b/added.job.xml" in {
    val file = "a/b/added.job.xml"
    assert(exists(live / file))
    val response = client.delete(uri(s"/master/api/live/$file")) await TestTimeout
    assert(response.status == OK)
    assert(!exists(live / file))
    val response2 = client.delete(uri(s"/master/api/live/$file")) await TestTimeout
    assert(response2.status == NotFound)
    val jsArray = client.get[JsArray](uri("/master/api/live/a/b/")) await TestTimeout
    assert(jsArray.elements.isEmpty)
  }

  "PUT /api/live/a/c/ creates a directory" in {
    assert(!exists(live / "a/c"))
    val response = client.putRaw(uri("/master/api/live/a/c/"), HttpEntity.Empty) await TestTimeout
    assert(response.status == Created)
    assert(isDirectory(live / "a/c"))
  }

  "DELETE /api/live/a is rejected because it denotes a directory" in {
    assert(isDirectory(live / "a"))
    val response = client.delete(uri(s"/master/api/live/a")) await TestTimeout
    assert(response.status == BadRequest)
    assert(response.entity.asString == "File is a directory")
  }

  "DELETE /api/live/a/ delete directory recursively" in {
    assert(isDirectory(live / "a"))
    val response = client.delete(uri(s"/master/api/live/a/")) await TestTimeout
    assert(response.status == OK)
    assert(!exists(live / "a"))
  }

  "DELETE /api/live/not-a-directory/" in {
    touch(live / "not-a-directory")
    val response = client.delete(uri(s"/master/api/live/not-a-directory/")) await TestTimeout
    assert(response.status == BadRequest)
    assert(response.entity.asString == "Not a directory")
  }

  "Forbidden paths" - {
    addUriTests(forbiddenPaths) { u =>
      assert(client.getHttpResponse(uri(u), ContentTypeRange.`*`.mediaRange).await(TestTimeout).status == Forbidden)
        assert(client.putRaw(uri(u), "x").await(TestTimeout).status == Forbidden)
        assert(client.delete(uri(u)).await(TestTimeout).status == Forbidden)
    }
  }

  "Allowed paths" - {
    addUriTests(allowedPaths) { u =>
      val content = Random.nextString(10)
      assert(client.putRaw(uri(u), content).await(TestTimeout).status == Created)
      val response = client.getHttpResponse(uri(u), ContentTypeRange.`*`.mediaRange).await(TestTimeout)
      assert(response.status == OK)
      assert(response.entity.data.asString == content)
      assert(client.getHttpResponse(uri(u), ContentTypeRange.`*`.mediaRange).await(TestTimeout).status == OK)
      assert(client.delete(uri(u)).await(TestTimeout).status == OK)
      assert(client.getHttpResponse(uri(u), ContentTypeRange.`*`.mediaRange).await(TestTimeout).status == NotFound)
    }
  }

  private def addUriTests(relativePaths: Iterable[String])(test: String => Unit): Unit = {
    for (path <- relativePaths if path.startsWith("/") && !path.endsWith("/")) path in {
      if (!path.contains(".") && !path.contains('\0')) {  // Something (Spray?) interprets ".." and "." in URI path
        removeDirectoryContentRecursivly(live)
        test("/master/api/live" + path)
      }
    }
  }

  private def uri(tail: String)(u: SchedulerUris): String =
    u.schedulerUri + tail
}
