package com.sos.scheduler.engine.test

import EnvironmentFiles._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.kernel.util.ResourcePath
import java.io.File
import java.net.URL
import org.joda.time.Instant.now
import org.springframework.core.io.Resource
import org.springframework.core.io.support.PathMatchingResourcePatternResolver

final class EnvironmentFiles(
    configResourcePath: ResourcePath,
    directory: File,
    nameMap: Map[String, String],
    fileTransformer: ResourceToFileTransformer) {

  private val resolver = new PathMatchingResourcePatternResolver
  private val lastModified = now() - 3.s

  private def copy() {
    for ((name, url) <- resourceUrls) copyResource(url, name)
  }

  private def resourceUrls: Iterable[(String, URL)] =
    (resourceUrls(defaultConfigResourcePath) ++ resourceUrls(configResourcePath)) map { o => nameOfUrl(o) -> o }

  private def resourceUrls(p: ResourcePath): Iterable[URL] =
    resources(p) map { _.getURL }

  private def resources(p: ResourcePath): Iterable[Resource] =
    resources(p, "*.xml") ++ resources(p, "*.ini")

  private def resources(p: ResourcePath, namePattern: String): Iterable[Resource] =
    resolver.getResources(s"classpath*:${p.path}/$namePattern")

  private def copyResource(url: URL, name: String) {
    val f = new File(directory, name)
    fileTransformer.transform(url, f)
    f.setLastModified(lastModified.getMillis)
  }

  private def nameOfUrl(u: URL): String = {
    val name = new File(u.getPath).getName
    nameMap.getOrElse(name, name)
  }
}

object EnvironmentFiles {
  def copy(
      configResourcePath: ResourcePath,
      directory: File,
      nameMap: Map[String, String] = Map(),
      fileTransformer: ResourceToFileTransformer = StandardResourceToFileTransformer.singleton) {
    new EnvironmentFiles(configResourcePath, directory, nameMap, fileTransformer).copy()
  }

  private val defaultConfigResourcePath: ResourcePath = new ResourcePath(classOf[EnvironmentFiles].getPackage, "config")
}

