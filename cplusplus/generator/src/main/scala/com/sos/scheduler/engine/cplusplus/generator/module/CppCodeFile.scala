package com.sos.scheduler.engine.cplusplus.generator.module

import com.sos.scheduler.engine.cplusplus.generator.Configuration._


trait CppCodeFile extends CodeFile {
    val encoding = cppEncoding

    override def toString = "C++ " + super.toString
}
