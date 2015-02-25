package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.JavaResource
import com.sos.scheduler.engine.test.TestEnvironmentFiles._
import java.io.File
import java.net.URL
import org.joda.time.Instant.now
import org.springframework.core.io.Resource
import org.springframework.core.io.support.PathMatchingResourcePatternResolver

private class TestEnvironmentFiles(
    configPackageResource: JavaResource,
    directory: File,
    renameFile: PartialFunction[String, String],
    fileTransformer: ResourceToFileTransformer) {

  private val resolver = new PathMatchingResourcePatternResolver
  private val lastModified = now() - 3.s

  private def copy(): Unit = for ((url, name) ← resourceUrls) copyResource(url, name)

  private def resourceUrls: Iterable[(URL, String)] =
    resourceUrls(DefaultConfigResource) ++ resourceUrls(configPackageResource) map { o ⇒ o → urlToDestinationName(o) }

  private def resourceUrls(resource: JavaResource): Iterable[URL] =
    resources(resource) map { _.getURL }

  private def resources(resource: JavaResource): Iterable[Resource] =
    ResourcePatterns flatMap { resources(resource, _) }

  private def resources(resource: JavaResource, namePattern: String): Iterable[Resource] =
    resolver.getResources(s"classpath*:${resource.path}/$namePattern")

  private def copyResource(url: URL, name: String): Unit = {
    val f = directory / name
    fileTransformer.transform(url, f)
    f.setLastModified(lastModified.getMillis)
  }

  private def urlToDestinationName(u: URL): String = {
    val name = new File(u.getPath).getName
    renameFile.applyOrElse(name, identity[String])
  }
}

object TestEnvironmentFiles {
  private val ResourcePatterns = List("*.xml", "*.ini", "*.dtd")
  private val DefaultConfigResource = JavaResource("com/sos/scheduler/engine/test/config")

  def copy(
      configJavaResource: JavaResource,
      directory: File,
      renameFile: PartialFunction[String, String] = PartialFunction.empty,
      fileTransformer: ResourceToFileTransformer = StandardResourceToFileTransformer.singleton): Unit = {
    new TestEnvironmentFiles(configJavaResource, directory, renameFile, fileTransformer).copy()
  }
}
