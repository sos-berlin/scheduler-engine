package com.sos.scheduler.kernel.cplusplus.generator.module

import com.sos.scheduler.kernel.cplusplus.generator.Configuration._
import com.sos.scheduler.kernel.cplusplus.generator.module._


trait CppModule extends Module {
    protected val headerCodeOption: Option[String]
    protected val sourceCodeOption: Option[String]

    lazy val headerCodeFileOption = cppCodeFileOption(headerCodeOption, "h")
    lazy val sourceCodeFileOption = cppCodeFileOption(sourceCodeOption, cppFileExtension)

    lazy val headerCodeFile = headerCodeFileOption.get
    lazy val sourceCodeFile = sourceCodeFileOption.get

    private def cppCodeFileOption(content: Option[String], fileExtension: String) = content map { c =>
        new CppCodeFile {
            val path = name + "." + fileExtension
            lazy val content =
                cppAndJavaComment + "\n" +
                c
        }
    }

    lazy val codeFiles = (headerCodeFileOption ++ sourceCodeFileOption).toList
}


object CppModule extends ModuleKind[CppModule] {
    def fileIsGeneratedAndCanBeDeleted(filename: String) = (filename endsWith ".h") || (filename endsWith cppFileExtension)
}
