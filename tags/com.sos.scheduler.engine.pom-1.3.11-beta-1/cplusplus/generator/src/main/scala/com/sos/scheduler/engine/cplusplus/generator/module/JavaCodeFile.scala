package com.sos.scheduler.engine.cplusplus.generator.module

import com.sos.scheduler.engine.cplusplus.generator.Configuration._


trait JavaCodeFile extends CodeFile {
    val encoding = javaEncoding
    
    override def toString = "Java " + super.toString
}
