package com.sos.scheduler.engine.kernel.util

import com.google.common.io.Resources.getResource
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import org.joda.time.DateTime
import org.joda.time.format.ISODateTimeFormat

final class MavenProperties(resourcePath: String) {

  private val properties: java.util.Properties =
    autoClosing(getResource(resourcePath).openStream()) { in ⇒
      val p = new java.util.Properties
      p.load(in)
      p
    }

  override def toString = s"$groupId:$artifactId-$version $branchAndCommitSuffix"

  def groupId = asString("project.groupId")

  def artifactId = asString("project.artifactId")

  def buildVersion: String = {
    var result = version
    if (version endsWith "-SNAPSHOT") result += s" $branchAndCommitSuffix"
    result
  }

  private def branchAndCommitSuffix = "(" +
    (List("branch", versionBranch, versionCommitHash) filter { _.nonEmpty } mkString " ") +
    s", built $buildDateTime)"

  def version: String =
    asString("project.version") match {
      case "${project.version}" ⇒ "(IDE development)"
      case o ⇒ o
    }

  def versionCommitHash = asString("sourceVersion.commitHash")

  def versionBranch = asString("sourceVersion.branch")

  def buildDateTime: DateTime =
    ISODateTimeFormat.dateTime.parseDateTime(asString("maven.build.timestamp"))

  private def asString(name: String): String =
    Option(properties.getProperty(name)) getOrElse { throw new NoSuchElementException(s"Unknown property '$name'") }
}
