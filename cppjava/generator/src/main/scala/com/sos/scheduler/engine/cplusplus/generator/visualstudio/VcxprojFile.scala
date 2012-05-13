package com.sos.scheduler.engine.cplusplus.generator.visualstudio

import com.sos.scheduler.engine.cplusplus.generator.Configuration._
import com.sos.scheduler.engine.cplusplus.generator.module.CppModule
import com.sos.scheduler.engine.cplusplus.scalautil.io.Util.closingFinally
import java.io.{File, FileOutputStream}
import org.jdom._
import org.jdom.input.SAXBuilder
import org.jdom.output.XMLOutputter
import org.jdom.xpath.XPath
import org.slf4j.LoggerFactory
import scala.collection.JavaConversions._

class VcxprojFile(vcxprojFile: File, newIncludes: List[String]) {
  import VcxprojFile._

  private val xmlOutputter = new XMLOutputter
  private val logger = LoggerFactory.getLogger(classOf[VcxprojFile].getName +" "+vcxprojFile.getName)

  logger.debug(vcxprojFile.toString)

  val updatedDocs = {
    val vcxprojFilterDoc = new VcxprojFilterDoc(new File(vcxprojFile.getPath + ".filters"))
    val originalGeneratedIncludes = vcxprojFilterDoc.originalGeneratedIncludes

    if (newIncludes.toSet == originalGeneratedIncludes.toSet)
      List()
    else
      List(new VcxprojDoc(vcxprojFile), vcxprojFilterDoc) map { doc =>
        val addedIncludes = newIncludes diff originalGeneratedIncludes
        doc.removeIncludes((originalGeneratedIncludes diff newIncludes) ++ addedIncludes)
        doc.addIncludes(addedIncludes)
        doc
      }
  }

  def store() {
    updatedDocs foreach { _.store() }
  }

  sealed abstract class Doc(val file: File) {
    private val document = readXml(file)
    protected def projectElement = document.getRootElement
    protected def namespace = projectElement.getNamespace
    protected val clCompileElements = MyXPath("x:ItemGroup/x:ClCompile").selectElements(projectElement)

    private val cppItemGroup = {    // Unter <Project> sind mehrere <ItemGroup>. Wir nehmen die erste, die eine .cxx-Datei enthält: <ItemGroup><ClCompile Include="...cxx"/>
      val endings = cppFileExtensions map { "."+ _ }
      def hasCppEnding(s: String) = endings exists s.endsWith
      def includeHasCppEnding(e: Element) = hasCppEnding(e getAttributeValue "Include")
      val cppClCompile = clCompileElements find includeHasCppEnding getOrElse {
        throw new RuntimeException("Visual Project file '"+ file +"' contains no filename ending with "+ (endings mkString " "))
      }
      cppClCompile.getParentElement ensuring { _.getName == "ItemGroup" }
    }

    def removeIncludes(includes: Seq[String]) {
      def toBeRemoved(clCompile: Element) = includes contains includeAttributeString(clCompile)
      logger.debug("clCompileElements="+clCompileElements+", includes="+includes+", toBeRemoved="+(clCompileElements filter toBeRemoved))
      clCompileElements filter toBeRemoved foreach { e =>
        logger.debug("Removing element <"+ e.getParent.asInstanceOf[Element].getName +"> -> "+ xml(e))
        e.getParent.removeContent(e)
      }
    }

    def addIncludes(includes: Seq[String]) {
      includes foreach { include =>
        val e = newClCompileElement(include)
        logger.debug("Adding element "+ xml(e))
        cppItemGroup.addContent(e)
        cppItemGroup.addContent("\n    ")
      }
    }

    protected def newClCompileElement(include: String) = new Element("ClCompile", namespace).setAttribute("Include", include)

    def toXml = xmlOutputter.outputString(document)

    def store() {
      defaultPrinter.println(file)
      closingFinally(new FileOutputStream(file)) { output => xmlOutputter.output(document, output) }
    }

    override def toString = file.getName
  }

  final class VcxprojDoc(file: File) extends Doc(file)

  final class VcxprojFilterDoc(file: File) extends Doc(file) {
    val originalGeneratedIncludes: Seq[String] = {
      val filterElements = MyXPath("x:ItemGroup/x:ClCompile/x:Filter").selectElements(projectElement)
      def isOurFilter(filter: Element) = filter.getValue endsWith visualStudioGeneratedFilesFilterName
      (filterElements filter isOurFilter map { e => includeAttributeString(e.getParentElement) }).distinct
    }

    val filterFullName = (MyXPath("x:ItemGroup/x:Filter[@Include]").selectElements(projectElement)
        map { _.getAttributeValue("Include", "") }
        find { _ endsWith visualStudioGeneratedFilesFilterName }  // Example: """Quelldateien\Generated C++/Java-Proxies"""
        getOrElse visualStudioGeneratedFilesFilterName)

    override def newClCompileElement(include: String) = {
      val filter = new Element("Filter", namespace).addContent(filterFullName)
      super.newClCompileElement(include)
          .addContent("\n      ")
          .addContent(filter)
          .addContent("\n    ")
    }
  }

  override def toString = vcxprojFile.getName
}

object VcxprojFile {
  def update(dir: File, modules: Seq[CppModule]) {
    val projectFilename = dir.getName + ".vcxproj"   // "scheduler/scheduler.vcxproj"
    val projectFile = new File(dir, projectFilename)
    if (projectFile.exists)
      new VcxprojFile(projectFile, modules map { _.sourceCodeFile.path } toList).store()
  }

  private def includeAttributeString(e: Element) = Option(e getAttributeValue "Include") getOrElse { throw new RuntimeException("<ClCompile> without attribute Include=") }

  private def readXml(f: File) = new SAXBuilder().build(f)

  private def xml(o: Element) = new XMLOutputter().outputString(o)

  private case class MyXPath(expr: String) {
    val xPath = XPath.newInstance(expr)
    xPath.addNamespace(Namespace.getNamespace("x", "http://schemas.microsoft.com/developer/msbuild/2003"))

    def selectNodes(node: Content): Seq[Content] = xPath.selectNodes(node).asInstanceOf[java.util.List[Content]]

    def selectElements(node: Content) = selectNodes(node) collect { case e: Element => e }
  }
}
