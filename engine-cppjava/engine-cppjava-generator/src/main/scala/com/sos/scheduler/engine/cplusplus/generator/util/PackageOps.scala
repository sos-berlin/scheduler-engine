package com.sos.scheduler.engine.cplusplus.generator.util

import java.net.JarURLConnection
import java.net.URL
import scala.collection.JavaConversions._

object PackageOps {
  private val classFilenameRegex = """^(.*)\.class""".r

  def classesOfPackage(packageName: String, classNameFilter: String ⇒ Boolean) =
    classNamesOfPackage(packageName) filter classNameFilter map classForName

  def classNamesOfPackage(packageName: String): List[String] = {
    require(packageName.nonEmpty, "Non-root package name required")
    val packagePath = packageName.replace('.', '/')
    val urls = Thread.currentThread.getContextClassLoader.getResources(packagePath).toList
    val entryNames = urls flatMap entryNamesOfUrl
    entryNames flatMap classNameOfEntryName filter { _.startsWith(packageName + ".")}
  }

  def entryNamesOfUrl(url: URL) = url.openConnection() match {
    case a: JarURLConnection => (a.getJarFile.entries map { _.getName }).toList
  //case a: FileURLConnection.. => new File(a.xx).listNames ... rekursiv durch die Unterverzeichnisse
    case _ => Nil
  }

  def classNameOfEntryName(entryName: String) = entryName match {
    case classFilenameRegex(className) => Some(className.replace('/', '.'))
    case _ => None
  }

  def classForName(name: String) =
    try Class.forName(name)
    catch {
      case t: ClassNotFoundException ⇒
        val u = new ClassNotFoundException(s"${t.getMessage} [class $name]", t)
        u.setStackTrace(t.getStackTrace)
        throw u
      case t: ExceptionInInitializerError ⇒
        val u = new RuntimeException(s"${t.getMessage} [class $name]", t)
        u.setStackTrace(t.getStackTrace)
        throw u
      case t: LinkageError ⇒
        val u = new LinkageError(s"${t.getMessage} [class $name]", t)
        u.setStackTrace(t.getStackTrace)
        throw u
    }
}
