package com.sos.scheduler.engine.common.scalautil

/** Wie java try(AutoClosable), aber für alle Klassen mit close().
  * @author Joacim Zschimmer */
object AutoClosing {

  private type HasClose = { def close() }

  private val logger = Logger(getClass)

  /** Wie Java 7 try-with-resource */
  def autoClosing[A <: HasClose with AnyRef, B](resource: A)(f: A => B): B = {
    if (resource eq null)  throw new NullPointerException("closingFinally: object is null")
    var closeFinally = true
    try f(resource)
    catch {
      case t: Throwable =>
        try resource.close()
        catch {
          case tt: Throwable => logger.error(s"Ignoring double exception while handling exception: $tt", tt)   // Java 7 stellt Throwable.addSuppressed() bereit
        }
        closeFinally = false
        throw t
    }
    finally if (closeFinally)
      resource.close()
  }
}
