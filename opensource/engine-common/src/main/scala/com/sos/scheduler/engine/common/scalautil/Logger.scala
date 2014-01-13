package com.sos.scheduler.engine.common.scalautil

import org.slf4j.{Logger => Slf4jLogger, Marker, LoggerFactory}

final class Logger(val delegate: Slf4jLogger) extends AnyVal {

  @inline def error(line: => String) {
    if (delegate.isErrorEnabled)
      delegate.error(line)
  }

  @inline def error(line: => String, t: Throwable) {
    if (delegate.isErrorEnabled)
      delegate.error(line, t)
  }

  @inline def warn(line: => String) {
    if (delegate.isWarnEnabled)
      delegate.warn(line)
  }

  @inline def warn(line: => String, t: Throwable) {
    if (delegate.isWarnEnabled)
      delegate.warn(line, t)
  }

  @inline def debug(line: => String) {
    if (delegate.isDebugEnabled)
      delegate.debug(line)
  }

  @inline def debug(line: => String, t: Throwable) {
    if (delegate.isDebugEnabled)
      delegate.debug(line, t)
  }

  @inline def info(line: => String) {
    if (delegate.isInfoEnabled)
      delegate.info(line)
  }

  @inline def info(line: => String, t: Throwable) {
    if (delegate.isInfoEnabled)
      delegate.info(line, t)
  }

  @inline def info(m: Marker, line: => String) {
    if (delegate.isInfoEnabled(m))
      delegate.info(m, line)
  }

  @inline def info(m: Marker, line: => String, t: Throwable) {
    if (delegate.isInfoEnabled(m))
      delegate.info(m, line, t)
  }

  @inline def trace(line: => String) {
    if (delegate.isTraceEnabled)
      delegate.trace(line)
  }

  @inline def trace(line: => String, t: Throwable) {
    if (delegate.isTraceEnabled)
      delegate.trace(line, t)
  }
}


object Logger {
//  def apply[A <: AnyRef](implicit c: ClassTag[A]): Logger =
//    apply(c.runtimeClass)

  def apply(c: Class[_]) =
    new Logger(LoggerFactory.getLogger(normalizeClassName(c)))

  /** Entfernt das '$' der object-Klasse. */
  private def normalizeClassName(c: Class[_]) =
    c.getName stripSuffix "$"
}
