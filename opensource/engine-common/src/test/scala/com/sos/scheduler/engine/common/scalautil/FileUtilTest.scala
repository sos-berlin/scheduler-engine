package com.sos.scheduler.engine.common.scalautil

import com.sos.scheduler.engine.common.scalautil.FileUtilTest._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import java.io.File
import java.nio.charset.StandardCharsets.{UTF_16BE, UTF_8}
import java.nio.file.{Files, Path}
import org.scalatest.Matchers._
import org.scalatest.{BeforeAndAfterAll, FreeSpec}


/**
 * @author Joacim Zschimmer
 */
final class FileUtilTest extends FreeSpec with BeforeAndAfterAll {

  private lazy val file = Files.createTempFile("FileUtilTest-", ".tmp").toFile

  override def afterAll(): Unit = {
    Files.delete(file)
  }

  "implicit fileToPath" in {
    new File("/a"): Path
  }

  "implicit pathToFile" in {
    new File("/a").toPath: File
  }

  "slash" in {
    assert(new File("/a") / "b" == new File("/a", "b"))
  }

  "contentString" in {
    file.contentString = TestString
    assert(file.contentString == TestString)
    assert(new String(Files.readAllBytes(file), UTF_8) == TestString)
  }

  "contentBytes" in {
    assert(file.contentBytes.toVector == TestBytes)
    file.contentBytes = Array[Byte](1, 2)
    assert(file.contentBytes.toVector == Vector[Byte](1, 2))
  }

  "write" in {
    file.write(TestString, UTF_16BE)
    assert(file.contentBytes.toVector == TestString.getBytes(UTF_16BE).toVector)
  }

  "append" in {
    file.append("X", UTF_16BE)
    assert(file.contentString(UTF_16BE) == TestString + "X")
  }
}

private object FileUtilTest {
  private val TestString = "AÅ"
  private val TestBytes = TestString.getBytes(UTF_8).toVector
  assert(TestBytes.length == 3)
}
