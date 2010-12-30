package com.sos.scheduler.kernel.cplusplus.generator.module

import com.sos.scheduler.kernel.cplusplus.generator.Configuration._


trait CppCodeFile extends CodeFile {
    val encoding = cppEncoding

    override def toString = "C++ " + super.toString
}
