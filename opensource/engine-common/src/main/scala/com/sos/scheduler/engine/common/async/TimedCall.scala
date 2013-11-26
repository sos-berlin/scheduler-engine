package com.sos.scheduler.engine.common.async

import TimedCall._
import com.sos.scheduler.engine.common.scalautil.Logger
import java.util.concurrent.Callable
import org.joda.time.Instant
import org.joda.time.format.ISODateTimeFormat
import scala.util.control.NonFatal
import scala.util.{Success, Try, Failure}

trait TimedCall[A] extends Callable[A] {

  def epochMillis: Long

  def call(): A

  final def instant =
    new Instant(epochMillis)

  private[async] final def onApply() {
    logger debug s"Calling $toString"
    val result = Try(call())
    callOnComplete(result)
  }

  private[async] final def onCancel() {
    logger debug s"Cancel $toString"
    callOnComplete(Failure(CancelledException()))
  }

  private def callOnComplete(o: Try[A]) {
    try onComplete(o)
    catch { case NonFatal(t) => logger.error(s"Error in onComplete() ignored: $t ($toString)", t) }
  }

  protected def onComplete(o: Try[A]) {
    o match {
      case Success(p) =>
      case Failure(e: CancelledException) => logger.debug(s"TimedCall '$toString' cancelled")
      case Failure(t) => logger.error(s"Error in TimedCall ignored: $t ($toString)", t)
    }
  }

  override def toString =
    Seq(
      Some(Try(toStringPrefix) getOrElse "toString error"),
      Some(s"at=$atString") filter { _ => epochMillis != shortTermMillis })
    .flatten mkString " "

  final def atString =
    if (epochMillis == shortTermMillis) "short-term"
    else ISODateTimeFormat.dateTime.print(epochMillis)

  def toStringPrefix =
    getClass.getSimpleName
}

object TimedCall {
  /** So bald wie möglich. */
  private[async] val shortTermMillis = 0
  /** So bald wie möglich. */
  private[async] val shortTerm = new Instant(0)
  private val logger = Logger(getClass)

  def apply[A](at: Instant)(f: => A) = new TimedCall[A] {
    def epochMillis = at.getMillis
    def call() = f
  }

  final case class CancelledException protected[async]() extends RuntimeException
}
