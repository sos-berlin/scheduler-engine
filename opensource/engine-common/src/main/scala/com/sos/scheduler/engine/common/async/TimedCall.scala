package com.sos.scheduler.engine.common.async

import java.util.concurrent.Callable
import org.joda.time.Instant
import org.joda.time.format.ISODateTimeFormat
import org.slf4j.LoggerFactory
import scala.util.control.NonFatal
import scala.util.{Success, Try, Failure}
import com.sos.scheduler.engine.common.scalautil.Logger

trait TimedCall[A] extends Callable[A] {

  import TimedCall._

  def instant = new Instant(epochMillis)

  def epochMillis: Long

  def call(): A

  final def apply() {
    logger debug s"Calling $toString"
    val result = Try(call())
    try onComplete(result)
    catch { case NonFatal(t) => logger.error(s"Error in onComplete() ignored: $t ($toString)", t) }
  }

  protected def onComplete(o: Try[A]) {
    o match {
      case Success(p) =>
      case Failure(t) => logger.error(s"Error in TimedCall ignored: $t ($toString)", t)
    }
  }

  override def toString =
    Seq(
      Some(Try(toStringPrefix) getOrElse "toString error"),
      Some(s"at=$atString") filter { _ => epochMillis != shortTermMillis })
    .flatten mkString " "

  final def atString = if (epochMillis == shortTermMillis) "short-term" else ISODateTimeFormat.dateTime.print(epochMillis)

  def toStringPrefix = getClass.getSimpleName
}

object TimedCall {
  private[async] val shortTermMillis = 0
  private[async] val shortTerm = new Instant(0)
  private val logger = Logger(getClass)

  def apply[A](at: Instant)(f: => A) = new TimedCall[A] {
    def epochMillis = at.getMillis
    def call() = f
  }
}
