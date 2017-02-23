package com.sos.scheduler.engine.test.binary

import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.time.TimeoutWithSteps
import com.sos.jobscheduler.common.time.WaitForCondition.{waitForCondition, waitFromNowFor}
import com.sos.scheduler.engine.common.system.Files.copyURLToFile
import com.sos.scheduler.engine.kernel.util.Util.ignore
import com.sos.scheduler.engine.test.binary.ResourcesAsFilesProvider._
import java.io.File
import java.time.Duration
import org.springframework.core.io.{FileSystemResource, Resource}

/** Stellt Resourcen als Dateien zur Verfügung.
  * Basiert auf [[org.springframework.core.io.support.PathMatchingResourcePatternResolver]], das nicht in jeder Umgebung funktionieren muss.
  * Ist also nur für den Test des Schedulers geeignet, nicht unbedingt für die Produktion. */
final class ResourcesAsFilesProvider(resources: Iterable[Resource], directory: File) {

  if (resources.isEmpty) logger.warn("No resources")

  private def apply() = resources map { r ⇒ r.getFilename → provideResourceAsFile(r) }

  private def provideResourceAsFile(r: Resource): ResourceFile = r match {
    case f: FileSystemResource ⇒
      new ResourceFile(f.getFile, false)
    case _ ⇒
      try {
        val f = new File(directory, r.getFilename)
        val copy = !resourceSeemsEqualToFile(r, f)
        if (copy) copyResource(r, f)
        new ResourceFile(f, copy)
      } catch {
        case x: Exception ⇒ throw new RuntimeException("Error while providing "+ r +": "+ x, x)
      }
    }
  }

object ResourcesAsFilesProvider {
  private final val logger = Logger(getClass)
  private final val lockFileTimeoutWithSteps = TimeoutWithSteps(5.s, 50.ms)

  private def copyResource(r: Resource, f: File): Unit = {
    logger.debug(s"copyURLToFile(${r.getURL}, $f)")
    copyURLToFile(r.getURL, f)
    val ok = waitFromNowFor(0L to 5000 by 10) { f.setLastModified(r.lastModified) }   // Jemand verzögert das Sperren der Datei? Der Windwos-Virenscanner?
    assert(ok, s"setLastModified $f")
    assert(resourceSeemsEqualToFile(r, f), s"Copying resource failed: $f")
  }

  private def resourceSeemsEqualToFile(r: Resource, f: File) =
    f.exists && f.lastModified == r.lastModified && f.length == r.contentLength

  def provideResourcesAsFiles(resources: Iterable[Resource], directory: File): Map[String, ResourceFile] = {
    val lockFile = new File(directory, "CREATION-LOCK")
    synchronizeWithParallelTests(lockFile, 5.s) {
      new ResourcesAsFilesProvider(resources, directory).apply().toMap
    }
  }

  private def synchronizeWithParallelTests[A](lockFile: File, timeout: Duration)(f: ⇒ A): A = {
    val lockExists = !lockFile.createNewFile
    if (lockExists) {
      waitForCondition(lockFileTimeoutWithSteps) { !lockFile.exists }
      if (lockFile.exists())
        logger.warn(s"Lock file $lockFile exists longer than ${lockFileTimeoutWithSteps.timeout.pretty}")
    }
    val result = try f
    finally ignore(lockFile.delete)
    result
  }
}
