package com.sos.scheduler.engine.cplusplus.generator.util

import java.net.JarURLConnection
import java.net.URL
import scala.collection.JavaConversions._


object PackageOps
{
    private val classFilenameRegex = """^(.*)\.class""".r

    def classesOfPackage(packageName: String) = classNamesOfPackage(packageName) map Class.forName
    
    def classNamesOfPackage(packageName: String): List[String] = {
        require(packageName.nonEmpty, "Non-root package name required")
        val packagePath = packageName.replace('.', '/')
        val urls = Thread.currentThread.getContextClassLoader.getResources(packagePath).toList
        val entryNames = urls flatMap entryNamesOfUrl
        entryNames flatMap classNameOfEntryName filter { _.startsWith(packageName + ".")}
    }
    
    def entryNamesOfUrl(url: URL) = url.openConnection() match {
        case a: JarURLConnection => a.getJarFile.entries map { _.getName } toList
      //case a: FileURLConnection.. => new File(a.xx).listNames ... rekursiv durch die Unterverzeichnisse
        case _ => Nil
    }

    def classNameOfEntryName(name: String) = name match {
        case classFilenameRegex(name) => Some(name.replace('/', '.'))
        case _ => None
    }
}
