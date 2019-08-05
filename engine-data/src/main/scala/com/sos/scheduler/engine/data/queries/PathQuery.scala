package com.sos.scheduler.engine.data.queries

import com.sos.scheduler.engine.base.serial.PathAndParameterSerializable
import com.sos.scheduler.engine.base.sprayjson.SprayJson.implicits.RichJsValue
import com.sos.scheduler.engine.data.filebased.{TypedPath, UnknownTypedPath}
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.data.queries.PathQuery._
import scala.collection.immutable.Seq
import scala.language.implicitConversions
import spray.json.{JsArray, JsObject, JsString, JsValue, RootJsonFormat}

/**
  * @author Joacim Zschimmer
  */
sealed trait PathQuery {

  final def matchesAnyType(path: TypedPath): Boolean =
    path match {
      case o: FolderPath ⇒ matches(o)
      case o ⇒ matches(o.asTyped[UnknownTypedPath])
    }

  /** PathQuery may apply to any `TypedPath`. */
  def matches[P <: TypedPath: TypedPath.Companion](path: P): Boolean

  def matchesAll = false

  def withRecursive(recursive: Boolean): PathQuery =
    this match {
      case o: FolderTree if !recursive ⇒ FolderOnly(o.folderPath)
      case o: FolderOnly if recursive ⇒ FolderTree(o.folderPath)
      case o ⇒ o
    }

  @deprecated("Does not work properly for PathQuery.Multiply")
  def typedPath[P <: TypedPath: TypedPath.Companion]: TypedPath

  def folderPath: FolderPath

  def toUriPath: String

  def toPathAndParameters[P <: TypedPath: TypedPath.Companion]: (String, Map[String, String]) =
    PathQuery.pathAndParameterSerializable.toPathAndParameters(this)
}

object PathQuery {
  val All = FolderTree(FolderPath.Root)

  def apply[P <: TypedPath: TypedPath.Companion](pattern: String): PathQuery =
    if (pattern endsWith "/*")
      FolderOnly(FolderPath.fromTrailingSlash(pattern stripSuffix "*"))  // P is ignored. It's a FolderPath denoting a subtree of P
    else
    if (pattern endsWith "/")
      FolderTree(FolderPath.fromTrailingSlash(pattern))  // P is ignored. It's a FolderPath denoting a subtree of P
    else {
      val checkedTypedPath = implicitly[TypedPath.Companion[P]].apply(pattern)
      SinglePath(checkedTypedPath.string)
    }

  def apply(path: FolderPath, isRecursive: Boolean = true) = Folder(path, isRecursive)

  implicit def apply(path: TypedPath): PathQuery =
    path match {
      case o: FolderPath ⇒ FolderTree(o)
      case o: TypedPath ⇒ SinglePath(o.string)
    }

  def fromUriPath[A <: TypedPath: TypedPath.Companion](path: String): PathQuery =
    if (path contains '|')
      Multiple(path.split('|').map(PathQuery.apply[A]).toVector)
    else
      apply[A](path)

  sealed trait Folder extends PathQuery {
    def isRecursive: Boolean
  }

  object Folder {
    def apply(folder: FolderPath, isRecursive: Boolean) = if (isRecursive) FolderTree(folder) else FolderOnly(folder)
    def unapply(o: Folder): Option[(FolderPath, Boolean)] = Some((o.folderPath, o.isRecursive))
  }

  final case class FolderTree(folderPath: FolderPath) extends Folder {
    def toUriPath = folderPath.withTrailingSlash
    def isRecursive = true
    override val matchesAll = folderPath == FolderPath.Root
    def matches[P <: TypedPath: TypedPath.Companion](path: P) = matchesAll || (folderPath isAncestorOf path)
    def typedPath[Ignored <: TypedPath: TypedPath.Companion] = folderPath
  }

  final case class FolderOnly(folderPath: FolderPath) extends Folder {
    def toUriPath = folderPath.withTrailingSlash + "*"
    def isRecursive = false
    def matches[P <: TypedPath: TypedPath.Companion](path: P) = folderPath isParentOf path
    def typedPath[Ignored <: TypedPath: TypedPath.Companion] = folderPath
  }

  final case class SinglePath(pathString: String) extends PathQuery {
    def toUriPath = pathString
    def isRecursive = false
    val folderPath = FolderPath.parentOf(UnknownTypedPath(pathString))
    def matches[P <: TypedPath: TypedPath.Companion](path: P) = path == as[P]
    def typedPath[P <: TypedPath: TypedPath.Companion]: TypedPath = as[P]
    def as[P <: TypedPath: TypedPath.Companion]: P = implicitly[TypedPath.Companion[P]].apply(pathString)
  }

  final case class Multiple(pathQueries: Seq[PathQuery]) extends PathQuery
  {
    require(pathQueries.nonEmpty, "PathQuery.Multiple must contain a path")

    def toUriPath = pathQueries.map(_.toUriPath).mkString("|")

    def matches[P <: TypedPath : TypedPath.Companion](path: P) =
      pathQueries exists (_ matches path)

    def typedPath[P <: TypedPath: TypedPath.Companion]: TypedPath =
      implicitly[TypedPath.Companion[P]].apply("/")  // Wrong, but not used

    def folderPath = {
      var shortest: Vector[String] = null
      for (segments <- pathQueries.map(_.folderPath.string.split("/").dropWhile(_.isEmpty).toVector)) {
        if (shortest == null) {
          shortest = segments
        } else {
          val n = shortest zip segments count { case (a, b) => a == b }
          shortest = shortest.take(n)
        }
      }
      FolderPath("/" + shortest.mkString("/"))
    }
  }

  def jsonFormat[P <: TypedPath: TypedPath.Companion]: RootJsonFormat[PathQuery] =
    new RootJsonFormat[PathQuery] {
      def write(q: PathQuery) =
        q match {
          case All =>
            JsObject()
          case Multiple(queries) if queries.size > 1 =>
            JsObject(Map("paths" → JsArray(queries.map(o => JsString(o.toUriPath)).toVector)))
          case _ =>
            JsObject(Map("path" → JsString(q.toUriPath)))
      }

      def read(json: JsValue) =
        json.asJsObject.fields.get("paths") match {
          case Some(paths) ⇒
            val a = paths.asJsArray
            if (a.elements.length == 1)
              PathQuery[P](a.elements.head.asJsString.value)
            else
              Multiple(a.elements.map(o => PathQuery[P](o.asJsString.value)))
          case None ⇒
            json.asJsObject.fields.get("path") match {
              case Some(path) ⇒ PathQuery[P](path.asJsString.value)
              case None ⇒ PathQuery.All
            }
        }
  }

  final def pathAndParameterSerializable[P <: TypedPath: TypedPath.Companion] =
    new PathAndParameterSerializable[PathQuery] {
      def toPathAndParameters(q: PathQuery) = q.toUriPath → Map()

      def fromPathAndParameters(pathAndParameters: (String, Map[String, String])) = {
        val (path, _) = pathAndParameters
        PathQuery.fromUriPath[P](path)
      }
    }
}
