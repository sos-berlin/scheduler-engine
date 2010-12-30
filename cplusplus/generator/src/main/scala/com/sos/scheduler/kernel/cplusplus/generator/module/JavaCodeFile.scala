package com.sos.scheduler.kernel.cplusplus.generator.module

import com.sos.scheduler.kernel.cplusplus.generator.Configuration._


trait JavaCodeFile extends CodeFile {
    val encoding = javaEncoding
    
    override def toString = "Java " + super.toString
}
