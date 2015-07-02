package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.utils.JavaResource
import java.nio.file.Files
import java.nio.file.Files.createTempDirectory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class TestEnvironmentFilesTest extends FreeSpec {

  private val packageResource = JavaResource("com/sos/scheduler/engine/test/testEnvironmentFilesTest/")

  "TestEnvironmentFiles.copy" in {
    val dir = createTempDirectory("test")
    TestEnvironmentFiles.copy(packageResource, dir)
    val subfile = dir / "subdirectory/test2.xml"
    assert(subfile.exists)
    val files = Files.list(dir).toImmutableSeq
    assert(files.toSet == (Set("test.xml", "subdirectory", "scheduler.xml", "factory.ini", "sos.ini") map dir./))
    Files.delete(subfile)
    files foreach Files.delete
    Files.delete(dir)
  }
}
