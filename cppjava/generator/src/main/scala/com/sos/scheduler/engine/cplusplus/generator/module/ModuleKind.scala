package com.sos.scheduler.engine.cplusplus.generator.module

import com.sos.scheduler.engine.cplusplus.generator.Configuration._
import com.sos.scheduler.engine.cplusplus.scalautil.io.FileUtil._
import java.io.File

/** Beschreibt eine Modul-Art, also z.B. Java- oder JNI-Module */
trait ModuleKind[M <: Module] {
  def fileIsGeneratedAndCanBeDeleted(filename: String): Boolean

  def removeFilesBut(dir: File, retainModules: Seq[Module]) {
    requireDirectoryExists(dir)

    def directoryIsForeign(file: File) = file.getName startsWith "."
    def fileIsRelevant(f: File) = if (f.isDirectory) !directoryIsForeign(f) else fileIsGeneratedAndCanBeDeleted(f.getName)
    val paths = listFilePathsRecursiveFiltered(dir)(fileIsRelevant)
    val retainPaths = retainModules flatMap { _.codeFiles map { _.path } }
    val deletePaths = paths diff retainPaths
    for (file <- deletePaths map { p ⇒ new File(dir, p) }) {
        defaultPrinter.println(s"Remove file $file")
        file.delete()
    }
  }
}
