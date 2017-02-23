package com.sos.scheduler.engine.plugins.newwebservice.routes.log

import akka.actor.{Actor, ActorRef}
import akka.spray._
import akka.util.ByteString
import com.sos.jobscheduler.base.exceptions.StandardPublicException
import com.sos.jobscheduler.common.log.LazyScalaLogger.AsLazyScalaLogger
import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.jobscheduler.common.scalautil.{Logger, SetOnce}
import com.sos.jobscheduler.common.utils.Exceptions._
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.schedulerThreadFuture
import com.sos.scheduler.engine.kernel.log.{LogSubscription, PrefixLog}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.defaultEncoding
import com.sos.scheduler.engine.plugins.newwebservice.routes.log.LogActor._
import java.io.{BufferedInputStream, InputStreamReader, Reader}
import java.nio.CharBuffer
import java.nio.file.Files.newInputStream
import java.nio.file.Path
import java.util.concurrent.atomic.AtomicBoolean
import scala.concurrent.ExecutionContext
import spray.can.Http
import spray.http.{ChunkedMessageEnd, ContentType, HttpData, HttpEntity, MessageChunk}
import spray.httpx.marshalling.MarshallingContext

/**
  * @author Joacim Zschimmer
  */
private[log] final class LogActor(prefixLog: PrefixLog, contentType: ContentType, marshallingContext: MarshallingContext)
  (implicit schedulerThreadCallQueue: SchedulerThreadCallQueue, executionContext: ExecutionContext)
extends Actor {

  // This Actor holds the file open. Under Windows, this can the inhibit the deletion of the log file !!!

  import context.stop
  private var responder: ActorRef = null
  private var marshallingEnded = false
  private var throwableOnce = new SetOnce[Throwable]
  private val expectingChunk = new AtomicBoolean
  private var reader = new SetOnce[Reader]
  private val charBuffer = CharBuffer.allocate(100000)

  private val logSubscription = new LogSubscription {
    def onStarted() = {}

    def onClosed() = logException(logger.asLazy.warn) { self ! End }   // File should be closed now to allow JobScheduler on Windows to close it !!!

    def onLogged() = logException(logger.asLazy.warn) { signalChunk() }
  }

  override def preStart() = {
    self ! Start
    super.preStart()
  }

  override def preRestart(throwable: Throwable, message: Option[Any]): Unit = {
    throwableOnce := throwable
    stop(self)  // Not restartable
    super.preRestart(throwable, message)
  }

  override def postStop() = {
    prefixLog.unsubscribe(logSubscription)
    for (r ← reader) r.close()
    if (!marshallingEnded) {
      if (responder == null) {
        marshallingContext.handleError(throwableOnce getOrElse { new StandardPublicException("Error while reading log") })
      } else {
        logException(logger.asLazy.debug) {  // Jetty's AsyncContinuation
          responder ! ChunkedMessageEnd
        }
      }
    }
    super.postStop()
  }

  def receive = {
    case Start ⇒
      (for (file ← schedulerThreadFuture { prefixLog.file }) yield
        self ! UseFile(file))
      .failed foreach { t ⇒
        logger.warn(t.toString, t)
        stop(self)
      }

    case End ⇒
      if (responder == null) {
        marshallingContext.marshalTo(HttpEntity(contentType, ByteString.empty))
      } else {
        responder ! ChunkedMessageEnd
      }
      marshallingEnded = true
      stop(self)

    case UseFile(file) ⇒
      if (file.toString.isEmpty) {
        throw new StandardPublicException("No log file")
      } else {
        reader := new InputStreamReader(new BufferedInputStream(newInputStream(file)), defaultEncoding)
        prefixLog.subscribe(logSubscription)
        self ! SendNextChunk
      }

    case SendNextChunk ⇒
      expectingChunk.set(true)
      for (chunk ← readChunk()) {
        if (responder == null) {
          responder = marshallingContext.startChunkedMessage(HttpEntity(contentType, chunk), ack = Some(SendNextChunk))(self)
        } else {
          responder ! MessageChunk(HttpData(chunk)).withAck(SendNextChunk)
        }
      }

    case _: Http.ConnectionClosed ⇒
      marshallingEnded = true
      stop(self)
  }

  private def signalChunk(): Unit = {
    if (expectingChunk.getAndSet(false)) {
      self ! SendNextChunk
    }
  }

  private def readChunk(): Option[ByteString] = {
    charBuffer.clear()
    reader().read(charBuffer) match {
      case -1 | 0 ⇒
        expectingChunk.set(true)
        None
      case len ⇒
        charBuffer.flip()
        val byteBuffer = defaultEncoding.encode(charBuffer)
        Some(createByteStringUnsafe(byteBuffer.array, byteBuffer.position, byteBuffer.limit))
    }
  }
}

private[log] object LogActor {
  private val logger = Logger(getClass)

  private case object Start
  private case object End
  private case class UseFile(file: Path)
  private case object SendNextChunk
}
