package com.sos.scheduler.engine.plugins.newwebservice.routes

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.FileUtils.deleteDirectoryRecursively
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.sprayutils.JsObjectMarshallers._
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.folder.FolderSubsystemClient
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.LiveRoute._
import java.nio.file.Files.{createDirectories, exists, isDirectory, move}
import java.nio.file.StandardCopyOption.ATOMIC_MOVE
import java.nio.file.{Files, NoSuchFileException, Path, Paths}
import scala.collection.JavaConversions._
import scala.concurrent.ExecutionContext
import spray.http.ContentTypes.`application/octet-stream`
import spray.http.StatusCodes.{BadRequest, Conflict, Created, Forbidden, NotFound, OK}
import spray.http.{HttpEntity, Uri}
import spray.json._
import spray.routing.Directives._
import spray.routing.{ExceptionHandler, Route}

// Test in JS1858IT
trait LiveRoute
{
  protected implicit def client: DirectSchedulerClient
  protected def folderSubsystem: FolderSubsystemClient
  protected implicit def webServiceContext: WebServiceContext
  protected def schedulerConfiguration: SchedulerConfiguration
  protected implicit def executionContext: ExecutionContext
  protected implicit def actorRefFactory: ActorRefFactory

  private lazy val live: Path = schedulerConfiguration.localConfigurationDirectory.toPath.toRealPath()

  protected final def liveRoute: Route =
    unmatchedPath { uriPath =>
      val pathString = rebuildPath(uriPath)
      if (!pathString.startsWith("/"))
        complete(BadRequest)  // Reserved URI
      else if (!isAllowedPath(pathString))
        complete(Forbidden)
      else
        handleExceptions(noSuchFileExceptionHandler) {
          val file = live.resolve(pathString.stripPrefix("/").stripSuffix("/")).normalize
          if (!file.startsWith(live) || file.endsWith(pathString))  // Double check
            complete(Forbidden)
          else if (pathString endsWith "/")
            directoryRoute(file)
          else
            fileRoute(file)
        }
    }

  private def directoryRoute(directory: Path): Route =
    get {
      dynamic {
        detach(()) {
          if (!exists(directory))
            complete(NotFound)
          else if (!isDirectory(directory))
            complete(BadRequest -> "Not a directory")
          else
            complete(
              autoClosing(
                Files.list(directory))(stream =>
                  JsArray(
                    asScalaIterator(stream.iterator)
                      .map(o => JsString(
                        o.getFileName.toString + (if (isDirectory(o)) "/" else "")))
                      .toVector)))
        }
      }
    } ~
    put {
      dynamic {
        detach(()) {
          entity(as[HttpEntity]) { httpEntity =>
            if (!exists(live))
              complete(NotFound)  // Do not create live directory (just in case)
            else if (!httpEntity.isEmpty)
              complete(BadRequest -> "A directory may only be created without content")
            else if (isDirectory(directory))
              complete(OK)
            else if (exists(directory))
              complete(Conflict -> "File exists")
            else {
              createDirectories(directory)
              complete(Created)
            }
          }
        }
      }
    } ~
    delete {
      dynamic {
        detach(()) {
          if (!exists(directory))
            complete(NotFound)
          else if (!isDirectory(directory))
            complete(BadRequest -> "Not a directory")
          else {
            logger.info(s"Web service deletes directory recursively: $directory")
            deleteDirectoryRecursively(directory, file => logger.info(s"Delete $file"))
            folderSubsystem.updateFolders()
            complete(OK)
          }
        }
      }
    }

  private def fileRoute(file: Path): Route =
    get {
      dynamic {
        if (!exists(file))
          complete(NotFound)
        else
          getFromFile(file, `application/octet-stream`)
      }
    } ~
    put {
      dynamic {
        detach(()) {
          entity(as[HttpEntity]) { httpEntity =>
            if (!exists(live))
              complete(NotFound)  // Do not create live directory (just in case)
            else {
              val existed = exists(file)
              logger.info(s"Web service writes file $file")
              createDirectories(file.getParent)
              val tmp = Paths.get(s"$file~")
              tmp.contentBytes = httpEntity match {
                case HttpEntity.Empty => Array.emptyByteArray
                case HttpEntity.NonEmpty(_, data) => data.toByteArray
              }
              move(tmp, file, ATOMIC_MOVE)
              folderSubsystem.updateFolders()
              complete(if (existed) OK else Created)
            }
          }
        }
      }
    } ~
    delete {
      dynamic {
        detach(()) {
          if (isDirectory(file))
            complete(BadRequest -> "File is a directory")
          else {
            logger.info(s"Web service deletes file $file")
            Files.delete(file)
            folderSubsystem.updateFolders()
            complete(OK)
          }
        }
      }
    }
}

object LiveRoute
{
  private val logger = Logger(getClass)

  private[routes] def rebuildPath(path: Uri.Path): String = {
    val sb = new StringBuilder
    var p = path
    while (!p.isEmpty) {
      p match {
        case Uri.Path.Slash(tail) =>
          sb += '/'
          p = tail
        case Uri.Path.Segment(segment, tail) =>
          sb ++= segment
          p = tail
        case Uri.Path.Empty =>
          throw new IllegalStateException
      }
    }
    sb.toString
  }

  private[routes] def isAllowedPath(path: String): Boolean =
    path.startsWith("/") &&
      !path.contains("//") &&
      !path.contains("/../") && !path.endsWith("/..") &&
      !path.contains("/./") && !path.endsWith("/.") &&
      !path.contains('\\') &&
      !path.contains('\0')

  private val noSuchFileExceptionHandler = ExceptionHandler {
    case t: NoSuchFileException => complete(NotFound)
  }

  object test {
    val forbiddenPaths = Vector(
      "",
      "a",
      "//",
      "/a//",
      "/a//b",
      "/..",
      "/../a",
      "/a/..",
      "/a/../",
      "/a/../b",
      "/a/.",
      "/a/./",
      "/a/./b",
      "\\",
      "/a\\",
      "/a\\b",
      "/null-byte\0/b")

    val allowedPaths = Vector(
      "/",
      "/a",
      "/a/",
      "/a/b",
      "/a/b/",
      "/.a",
      "/.a/.b")
  }
}
