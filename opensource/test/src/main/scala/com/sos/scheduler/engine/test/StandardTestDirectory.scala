package com.sos.scheduler.engine.test

import StandardTestDirectory._
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.scalautil.SideEffect._
import com.sos.scheduler.engine.common.system.Files._
import java.io.File

trait StandardTestDirectory
extends HasTestDirectory {
  this: HasCloser =>

  protected def testName: String

  lazy val testDirectory: File =
    System.getProperty(workDirectoryPropertyName) match {
      case null =>
        val result = new File(System.getProperty("java.io.tmpdir"), s"test-$testName")
        onClose { tryRemoveDirectoryRecursivly(testDirectory) }
        result
      case workDir =>
        new File(workDir).mkdir
        new File(workDir, testName) sideEffect makeCleanDirectory
    }
}

private object StandardTestDirectory {
  private val workDirectoryPropertyName = "com.sos.scheduler.engine.test.directory"

  private def makeCleanDirectory(directory: File) {
    makeDirectory(directory)
    removeDirectoryContentRecursivly(directory)
  }
}
