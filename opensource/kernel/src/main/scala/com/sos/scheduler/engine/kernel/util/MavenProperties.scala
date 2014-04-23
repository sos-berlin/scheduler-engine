package com.sos.scheduler.engine.kernel.util

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing

final class MavenProperties(classResource: ClassResource) {

  private val properties: java.util.Properties =
    autoClosing(classResource.getInputStream) { in =>
      val p = new java.util.Properties
      p.load(in)
      p
    }

  def groupId: String =
    apply("project.groupId")

  def artifactId: String =
    apply("project.artifactId")

  def buildVersion: String = {
    val v = version
    if (v endsWith "-SNAPSHOT") s"$v (git $versionCommitHash)"
    else v
  }

  def version: String =
    apply("project.version") match {
      case "${project.version}" ⇒ "(IDE development)"
      case o ⇒ o
    }

  def versionCommitHash: String =
    apply("project.versionCommitHash")

  def apply(name: String): String =
    Option(properties.getProperty(name)) getOrElse { throw new NoSuchElementException(s"Unknown property '$name'") }

  override def toString =
    s"$groupId/$artifactId-$version ($versionCommitHash)"
}


object MavenProperties {
  private val resourceSimpleName = "maven.properties"

  def apply(clas: Class[_]): MavenProperties =
    new MavenProperties(new ClassResource(clas, resourceSimpleName))
}
