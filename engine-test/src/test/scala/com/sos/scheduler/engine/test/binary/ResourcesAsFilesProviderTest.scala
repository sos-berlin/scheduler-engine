package com.sos.scheduler.engine.test.binary

import com.google.common.io.Files.createTempDir
import com.sos.scheduler.engine.common.system.Files.removeDirectoryRecursivly
import com.sos.scheduler.engine.test.binary.ResourcesAsFilesProvider.provideResourcesAsFiles
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import org.springframework.core.io.support.PathMatchingResourcePatternResolver

@RunWith(classOf[JUnitRunner])
final class ResourcesAsFilesProviderTest extends FreeSpec {

  "provideResourcesAsFiles" in {
    val directory = createTempDir()
    try {
      val resources = (new PathMatchingResourcePatternResolver).getResources("classpath*:com/sos/scheduler/engine/test/files/config/*")
      provideResourcesAsFiles(resources, directory).keySet shouldEqual Set("scheduler.xml", "factory.ini", "sos.ini")
    }
    finally removeDirectoryRecursivly(directory)
  }
}

