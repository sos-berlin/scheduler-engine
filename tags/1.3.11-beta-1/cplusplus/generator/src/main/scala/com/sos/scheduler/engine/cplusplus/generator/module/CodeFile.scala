package com.sos.scheduler.engine.cplusplus.generator.module

import com.sos.scheduler.engine.cplusplus.generator.util.Util._
import com.sos.scheduler.engine.cplusplus.scalautil.io.FileUtil._
import java.io.File
import java.nio.charset.Charset


trait CodeFile {
    val path: String

    val encoding: Charset

    def content: String

    def writeToDirectory(directory: File) {
        requireDirectoryExists(directory, "Missing output directory for '" + this + "': " + directory)
        writingFileAndLog(new File(directory, path), encoding) { _ write content }
    }

    override def toString = "file " + path
}
