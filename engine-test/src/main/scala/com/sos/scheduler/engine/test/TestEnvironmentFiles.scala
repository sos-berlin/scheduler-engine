package com.sos.scheduler.engine.test

import TestEnvironmentFiles._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.JavaResource
import java.io.File
import java.net.URL
import org.joda.time.Instant.now
import org.springframework.core.io.Resource
import org.springframework.core.io.support.PathMatchingResourcePatternResolver

private class TestEnvironmentFiles(
    configJavaResource: JavaResource,
    directory: File,
    nameMap: Map[String, String],
    fileTransformer: ResourceToFileTransformer) {

  private val resolver = new PathMatchingResourcePatternResolver
  private val lastModified = now() - 3.s

  private def copy(): Unit = for ((name, url) ← resourceUrls) copyResource(url, name)

  private def resourceUrls: Iterable[(String, URL)] =
    resourceUrls(DefaultConfigResource) ++ resourceUrls(configJavaResource) map { o ⇒ nameOfUrl(o) → o }

  private def resourceUrls(resource: JavaResource): Iterable[URL] =
    resources(resource) map { _.getURL }

  private def resources(resource: JavaResource): Iterable[Resource] =
    ResourcePatterns flatMap { resources(resource, _) }

  private def resources(resource: JavaResource, namePattern: String): Iterable[Resource] =
    resolver.getResources(s"classpath*:${resource.path}/$namePattern")

  private def copyResource(url: URL, name: String): Unit = {
    val f = new File(directory, name)
    fileTransformer.transform(url, f)
    f.setLastModified(lastModified.getMillis)
  }

  private def nameOfUrl(u: URL): String = {
    val name = new File(u.getPath).getName
    nameMap.getOrElse(name, name)
  }
}

object TestEnvironmentFiles {
  private val ResourcePatterns = List("*.xml", "*.ini", "*.dtd")
  private val DefaultConfigResource = JavaResource("com/sos/scheduler/engine/test/config")

  def copy(
      configJavaResource: JavaResource,
      directory: File,
      nameMap: Map[String, String] = Map(),
      fileTransformer: ResourceToFileTransformer = StandardResourceToFileTransformer.singleton): Unit = {
    new TestEnvironmentFiles(configJavaResource, directory, nameMap, fileTransformer).copy()
  }
}
