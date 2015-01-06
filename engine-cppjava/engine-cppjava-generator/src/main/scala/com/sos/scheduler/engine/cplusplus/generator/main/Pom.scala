//package com.sos.scheduler.engine.cplusplus.generator.main
//
//import com.sos.scheduler.engine.cplusplus.scalautil.io.Util._
//import java.util.Properties
//
//pom.properties steht im Test nicht bereit, erst bei install im Jar.
//
//class Pom(projectName: String, artifactName: String) {
//    lazy val version = pomProperties.get("version").asInstanceOf[String]
//
//    lazy val pomProperties = {
//        val resourcePath = "/META-INF/maven/" + projectName + "/" + artifactName + "/pom.properties"
//        val result = new Properties
//        closingFinally(openResource(resourcePath)) { in => result.load(in) }
//        result
//    }
//
//    private def openResource(path: String) = Option(getClass.getResourceAsStream(path)) getOrElse {
//        throw new RuntimeException("Missing Java resource '" + path + "'")
//    }
//
//    override def toString = projectName + "/" + artifactName + " (" + version + ")"
//}
