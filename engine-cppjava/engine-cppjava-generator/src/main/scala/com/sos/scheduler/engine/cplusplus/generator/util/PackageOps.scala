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

  private def entryNamesOfUrl(url: URL) = url.openConnection() match {
    case o: JarURLConnection ⇒ entryNamesOfJar(o)
  //case a: FileURLConnection.. => new File(a.xx).listNames ... rekursiv durch die Unterverzeichnisse
    case _ => Nil
  }

  private def entryNamesOfJar(jarURLConnection: JarURLConnection) = {
    // Since deployment of JobScheduler v1.8-RC1, "mvn deploy" fails with java.io.FileNotFoundException: JAR entry com/sos/scheduler/engine not found in /home/jenkins/.m2/repository/com/sos/scheduler/engine/engine-common/1.8.0/engine-common-1.8.0.jar
    // So we open the jar URL with root entry "/", and then the needless JarURLConnection.entryName is null.
    val rootEntryJarUrl = new URL(s"jar:${jarURLConnection.getJarFileURL}!/")
    (rootEntryJarUrl.openConnection().asInstanceOf[JarURLConnection].getJarFile.entries map { _.getName }).toList
  }

  def classNameOfEntryName(entryName: String) = entryName match {
    case classFilenameRegex(className) => Some(className.replace('/', '.'))
    case _ => None
  }

  def classForName(name: String) =
    try Class.forName(name, /*initialize=*/false, getClass.getClassLoader)
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
