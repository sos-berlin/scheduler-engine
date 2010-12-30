//package com.sos.scheduler.kernel.cplusplus.generator.visualstudio
//
//import com.sos.scheduler.kernel.cplusplus.generator.Configuration._
//import com.sos.scheduler.kernel.cplusplus.generator.module.CppModule
//import com.sos.scheduler.kernel.cplusplus.generator.util.XmlUtil._
//import java.io.File
//import java.nio.charset.Charset
//import java.util.regex.Pattern
//import scala.util.matching.Regex
//import scala.xml._
//import scala.xml.transform._
//
//
///** Visual Studio 2010 project files *.vcxproj and *.vcxproj.filter: Austausch der Namen der generierten C++-Dateien. */
//class VcxprojFile(vcxprojFile: File, newIncludes: List[String]) {
//    import VcxprojFile._
//
//    val vcxprojDoc = new VcxprojDoc(vcxprojFile)
//    val vcxprojFilterDoc = new VcxprojFilterDoc(new File(vcxprojFile.getPath + ".filters"))
//    val docs = List(vcxprojDoc, vcxprojFilterDoc)
//    private val encoding = Charset.forName("UTF-8")
//
//    def store() {
//        if (newIncludes.toSet != originalIncludes.toSet) {
//            docs foreach { doc =>
//                defaultPrinter.println(doc.file)
//                val result = doc.updatedDocument
//                XML.save(doc.file.getPath, result, enc=encoding.name)
//            }
//        }
//    }
//
//    private lazy val originalIncludes: Seq[String] = {
//        val elements = vcxprojFilterDoc.document \ "ItemGroup" \ "ClCompile" filter nodeHasOurFilter
//        elements map clCompileInclude
//    }
//
//    abstract class Doc(val file: File) {
//        val document = XML.loadFile(file)
//
//        lazy val updatedDocument: Elem = {
//            val (removedIncludes, addedIncludes) = {
//                val sameIncludes = newIncludes intersect originalIncludes
//                (originalIncludes diff sameIncludes, newIncludes diff sameIncludes)
//            }
//
//            def pathsInserted(doc: Elem) = {
//                // Unter <Project> sind mehrere <ItemGroup>. Wir nehmen die erste, die eine .cxx-Dateien enthält: <ItemGroup><ClCompile Include="...cxx"/>
//                val cxxItemGroup = {
//                    val endings = cppFileExtensions map { "." + _ }
//
//                    def aClCompileIncludeEndsWith(node: Node, endings: List[String]) =
//                        node \ "ClCompile" exists { clCompile => endings exists { end => clCompileInclude(clCompile) endsWith end } }
//
//                    doc \ "ItemGroup" find { n => aClCompileIncludeEndsWith(n, endings) } getOrElse {
//                        throw new RuntimeException("Visual Project file '" + file + "' contains no filename ending with " + endings.mkString(" "))
//                    }
//                }
//
//                val insertRule = new RewriteRule {
//                    override def transform(node: Node): Seq[Node] = node match {
//                        case n if n eq cxxItemGroup =>
//                            <ItemGroup>{n.child ++ newClCompileElements(addedIncludes)}</ItemGroup>
//                        case o => o
//                    }
//                }
//
//                replaceNodes(doc) {
//                    case node if node eq cxxItemGroup =>
//                        <ItemGroup>{node.child ++ newClCompileElements(addedIncludes)}</ItemGroup>
//                }.asInstanceOf[Elem]
//            }
//
//            def pathsRemoved(doc: Elem) = replaceNodes(doc) {
//                case node @ <ClCompile>{_*}</ClCompile>  if removedIncludes contains clCompileInclude(node) =>
//                    NodeSeq.Empty
//            }.asInstanceOf[Elem]
//
//            pathsRemoved(pathsInserted(document))
//        }
//
//        protected def newClCompileElements(paths: Seq[String]): NodeSeq
//    }
//
//    class VcxprojDoc(file: File) extends Doc(file) {
//        def newClCompileElements(includes: Seq[String]) = includes map { include =>
//            <ClCompile Include={include}/>
//        }
//    }
//
//    class VcxprojFilterDoc(file: File) extends Doc(file) {
//        private val filterFullName = {
//            document \ "ItemGroup" \ "ClCompile" find nodeHasOurFilter match {
//                case Some(node) => (node \ "Filter").text  // Example: """Quelldateien\Generated C++/Java-Proxies"""
//                case None => visualStudioFilterName
//            }
//        }
//
//        def newClCompileElements(includes: Seq[String]) = includes map { include =>
//            <ClCompile Include={include}><Filter>{filterFullName}</Filter></ClCompile>
//        }
//    }
//}
//
//
//object VcxprojFile {
//    private def clCompileInclude(node: Node) = {
//        assert(node.label == "ClCompile")
//        (node \ "@Include").text
//    }
//
//    private val FilterRegex = new Regex("""^(.*\\)?""" + Pattern.quote(visualStudioFilterName) + "$")  // Example: """Quelldateien\Generated C++/Java-Proxies""
//
//    private def nodeHasOurFilter(n: Node) = {
//        def nameIsOurFilter(name: String) = (FilterRegex findFirstMatchIn name).isDefined
//        nameIsOurFilter((n \ "Filter").text)
//    }
//
//    def update(dir: File, modules: Seq[CppModule]) {
//        val projectFilename = dir.getName + ".vcxproj"   // "scheduler/scheduler.vcxproj"
//        val projectFile = new File(dir, projectFilename)
//        if (projectFile.exists)
//            new VcxprojFile(projectFile, modules map { _.sourceCodeFile.path } toList).store()
//    }
//}
