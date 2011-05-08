package com.sos.scheduler.engine.cplusplus.generator.main

import com.sos.scheduler.engine.cplusplus.scalautil.io.Util._
import java.util.Properties


class MavenProperties(clas: Class[_]) {
    private val resourceSimpleName = "maven.properties"
    def groupId = get("project.groupId")
    def artifactId = get("project.artifactId")
    def version = get("project.version")

    def get(name: String) = getOption(name) getOrElse { throw new RuntimeException("Unknown property '" + name + "'") }

    def getOption(name: String) = Option(properties.get(name).asInstanceOf[String])

    lazy val properties = {
        val resourcePath = clas.getName.split('.').dropRight(1).mkString("/", "/", "/") + resourceSimpleName
        val result = new Properties
        closingFinally(openResource(resourcePath)) { in => result.load(in) }
        result
    }

    private def openResource(path: String) = Option(clas.getResourceAsStream(path)) getOrElse {
        throw new RuntimeException("Missing Java resource '" + path + "'")
    }

    override def toString = groupId + "/" + artifactId + " (" + version + ")"
}
