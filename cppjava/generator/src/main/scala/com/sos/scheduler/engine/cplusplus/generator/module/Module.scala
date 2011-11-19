package com.sos.scheduler.engine.cplusplus.generator.module

import java.io.File


trait Module {
    val name: String
    
    val codeFiles: List[CodeFile]

    def writeToDirectory(d: File) {
        require(d.getPath.nonEmpty, "Missing output directory")
        codeFiles foreach { _.writeToDirectory(d) }
    }
}
