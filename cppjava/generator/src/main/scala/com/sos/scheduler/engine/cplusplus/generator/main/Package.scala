package com.sos.scheduler.engine.cplusplus.generator.main

import com.sos.scheduler.engine.cplusplus.generator.util.PackageOps._
import com.sos.scheduler.engine.cplusplus.runtime.CppProxy
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp

case class Package(name: String) {
  import Package._

  def relevantClasses = {
    val allClasses = classesOfPackage(name)
    require(allClasses.nonEmpty, "Package '" + name + "' is empty")
    val result = allClasses filter classIsRelevant
    require(result.nonEmpty, "Package '" + name + "' contains no relevant class:\n" + info(allClasses))
    result
  }

  private def info(classes: Iterable[Class[_]]) = {
    def f(c: Class[_]) = c.getName +
        " isInterface=" + c.isInterface +
        " inheritsCppProxy=" + classOf[CppProxy].isAssignableFrom(c) +
        " cppProxyPackage=" + (c.getPackage != classOf[CppProxy].getPackage) +
        " @ForCpp=" + classIsForCpp(c)
    classes map f mkString "\n"
  }
}

object Package {
  private def classIsRelevant(c: Class[_]) = classIsCppProxy(c) || classIsForCpp(c)

  private def classIsCppProxy(c: Class[_]) = c.isInterface  &&  classOf[CppProxy].isAssignableFrom(c)  &&
    c.getPackage != classOf[CppProxy].getPackage

  private def classIsForCpp(c: Class[_]) = c.getAnnotation(classOf[ForCpp]) != null && !isScalaSingletonClass(c)

  private def isScalaSingletonClass(c: Class[_]) = c.getName endsWith "$"
}
