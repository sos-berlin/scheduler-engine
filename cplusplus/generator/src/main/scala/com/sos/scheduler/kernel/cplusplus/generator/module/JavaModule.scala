package com.sos.scheduler.kernel.cplusplus.generator.module

import com.sos.scheduler.kernel.cplusplus.generator.Configuration._
import JavaModule._


trait JavaModule extends Module {
    val code: String

    lazy val codeFile = new JavaCodeFile {
        val path = name.replace('.', '/') + ".java"
        val content =
            cppAndJavaComment + "\n" +
            code
    }

    lazy val codeFiles = List(codeFile)
}


object JavaModule extends ModuleKind[JavaModule] {
    def fileIsGeneratedAndCanBeDeleted(filename: String) = filename endsWith ".java"
}
