package com.sos.scheduler.kernel.cplusplus.generator.main

import com.sos.scheduler.kernel.cplusplus.generator.util.PackageOps._
import com.sos.scheduler.kernel.cplusplus.runtime.CppProxy
import com.sos.scheduler.kernel.cplusplus.runtime.annotation.ForCpp
import Package._


case class Package(name: String)
{
    def relevantClasses = {
        val result = classesOfPackage(name) filter classIsRelevant
        require(result.nonEmpty, "Package '" + name + "' contains no relevant class")
        result
    }
}


object Package {
    private def classIsRelevant(c: Class[_]) = classIsCppProxy(c) || classIsForCpp(c)

    private def classIsCppProxy(c: Class[_]) = c.isInterface()  &&  classOf[CppProxy].isAssignableFrom(c)  &&
        c.getPackage() != classOf[CppProxy].getPackage

    private def classIsForCpp(c: Class[_]) = c.getAnnotation(classOf[ForCpp]) != null
}
