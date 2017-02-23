package com.sos.scheduler.engine.test

import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.jobscheduler.common.utils.JavaResource
import java.nio.file.Files.{createTempDirectory, delete, exists}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import org.springframework.core.io.support.PathMatchingResourcePatternResolver

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class TestEnvironmentFilesTest extends FreeSpec {

  "Spring getResources" in {
    val r = new PathMatchingResourcePatternResolver
    val result = r.getResources(s"classpath*:com/sos/scheduler/engine/test/testEnvironmentFilesTest/standardlayout/config/test-config.xml")
    assert(result.size == 1)
  }

  "TestEnvironmentFiles.copy with standard test package layout" in {
    val packageResource = JavaResource("com/sos/scheduler/engine/test/testEnvironmentFilesTest/standardlayout")
    val dir = createTempDirectory("test")
    TestEnvironmentFiles.copy(packageResource, dir)
    delete(dir / "config" / "live" / "test-live.xml")
    delete(dir / "config" / "live")
    delete(dir / "config" / "sos.ini")
    delete(dir / "config" / "factory.ini")
    delete(dir / "config" / "scheduler.xml")
    delete(dir / "config" / "test-config.xml")
    delete(dir / "config")
    delete(dir)
  }

  "TestEnvironmentFiles.copy with simple test package layout" in {
    val packageResource = JavaResource("com/sos/scheduler/engine/test/testEnvironmentFilesTest/simplelayout")
    val dir = createTempDirectory("test")
    TestEnvironmentFiles.copy(packageResource, dir)
    delete(dir / "config" / "live" / "subdirectory" / "test2.job.xml")
    delete(dir / "config" / "live" / "subdirectory")
    delete(dir / "config" / "live" / "test.job.xml")
    delete(dir / "config" / "live")
    delete(dir / "config" / "sos.ini")
    delete(dir / "config" / "factory.ini")
    delete(dir / "config" / "scheduler.xml")
    delete(dir / "config" / "test.xml")
    delete(dir / "config")
    delete(dir)
  }
}
