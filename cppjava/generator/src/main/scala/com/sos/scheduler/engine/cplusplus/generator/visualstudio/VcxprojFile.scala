package com.sos.scheduler.engine.cplusplus.generator.visualstudio

import com.sos.scheduler.engine.cplusplus.generator.Configuration._
import com.sos.scheduler.engine.cplusplus.generator.module.CppModule
import com.sos.scheduler.engine.cplusplus.scalautil.io.Util.closingFinally
import java.io.File
import java.io.FileOutputStream
import org.jdom._
import org.jdom.input.SAXBuilder
import org.jdom.output.XMLOutputter
import org.jdom.xpath.XPath
import scala.collection.JavaConversions._


class VcxprojFile(vcxprojFile: File, newIncludes: List[String]) {
    import VcxprojFile._

    private val saxBuilder = new SAXBuilder
    private val xmlOutputter = new XMLOutputter

    val updatedDocs = {
        val vcxprojFilterDoc = new VcxprojFilterDoc(new File(vcxprojFile.getPath + ".filters"))
        val originalIncludes = vcxprojFilterDoc.originalIncludes

        if (newIncludes.toSet == originalIncludes.toSet)
            List()
        else
            List(new VcxprojDoc(vcxprojFile), vcxprojFilterDoc) map { doc =>
                val addedIncludes = newIncludes diff originalIncludes
                doc.removeIncludes((originalIncludes diff newIncludes) ++ addedIncludes)
                doc.addIncludes(addedIncludes)
                doc
            }
    }

    def store() {
        updatedDocs foreach { _.store }
    }

    abstract class Doc(val file: File) {
        val document = saxBuilder.build(file)
        protected def projectElement = document.getRootElement
        protected def namespace = projectElement.getNamespace
        protected val clCompileElements = MyXPath("x:ItemGroup/x:ClCompile").selectElements(projectElement)

        private val cppItemGroup = {    // Unter <Project> sind mehrere <ItemGroup>. Wir nehmen die erste, die eine .cxx-Datei enthält: <ItemGroup><ClCompile Include="...cxx"/>
            val endings = cppFileExtensions map { "." + _ }
            def hasCppEnding(s: String) = endings exists s.endsWith
            def includeHasCppEnding(e: Element) = hasCppEnding(e.getAttributeValue("Include"))
            val cppClCompile = clCompileElements find includeHasCppEnding getOrElse {
                throw new RuntimeException("Visual Project file '" + file + "' contains no filename ending with " + endings.mkString(" "))
            }
            cppClCompile.getParentElement ensuring { _.getName == "ItemGroup" }
        }

        def removeIncludes(includes: Seq[String]) {
            def toBeRemoved(clCompile: Element) = includes contains includeAttributeString(clCompile)
            clCompileElements filter toBeRemoved foreach { e => e.getParent.removeContent(e) }
        }

        def addIncludes(includes: Seq[String]) {
            includes foreach { include => cppItemGroup.addContent(newClCompileElement(include)) }
        }

        protected def newClCompileElement(include: String) = new Element("ClCompile", namespace).setAttribute("Include", include)

        def toXml = xmlOutputter.outputString(document)

        def store() {
            defaultPrinter.println(file)
            closingFinally(new FileOutputStream(file)) { output => xmlOutputter.output(document, output) }
        }
    }

    class VcxprojDoc(file: File) extends Doc(file) 

    class VcxprojFilterDoc(file: File) extends Doc(file) {
        val filterFullName = {
            MyXPath("x:ItemGroup/x:ClCompile/x:Filter")
            .selectElements(projectElement) find { _.getValue() endsWith visualStudioFilterName } match {
                case Some(filter) => filter.getText     // Example: """Quelldateien\Generated C++/Java-Proxies"""
                case None => visualStudioFilterName
            }
        }

        val originalIncludes: Seq[String] = {
            val filterElements = MyXPath("x:ItemGroup/x:ClCompile/x:Filter").selectElements(projectElement)
            def isOurFilter(filter: Element) = filter.getValue endsWith visualStudioFilterName
            filterElements filter isOurFilter map { e => includeAttributeString(e.getParentElement) } distinct
        }

        override def newClCompileElement(include: String) = super.newClCompileElement(include)
            .addContent(new Element("Filter", namespace).addContent(filterFullName))
    }
}


object VcxprojFile {
    def update(dir: File, modules: Seq[CppModule]) {
        val projectFilename = dir.getName + ".vcxproj"   // "scheduler/scheduler.vcxproj"
        val projectFile = new File(dir, projectFilename)
        if (projectFile.exists)
            new VcxprojFile(projectFile, modules map { _.sourceCodeFile.path } toList).store()
    }

    private def includeAttributeString(e: Element) = e.getAttributeValue("Include") match {
        case o: String => o
        case null => throw new RuntimeException("<ClCompile> without attribute Include=")
    }
    
    private case class MyXPath(expr: String) {
        val xPath = XPath.newInstance(expr)
        xPath.addNamespace(Namespace.getNamespace("x", "http://schemas.microsoft.com/developer/msbuild/2003"))

        def selectNodes(node: Content): Seq[Content] = xPath.selectNodes(node).asInstanceOf[java.util.List[Content]]

        def selectElements(node: Content) = selectNodes(node) collect { case e: Element => e }
    }
}
