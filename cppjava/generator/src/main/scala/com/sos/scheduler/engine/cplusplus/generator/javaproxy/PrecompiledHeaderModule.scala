package com.sos.scheduler.engine.cplusplus.generator.javaproxy

import com.sos.scheduler.engine.cplusplus.generator.Configuration._
import com.sos.scheduler.engine.cplusplus.generator.cpp.Cpp._
import com.sos.scheduler.engine.cplusplus.generator.module._
import java.io.File

class PrecompiledHeaderModule(outputDirectory: File) extends CppModule {
  val name = precompiledModuleName
  lazy val headerCodeOption = Some((cppStandardIncludes map includeQuoted).mkString)
  lazy val includeHeader = includeQuoted(headerCodeFile.path) + "\n"
  lazy val sourceCodeOption = Some(includeHeader)
}
