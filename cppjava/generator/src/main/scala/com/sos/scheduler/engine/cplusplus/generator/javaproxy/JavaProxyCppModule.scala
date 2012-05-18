package com.sos.scheduler.engine.cplusplus.generator.javaproxy

import com.sos.scheduler.engine.cplusplus.generator.cpp.Cpp._
import com.sos.scheduler.engine.cplusplus.generator.javaproxy.clas._
import com.sos.scheduler.engine.cplusplus.generator.module.CppModule
import JavaProxyCppModule._

/** Generator für C++-Code eines Java-Proxys, also der in C++ zu nutzenden Java-Klassen. */
class JavaProxyCppModule(cppClass: CppClass, knownClasses: Set[Class[_]], includeHeader: String) extends CppModule {
  val name = fileBasenameOfClass(cppClass.javaClass)

  lazy val headerCodeOption = Some(headerOnce(cppClass.headerPreprocessorMacro)(
    includeHeader +
      knownClassHeaderIncludes(cppClass.javaSuperclasses) +
    "\n" +
    cppClass.neededForwardDeclarations +
    "\n\n" +
    cppClass.headerCode))

  lazy val sourceCodeOption = Some(
    includeHeader +
    includeQuoted(headerCodeFile.path) +
    knownClassHeaderIncludes(cppClass.directlyUsedJavaClasses) +
    "\n" +
    cppClass.sourceCode)

  private def knownClassHeaderIncludes(classes: Set[Class[_]]) =
    (classes intersect knownClasses map includeClassHeader).toSeq.sorted.mkString
}


object JavaProxyCppModule {
  private def fileBasenameOfClass(clas: Class[_]) = clas.getName.replace(".", "__")
  private def includeClassHeader(clas: Class[_]) = includeQuoted(fileBasenameOfClass(clas) + ".h")
}
