package com.sos.scheduler.engine.plugins.newwebservice.routes.cpp

/**
  * @author Joacim Zschimmer
  */
import akka.actor._
import akka.util.ByteString
import com.sos.jobscheduler.base.exceptions.StandardPublicException
import com.sos.jobscheduler.common.log.LazyScalaLogger.AsLazyScalaLogger
import com.sos.jobscheduler.common.scalautil.{Logger, SetOnce}
import com.sos.jobscheduler.common.utils.Exceptions.logException
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyInvalidatedException
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.schedulerThreadFuture
import com.sos.scheduler.engine.kernel.cppproxy.HttpChunkReaderC
import com.sos.scheduler.engine.kernel.http.SchedulerHttpResponse
import com.sos.scheduler.engine.plugins.newwebservice.routes.cpp.ChunkingActor._
import scala.concurrent.{ExecutionContext, Promise}
import scala.util.control.NonFatal
import spray.can.Http
import spray.http.{ChunkedMessageEnd, ContentType, HttpEntity, MessageChunk}
import spray.httpx.marshalling.MarshallingContext

private[cpp] class ChunkingActor(
  chunkReaderC: HttpChunkReaderC,
  notifier: Notifier,
  contentType: ContentType,
  marshallingContext: MarshallingContext,
  terminatedPromise: Promise[Unit])
(implicit
  schedulerThreadCallQueue: SchedulerThreadCallQueue,
  executionContext: ExecutionContext)
extends Actor with ActorLogging {
  import context.stop

  private var expectingChunk = true
  private var responder: ActorRef = null
  private var stopped = false
  private var marshallingEnded = false
  private var throwableOnce = new SetOnce[Throwable]

  notifier.onChunkIsReady {
    self ! ChunkIsReady
  }
  mySchedulerThreadFuture {
    if (chunkReaderC.next_chunk_is_ready) {
      self ! ChunkIsReady
    }
  }

  override def postStop() = {
    stopped = true
    terminatedPromise.trySuccess(())
    if (!marshallingEnded) {
      if (responder == null) {
        marshallingContext.handleError(throwableOnce getOrElse { new StandardPublicException("Error while expecting response from JobScheduler (C++)") })
      } else {
        logException(logger.asLazy.debug) {  // Jetty's AsyncContinuation
          responder ! ChunkedMessageEnd
        }
      }
    }
    super.postStop()
  }

  def receive = {
    case ExpectChunk ⇒
      expectingChunk = true

    case ChunkIsReady ⇒
      if (expectingChunk) {
        expectingChunk = false
        readNextChunk()
      }

    case ChunkRead(bytes) ⇒
      if (bytes.nonEmpty) {
        if (responder == null) {
          responder = marshallingContext.startChunkedMessage(HttpEntity(contentType, bytes), ack = Some(RequestChunk))(self)
        } else {
          responder ! MessageChunk(bytes).withAck(RequestChunk)
        }
      } else {
        if (responder == null) {
          marshallingContext.marshalTo(HttpEntity(contentType, ByteString.empty))
        } else {
          responder ! ChunkedMessageEnd
        }
        marshallingEnded = true
        stop(self)
      }

    case RequestChunk ⇒
      readNextChunk()

    case _: Http.ConnectionClosed ⇒
      marshallingEnded = true
      stop(self)
  }

  private def readNextChunk() =
    mySchedulerThreadFuture {
      if (chunkReaderC.next_chunk_is_ready) {
        self ! ChunkRead(chunkReaderC.read_from_chunk(RecommendedChunkSize))
      } else {
        self ! ExpectChunk
      }
    }

  private def mySchedulerThreadFuture(body: ⇒ Unit): Unit =
    schedulerThreadFuture {
      try body
      catch {
        case _: CppProxyInvalidatedException if stopped ⇒
        case NonFatal(t) ⇒
          logger.warn(t.toString)
          throwableOnce.trySet(t)
          stop(self)
          throw t
      }
    }
}

object ChunkingActor {
  private val logger = Logger(getClass)
  private val RecommendedChunkSize = 1000*10

  private[cpp] class Notifier {
    private val callbackOnce = new SetOnce[() ⇒ Unit]

    val schedulerHttpResponse = new SchedulerHttpResponse {
      def onNextChunkIsReady() = for (callback ← callbackOnce) logException(logger.asLazy.warn) { callback() }
    }

    def onChunkIsReady(callback: ⇒ Unit) = callbackOnce := { () ⇒ callback }
  }

  private case object RequestChunk
  private case object ChunkIsReady
  private case object ExpectChunk
  private case class ChunkRead(bytes: Array[Byte])
}
