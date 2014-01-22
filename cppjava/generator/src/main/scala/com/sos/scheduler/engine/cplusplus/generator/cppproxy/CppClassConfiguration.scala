package com.sos.scheduler.engine.cplusplus.generator.cppproxy

import com.sos.scheduler.engine.cplusplus.generator.util.MyRichString._
import com.sos.scheduler.engine.cplusplus.runtime.annotation

/** Parameter für einen C++-Proxy, angegeben als Annotation @CppClass zum Java-Interface, das den Proxy beschreibt. */
final case class CppClassConfiguration(
    interface: Class[_],
    className: String,
    directory: String = "",
    includeOption: Option[String] = None)


object CppClassConfiguration {
  def apply(interface: Class[_]) = interface.getAnnotation(classOf[annotation.CppClass]) match {
    case null =>
      new CppClassConfiguration(interface, className = interface.getSimpleName)
    case a =>
      new CppClassConfiguration(interface, className = a.clas, directory = a.directory, includeOption = noneIfEmpty(a.include))
  }
}
