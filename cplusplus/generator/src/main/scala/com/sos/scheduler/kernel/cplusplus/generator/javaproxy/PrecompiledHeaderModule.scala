package com.sos.scheduler.kernel.cplusplus.generator.javaproxy

import com.sos.scheduler.kernel.cplusplus.generator.Configuration._
import com.sos.scheduler.kernel.cplusplus.generator.cpp.Cpp._
import com.sos.scheduler.kernel.cplusplus.generator.module._
import com.sos.scheduler.kernel.cplusplus.generator.util.Util._
import com.sos.scheduler.kernel.cplusplus.scalautil.io.FileUtil._
import java.io.File
import scala.collection.mutable


class PrecompiledHeaderModule(outputDirectory: File) extends CppModule {
    val name = precompiledModuleName

    lazy val headerCodeOption = Some((cppStandardIncludes map includeQuoted).mkString)

    lazy val includeHeader = includeQuoted(headerCodeFile.path) + "\n"
    
    lazy val sourceCodeOption = Some(includeHeader)
}
