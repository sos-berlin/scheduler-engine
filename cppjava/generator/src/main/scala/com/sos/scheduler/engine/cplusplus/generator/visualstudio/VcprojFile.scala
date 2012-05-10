package com.sos.scheduler.engine.cplusplus.generator.visualstudio

import com.sos.scheduler.engine.cplusplus.generator.Configuration._
import com.sos.scheduler.engine.cplusplus.generator.module.CppModule
import com.sos.scheduler.engine.cplusplus.generator.util.XmlUtil._
import java.io.File
import java.nio.charset.Charset
import scala.xml._


/** Visual Studio 2008 project file *.vcproj. Austausch der Namen der generierten C++-Dateien. */
class VcprojFile(file: File, newPaths: List[String]) {
  private val encoding = Charset.forName("Windows-1252")      // Besser: Dem Original-Dokument entnehmen
  private val document = XML.loadFile(file)

  def store() {
    originalPathsOption foreach { o =>
      if (o.toSet != newPaths.toSet) {
        defaultPrinter.println(file)
        saveAndCorrectScalaBug(file, updatedDocument , encoding)     //XML.save(file.getPath, updatedDocument, enc="Windows-1252")
      }
    }
  }

  def updatedDocument: Elem = {
    val newFilter = newFilterElement(newPaths)
    replaceNodes(document) {
      case e @ <Filter>{_*}</Filter>  if elementIsCppproxyFilter(e) => newFilter
    }.asInstanceOf[Elem]
  }

  /** @return None, wenn Filter-Element fehlt */
  private lazy val originalPathsOption: Option[Seq[String]] = {
    val filterOption = document \ "Files" \\ "Filter" find elementIsCppproxyFilter
    filterOption map { _ \ "File" map { f => normalizedPath((f \ "@RelativePath").text) } }
  }

  private def normalizedPath(path: String) = """^[.][\/]""".r.replaceFirstIn(path, "")

  private def newFilterElement(paths: Seq[String]) =
    <Filter Name={visualStudioFilterName}>{
      NodeSeq.fromSeq(paths map { p => <File RelativePath={p}/>})
    }</Filter>

  def elementIsCppproxyFilter(e: Node) = e.label == "Filter"  &&  (e \ "@Name").text == visualStudioFilterName
}


object VcprojFile {
  def update(dir: File, modules: Seq[CppModule]) {
    val projectFilename = dir.getName + ".vcproj"   // "scheduler/scheduler.vcproj"
    val projectFile = new File(dir, projectFilename)
    if (projectFile.exists)
      new VcprojFile(projectFile, modules map { _.sourceCodeFile.path } toList).store()
  }
}
