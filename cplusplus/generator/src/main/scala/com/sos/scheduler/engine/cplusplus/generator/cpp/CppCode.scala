package com.sos.scheduler.engine.cplusplus.generator.cpp


trait CppCode {
    def headerCode: String
    def sourceCode: String
}

object CppCode {
    val empty = new CppCode {
        def headerCode = ""
        def sourceCode = ""
    }
}
