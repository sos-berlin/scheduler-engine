package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.JavaResource
import com.sos.scheduler.engine.data.filebased.TypedPath
import com.sos.scheduler.engine.test.TestEnvironmentFiles._
import com.sos.scheduler.engine.test.util.JavaResourceResolver._
import java.io.File
import java.net.URL
import java.nio.file.Files._
import java.nio.file.Path
import java.time.Instant.now
import scala.collection.immutable

private class TestEnvironmentFiles(
    configPackageResource: JavaResource,
    directory: File,
    renameFile: PartialFunction[String, String],
    fileTransformer: ResourceToFileTransformer) {

  private val lastModified = now() - 3.s

  private def copy(): Unit = for ((url, relativePath) ← resourceUrls) copyResource(url, destination(relativePath) / relativePath)

  private def destination(relPath: String) =
    if (Subdirectories exists relPath.startsWith)
      directory
    else if (TypedPath.extensions exists relPath.endsWith)
      directory / "config" / "live"
    else
      directory / "config"

  private def resourceUrls: Vector[(URL, String)] = {
    val defaultResources = knownSubdirectoriesResourceUrls(DefaultConfigResource.path) map { o ⇒ o → new File(o.getPath).getName }
    val testResources = testResourceUrls(configPackageResource) map { o ⇒ o → urlToDestinationRelativePath(o) }
    defaultResources ++ testResources
  }

  private def testResourceUrls(packageResource: JavaResource): Vector[URL] = {
    val path = packageResource.path stripSuffix "/"
    val fromKnownSubdirectories = knownSubdirectoriesResourceUrls(path)
    val fromKnownExtensions = if (fromKnownSubdirectories.isEmpty) knownExtensionsResourceUrls(path) else Nil
    fromKnownSubdirectories ++ fromKnownExtensions
  }

  // For original package layout: config/, config/live/ etc.
  private def knownSubdirectoriesResourceUrls(path: String): Vector[URL] =
    Subdirectories flatMap { o ⇒ resourcePatternToUrls(s"$path/$o/**/*") } filterNot { _.getPath endsWith "/" }

  // For simple test package layout: all files are located in test package and subdirectory and detected via its filename extensions
  private def knownExtensionsResourceUrls(path: String) =
    resourcePatternToUrls(s"$path/**/*") filter { u ⇒ KnownExtensions exists u.getPath.endsWith }

  private def copyResource(url: URL, to: Path): Unit = {
    createDirectories(to.getParentFile)
    fileTransformer.transform(url, to)
    to.setLastModified(lastModified.toEpochMilli)
  }

  private def urlToDestinationRelativePath(u: URL): String = {
    def packagePath = s"/${configPackageResource.path stripSuffix "/"}/"
    val path = u.getPath.replace("%20", " ")
    val relativePath = path lastIndexOf packagePath match {
      case -1 ⇒ throw new RuntimeException(s"'$u' does not contain $packagePath")
      case i ⇒ path.substring(i + packagePath.length)
    }
    renameFile.applyOrElse(relativePath, identity[String])
  }
}

object TestEnvironmentFiles {
  private val KnownExtensions = Set(".xml", ".ini", ".dtd", ".js", ".txt")
  private val Subdirectories = Vector("config", "agent")
  private val DefaultConfigResource = JavaResource("com/sos/scheduler/engine/test/files")

  def copy(
      configJavaResource: JavaResource,
      directory: File,
      renameFile: PartialFunction[String, String] = PartialFunction.empty,
      fileTransformer: ResourceToFileTransformer = StandardResourceToFileTransformer.singleton): Unit = {
    new TestEnvironmentFiles(configJavaResource, directory, renameFile, fileTransformer).copy()
  }
}
