package com.sos.scheduler.engine.cplusplus.generator.main

import Main._
import com.sos.scheduler.engine.cplusplus.generator._
import com.sos.scheduler.engine.cplusplus.runtime.CppProxy

/** Generiert Java- und C++-Proxys.
  * Für ein C++-Proxy in Java wird ein Java-Interface erwartet, das die C++-Klasse beschreibt.
  * Als Programmparameter können Klassen oder Java-Pakete angegeben werden.
  * Aus den Java-Paketen wählt der Generator die Interfaces aus, die das Interface CppProxy implementieren
  * (aber nicht mit @JavaOnlyInterface annotiert sind)
  * oder die Klassen oder Interfaces, die mit @ForCpp annotiert sind.
  */
final class Main(args: Array[String]) {
  private val parameters = Parameters.ofCommandLine(args)

  private val (cppProxyInterfaces, javaProxies) = {
    val (classNames, packageNames) = parameters.classOrPackageNames partition isClassName
    val packageClasses = packageNames flatMap { name ⇒
      Package(name).relevantClasses(classNameFilter = { name ⇒ !classNameIsExcluded(parameters.excludedPackagesOrClasses)(name) })
    }
    val classes = (classNames map Class.forName) ++ packageClasses
    classes partition classOf[CppProxy].isAssignableFrom
  }

  def apply(): Unit = {
    generateCppProxies()
    generateJavaProxies()
  }

  private def generateCppProxies(): Unit = {
    cppproxy.Generator.generate(parameters.cppOutputDirectory, parameters.javaOutputDirectory, cppProxyInterfaces)
  }

  private def generateJavaProxies(): Unit = {
    for (dir <- parameters.cppOutputDirectory)
      javaproxy.Generator.generate(dir, javaProxies ++ cppProxyInterfaces, parameters.deep)
  }
}

/** Exception nicht nach stderr aus und gibt keinen Exit code zurück (s. MainWithExitCode). */
object Main {
  def main(args: Array[String]): Unit = {
    new Main(args).apply()
  }

  private[main] def isClassName(x: String) = {
    val simpleName = ("" :: x.split('.').toList).last
    simpleName.nonEmpty && simpleName.head.isUpper
  }

  private[main] def classNameIsExcluded(excludedClassOrPackageNames: Set[String])(className: String) =
    excludedClassOrPackageNames(className) || (excludedClassOrPackageNames exists { e ⇒ className startsWith s"$e." } )
}
