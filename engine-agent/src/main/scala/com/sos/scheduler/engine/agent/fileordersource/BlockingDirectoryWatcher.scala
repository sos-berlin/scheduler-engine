package com.sos.scheduler.engine.agent.fileordersource

import com.sos.scheduler.engine.agent.fileordersource.BlockingDirectoryWatcher._
import com.sos.scheduler.engine.common.scalautil.AutoClosing.closeOnError
import com.sos.scheduler.engine.common.scalautil.{HasCloser, Logger}
import com.sos.scheduler.engine.common.time.ScalaTime._
import java.nio.file.StandardWatchEventKinds._
import java.nio.file.{FileSystems, Path, WatchEvent}
import java.time.Instant
import java.time.Instant.now
import java.util.concurrent.TimeUnit.MILLISECONDS
import scala.collection.JavaConversions._
import scala.concurrent._

/**
 * @author Joacim Zschimmer
 *
 * @param pathMatches Predicate for a `Path` resolved against `directory`
 */
private[fileordersource] final class BlockingDirectoryWatcher(directory: Path, pathMatches: Path ⇒ Boolean)
extends HasCloser {
  private val logPrefix = s"Watching directory $directory"

  logger.trace(s"$logPrefix FileSystems.getDefault.newWatchService()")
  private val watchService = FileSystems.getDefault.newWatchService()
  onClose {
    watchService.close()
    logger.trace(s"$logPrefix close: watchService.close() called")
  }

  private val registeredWatchKey = closeOnError(closer) {
    logger.trace(s"$logPrefix directory.register(watchService, ENTRY_CREATE)")
    directory.register(watchService, ENTRY_CREATE)
  }
  onClose {
    // Paranoid?
    registeredWatchKey.cancel()
    logger.trace(s"$logPrefix close: registeredWatchKey.cancel() called")
  }

  def waitForMatchingDirectoryChange(until: Instant): Unit =
    while (!waitForNextChange(until)) {}

  /**
   * Waits until any directory change.
   *
   * @return true, iff Path matches `pathMatches` or the event OVERFLOW has occurred or the time is over.
   */
  def waitForNextChange(until: Instant): Boolean = {
    val remainingMillis = (until - now()).toMillis
    remainingMillis <= 0 || {
      val watchKey = blocking {
        logger.trace(s"$logPrefix watchService.poll($remainingMillis, MILLISECONDS), blocking")
        watchService.poll(remainingMillis, MILLISECONDS)
      }
      if (watchKey == null) {
        logger.trace(s"$logPrefix, expired")
        false
      } else
        try
          watchKey.pollEvents().asInstanceOf[java.util.List[WatchEvent[Path]]] exists { event ⇒
            logger.trace(s"$logPrefix, event ${event.kind} ${event.context}")
            val result = event.kind == OVERFLOW || pathMatches(directory resolve event.context)
            logger.trace(s"event.kind=${event.kind} result=$result")
            result
          }
        finally {
          watchKey.reset()
          logger.trace(s"$logPrefix watchKey.reset() called")
        }
    }
  }
}

object BlockingDirectoryWatcher {
  private val logger = Logger(getClass)
}
