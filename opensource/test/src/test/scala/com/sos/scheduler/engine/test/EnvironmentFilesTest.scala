package com.sos.scheduler.engine.test

import EnvironmentFilesTest._
import com.google.common.io.Files.createTempDir
import com.sos.scheduler.engine.common.system.Files.removeFile
import com.sos.scheduler.engine.kernel.util.Classes.springPattern
import com.sos.scheduler.engine.kernel.util.ResourcePath
import java.io.File
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import org.springframework.core.io.support.PathMatchingResourcePatternResolver

@RunWith(classOf[JUnitRunner])
final class EnvironmentFilesTest extends FunSuite {

  test("copy") {
    val dir = createTempDir()
    try {
      dir.list.toSet should be ('empty)
      TestEnvironmentFiles.copy(new ResourcePath(classOf[EnvironmentFilesTest].getPackage, "config"), dir)
      dir.list.toSet should equal (expectedNames)
    }
    finally {
      for (name <- expectedNames) removeFile(new File(dir, name))
      removeFile(dir)
    }
  }

  test("Spring getResources") {
    val r = new PathMatchingResourcePatternResolver
    val result = r.getResources(springPattern(classOf[EnvironmentFilesTest].getPackage, "config/scheduler.xml"))
    result should have size(1)
  }
}

private object EnvironmentFilesTest {
  val expectedNames = Set("scheduler.xml", "factory.ini", "sos.ini")
}
