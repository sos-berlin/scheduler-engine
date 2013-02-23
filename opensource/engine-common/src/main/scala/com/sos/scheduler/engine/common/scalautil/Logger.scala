package com.sos.scheduler.engine.common.scalautil

import org.slf4j.{Logger => Slf4jLogger, Marker, LoggerFactory}
import scala.reflect.ClassTag

final class Logger(val delegate: Slf4jLogger) extends AnyVal {

  @inline def error(f: => Any) {
    if (delegate.isErrorEnabled) delegate.warn(f.toString)
  }

  @inline def error(f: => Any, t: Throwable) {
    if (delegate.isErrorEnabled) delegate.warn(f.toString, t)
  }

  @inline def warn(f: => Any) {
    if (delegate.isWarnEnabled) delegate.warn(f.toString)
  }

  @inline def warn(f: => Any, t: Throwable) {
    if (delegate.isWarnEnabled) delegate.warn(f.toString, t)
  }

  @inline def debug(f: => Any) {
    if (delegate.isDebugEnabled) delegate.debug(f.toString)
  }

  @inline def debug(f: => Any, t: Throwable) {
    if (delegate.isDebugEnabled) delegate.debug(f.toString, t)
  }

  @inline def info(f: => Any) {
    if (delegate.isInfoEnabled) delegate.info(f.toString)
  }

  @inline def info(f: => Any, t: Throwable) {
    if (delegate.isInfoEnabled) delegate.info(f.toString, t)
  }

  @inline def info(m: Marker, f: => Any) {
    if (delegate.isInfoEnabled(m)) delegate.info(m, f.toString)
  }

  @inline def info(m: Marker, f: => Any, t: Throwable) {
    if (delegate.isInfoEnabled(m)) delegate.info(m, f.toString, t)
  }

  @inline def trace(f: => Any) {
    if (delegate.isTraceEnabled) delegate.trace(f.toString)
  }

  @inline def trace(f: => Any, t: Throwable) {
    if (delegate.isTraceEnabled) delegate.trace(f.toString, t)
  }
}

object Logger {
  def apply[A <: AnyRef](implicit c: ClassTag[A]): Logger = apply(c.runtimeClass)

  def apply(c: Class[_]) = new Logger(LoggerFactory.getLogger(normalizeClassName(c)))

  /** Entfernt das '$' der object-Klasse. */
  private def normalizeClassName(c: Class[_]) = {
    val name = c.getName
    val n = name.length - 1
    if (name(n) == '$') name.substring(0, n)
    else name
  }
}