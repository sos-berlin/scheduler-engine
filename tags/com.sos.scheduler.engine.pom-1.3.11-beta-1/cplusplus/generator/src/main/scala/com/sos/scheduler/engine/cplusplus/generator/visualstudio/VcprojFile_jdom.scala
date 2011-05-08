//package com.sos.scheduler.engine.cplusplus.generator.cppproxy
//
//import com.sos.scheduler.engine.cplusplus.generator.Configuration._
//import com.sos.scheduler.engine.cplusplus.generator.cpp.CppModule
//import com.sos.scheduler.engine.cplusplus.generator.helper.XmlUtil._
//import com.sos.scheduler.engine.cplusplus.scalautil.io.Util.closingFinally
//import java.io.File
//import java.io.FileOutputStream
//import java.io.StringWriter
//import org.jdom._
//import org.jdom.input.SAXBuilder
//import org.jdom.output.XMLOutputter
//import org.jdom.xpath.XPath
//import scala.collection.JavaConversions._
//
//
//class VisualStudioProjectFile(file: File) {
//    private val saxBuilder = new SAXBuilder
//    val document = saxBuilder.build(file)
//    private var modified = false
//
//    def updateAndStore(modules: List[CppModule]) {
//        update(modules)
//        store()
//    }
//
//    def update(modules: List[CppModule]) {
//        val modulePaths = modules map { _.sourceCodeFile.path }
//        filterElementOption foreach { filterElement =>
//            if (pathsOfFilterElement(filterElement).toSet != modulePaths.toSet) {
//                filterElement.setContent(filterElementOfPaths(modulePaths))
//                modified = true
//            }
//        }
//    }
//
//    private def filterElementOption = {
//        val xpath = XPath.newInstance("/VisualStudioProject/Files//Filter[@Name='" + visualStudioSourceFileFilterName + "']")
//        val result = xpath.selectNodes(document).asInstanceOf[java.util.List[Element]].headOption
//        System.err.println("********* filter=" + result)
//        result
//    }
//
//    private def pathsOfFilterElement(filterElement: Element): List[String] = {
//        val files = XPath.newInstance("File").selectNodes(filterElement).toList
//        files flatMap { case e: Element => Option(e.getAttributeValue("RelativePath")) }
//    }
//
//    private def filterElementOfPaths(paths: List[String]) =
//        new Element("Filter")
//        .setAttribute("Name", visualStudioSourceFileFilterName)
//        .addContent(paths map { p => new Element("File").setAttribute("RelativePath", p) })
//
//    private def normalizedPath(path: String) = """^[.][\/]""".r.replaceFirstIn(path, "")
//
//    def toXml = closingFinally(new StringWriter()) { w =>
//        new XMLOutputter().output(document, w)
//        w.toString
//    }
//
//
//    def store() {
//        if (modified) {
//            defaultPrinter.println(file)
//            closingFinally(new FileOutputStream(file)) { output => new XMLOutputter().output(document, output) }
//        }
//    }
//
//    private def contentIsCppproxyFilter(c: Content) = c match {
//        case e: Element => e.getName == "Filter"  &&  visualStudioSourceFileFilterName.equals(e.getAttributeValue("Name"))
//        case _ => false
//    }
//
//    def replaceContents(root: Content)(f: PartialFunction[Content, Content]) = {
//        def t(content: Content): Content = content match {
//            case n if (f isDefinedAt n) => f(n)
//            case e: Element if e.getDescendants.asInstanceOf[java.util.List[Content]] exists f.isDefinedAt =>
//                new Element(e.getName).addContent(e.getChildren.asInstanceOf[java.util.List[Content]] map t)
//            case n => n
//        }
//        t(root)
//    }
//}
