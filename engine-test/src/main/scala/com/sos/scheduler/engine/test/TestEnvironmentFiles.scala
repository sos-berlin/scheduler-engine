package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.JavaResource
import com.sos.scheduler.engine.test.TestEnvironmentFiles._
import com.sos.scheduler.engine.test.util.JavaResourceResolver._
import java.io.File
import java.net.URL
import java.nio.file.Files._
import org.joda.time.Instant.now
import scala.collection.immutable

private class TestEnvironmentFiles(
    configPackageResource: JavaResource,
    directory: File,
    renameFile: PartialFunction[String, String],
    fileTransformer: ResourceToFileTransformer) {

  private val lastModified = now() - 3.s

  private def copy(): Unit = for ((url, relativePath) ← resourceUrls) copyResource(url, relativePath)

  private def resourceUrls: Iterable[(URL, String)] = {
    val defaultResources = resourceUrls(DefaultConfigResource) map { o ⇒ o → new File(o.getPath).getName }
    val testResources = resourceUrls(configPackageResource) map { o ⇒ o → urlToDestinationRelativePath(o) }
    defaultResources ++ testResources
  }

  private def resourceUrls(packageResource: JavaResource): immutable.Seq[URL] = {
    val path = packageResource.path stripSuffix "/"
    resourcePatternToUrls(s"$path/**/*") filter { u ⇒ NameExtensions exists u.getPath.endsWith }
  }

  private def copyResource(url: URL, relativePath: String): Unit = {
    val f = directory / relativePath
    createDirectories(f.getParentFile)
    fileTransformer.transform(url, f)
    f.setLastModified(lastModified.getMillis)
  }

  private def urlToDestinationRelativePath(u: URL): String = {
    def packagePath = s"/${configPackageResource.path stripSuffix "/"}/"
    val relativePath = u.getPath lastIndexOf packagePath match {
      case -1 ⇒ throw new RuntimeException(s"'$u' does not contain $packagePath")
      case i ⇒ u.getPath.substring(i + packagePath.length)
    }
    renameFile.applyOrElse(relativePath, identity[String])
  }
}

object TestEnvironmentFiles {
  private val NameExtensions = Set(".xml", ".ini", ".dtd", ".js")
  private val DefaultConfigResource = JavaResource("com/sos/scheduler/engine/test/config")

  def copy(
      configJavaResource: JavaResource,
      directory: File,
      renameFile: PartialFunction[String, String] = PartialFunction.empty,
      fileTransformer: ResourceToFileTransformer = StandardResourceToFileTransformer.singleton): Unit = {
    new TestEnvironmentFiles(configJavaResource, directory, renameFile, fileTransformer).copy()
  }
}
