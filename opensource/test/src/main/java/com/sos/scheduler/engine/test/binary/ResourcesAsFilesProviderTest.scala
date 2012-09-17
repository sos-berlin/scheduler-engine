package com.sos.scheduler.engine.test.binary

import com.google.common.io.Files.createTempDir
import com.sos.scheduler.engine.kernel.util.Classes.springPattern
import com.sos.scheduler.engine.kernel.util.Files.removeDirectoryRecursivly
import com.sos.scheduler.engine.test.SchedulerTest
import com.sos.scheduler.engine.test.binary.ResourcesAsFilesProvider.provideResourcesAsFiles
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import org.springframework.core.io.support.PathMatchingResourcePatternResolver

@RunWith(classOf[JUnitRunner])
final class ResourcesAsFilesProviderTest extends FunSuite {

  test("provideResourcesAsFiles") {
    val directory = createTempDir()
    try {
      val pattern = springPattern(classOf[SchedulerTest].getPackage, "config/*")
      val resources = (new PathMatchingResourcePatternResolver).getResources(pattern)
      provideResourcesAsFiles(resources, directory).keySet should equal (Set("scheduler.xml", "factory.ini", "sos.ini"))
    } finally
      removeDirectoryRecursivly(directory)
  }
}

