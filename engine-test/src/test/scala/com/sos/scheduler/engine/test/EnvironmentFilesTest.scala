package com.sos.scheduler.engine.test

import com.google.common.io.Files.createTempDir
import com.sos.scheduler.engine.common.system.Files.removeFile
import com.sos.scheduler.engine.common.utils.JavaResource
import com.sos.scheduler.engine.test.EnvironmentFilesTest._
import java.io.File
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import org.springframework.core.io.support.PathMatchingResourcePatternResolver

@RunWith(classOf[JUnitRunner])
final class EnvironmentFilesTest extends FreeSpec {

  "copy" in {
    val dir = createTempDir()
    try {
      dir.list.toSet should be ('empty)
      TestEnvironmentFiles.copy(JavaResource("com/sos/scheduler/engine/test/config"), dir)
      dir.list.toSet should equal (ExpectedNames)
    }
    finally {
      for (name ← ExpectedNames) removeFile(new File(dir, name))
      removeFile(dir)
    }
  }

  "Spring getResources" in {
    val r = new PathMatchingResourcePatternResolver
    val result = r.getResources(s"classpath*:com/sos/scheduler/engine/test/config/scheduler.xml")
    result should have size 1
  }
}

private object EnvironmentFilesTest {
  private val ExpectedNames = Set("scheduler.xml", "factory.ini", "sos.ini")
}
