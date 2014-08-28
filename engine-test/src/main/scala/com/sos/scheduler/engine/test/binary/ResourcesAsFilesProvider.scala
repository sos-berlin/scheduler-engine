package com.sos.scheduler.engine.test.binary

import com.sos.scheduler.engine.common.system.Files.copyURLToFile
import com.sos.scheduler.engine.kernel.util.Util.ignore
import com.sos.scheduler.engine.test.util.time.TimeoutWithSteps
import com.sos.scheduler.engine.test.util.time.WaitForCondition.{waitForCondition, waitFromNowFor}
import java.io.File
import org.joda.time.Duration
import org.joda.time.Duration.{millis, standardSeconds}
import org.slf4j.LoggerFactory.getLogger
import org.springframework.core.io.FileSystemResource
import org.springframework.core.io.Resource

/** Stellt Resourcen als Dateien zur Verfügung -
  * basiert auf [[org.springframework.core.io.support.PathMatchingResourcePatternResolver]], das nicht in jeder Umgebung funktionieren muss.
  * Ist also nur für den Test des Schedulers geeignet, nicht unbedingt für die Produktion. */
class ResourcesAsFilesProvider(resources: Iterable[Resource], directory: File) {

  import ResourcesAsFilesProvider._

  if (resources.isEmpty) logger.warn("No resources")

  private def apply() = resources map { r => r.getFilename -> provideResourceAsFile(r) }

  private def provideResourceAsFile(r: Resource): ResourceFile = r match {
    case f: FileSystemResource =>
      new ResourceFile(f.getFile, false)
    case _ =>
      try {
        val f = new File(directory, r.getFilename)
        val copy = !resourceSeemsEqualToFile(r, f)
        if (copy) copyResource(r, f)
        new ResourceFile(f, copy)
      } catch {
        case x: Exception => throw new RuntimeException("Error while providing "+ r +": "+ x, x)
      }
    }
  }

object ResourcesAsFilesProvider {
  private final val logger = getLogger(classOf[ResourcesAsFilesProvider])
  private final val lockFileTimeoutWithSteps = TimeoutWithSteps(standardSeconds(5), millis(50))

  private def copyResource(r: Resource, f: File): Unit = {
    logger.debug("copyURLToFile("+ r.getURL +", "+ f +")")
    copyURLToFile(r.getURL, f)
    val ok = waitFromNowFor(0L to 5000 by 10) { f.setLastModified(r.lastModified) }   // Jemand verzögert das Sperren der Datei? Der Windwos-Virenscanner?
    assert(ok, s"setLastModified $f")
    assert(resourceSeemsEqualToFile(r, f), s"Copying resource failed: $f")
  }

  private def resourceSeemsEqualToFile(r: Resource, f: File) =
    f.exists && f.lastModified == r.lastModified && f.length == r.contentLength

  def provideResourcesAsFiles(resources: Iterable[Resource], directory: File): Map[String, ResourceFile] = {
    val lockFile = new File(directory, "CREATION-LOCK")
    synchronizeWithParallelTests(lockFile, standardSeconds(5)) {
      new ResourcesAsFilesProvider(resources, directory).apply().toMap
    }
  }

  private def synchronizeWithParallelTests[A](lockFile: File, timeout: Duration)(f: => A): A = {
    val lockExists = !lockFile.createNewFile
    if (lockExists) {
      waitForCondition(lockFileTimeoutWithSteps) { !lockFile.exists }
      if (lockFile.exists())
        logger.warn("Lock file "+lockFile+" exists longer than "+lockFileTimeoutWithSteps.timeout)
    }
    val result = try f
    finally ignore(lockFile.delete)
    result
  }
}
