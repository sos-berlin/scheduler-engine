package com.sos.scheduler.engine.common.async

import java.util.concurrent.Callable
import org.joda.time.format.ISODateTimeFormat
import org.slf4j.LoggerFactory
import scala.util.control.NonFatal
import scala.util.{Success, Try, Failure}

trait TimedCall[A] extends Callable[A] {

  import TimedCall._

  def at: Long

  def call(): A

  final def apply() {
    val result = Try(call())
    try onComplete(result)
    catch { case NonFatal(t) => logger.error(s"Error in onComplete() ignored: $t ($toString)", t) }
  }

  protected def onComplete(o: Try[A]) {
    o match {
      case Success(p) =>
      case Failure(t) => logger.error(s"Error in asynchronous operation ignored: $t ($toString)", t)
    }
  }

  override def toString = Seq(toStringPrefix, s"at=$atString") mkString " "

  final def atString = if (at == shortTerm) "short-term" else ISODateTimeFormat.dateTime.print(at)

  def toStringPrefix = getClass.getSimpleName
}

object TimedCall {
  private[async] val shortTerm = 0L
  private val logger = LoggerFactory.getLogger(classOf[TimedCall[_]])

  def apply[A](at: Long)(f: => A) = new AbstractTimedCall[A](at){
    def call() = f
  }
}
