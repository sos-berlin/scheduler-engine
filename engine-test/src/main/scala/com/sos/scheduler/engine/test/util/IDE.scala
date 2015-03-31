package com.sos.scheduler.engine.test.util

object IDE {
  def isRunningUnderIDE: Boolean = {
    (sys.props contains "jobscheduler.ide") || {
      val mainClassName = sys.props.getOrElse("sun.java.command", "")
      mainClassName.startsWith("com.intellij.") || mainClassName.startsWith("org.jetbrains.")
    }
  }

//  Method according to http://stackoverflow.com/questions/8417524
//  import java.lang.management.ManagementFactory
//  import scala.collection.JavaConversions._
//  val isRunningUnderDebugger: Boolean = ManagementFactory.getRuntimeMXBean.getInputArguments exists { _ startsWith "-agentlib" }
}
