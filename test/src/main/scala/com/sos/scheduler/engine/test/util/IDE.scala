package com.sos.scheduler.engine.test.util

object IDE {
  def isRunningUnderIDE: Boolean = {
    (sys.props contains "jobscheduler.ide") || {
      val mainClassName = sys.props.getOrElse("sun.java.command", "")
      mainClassName.startsWith("com.intellij.") || mainClassName.startsWith("org.jetbrains.")
    }
  }
}
