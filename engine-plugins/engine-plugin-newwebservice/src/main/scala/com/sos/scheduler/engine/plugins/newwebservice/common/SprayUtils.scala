package com.sos.scheduler.engine.plugins.newwebservice.common

import shapeless._
import spray.http.HttpHeaders.Accept
import spray.http.MediaType
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
object SprayUtils {

  object implicits {
    implicit class RichOption[A](val delegate: Option[A]) extends AnyVal {
      def applyRoute(f: A ⇒ Route): Route =
        delegate match {
          case Some(a) ⇒ f(a)
          case None ⇒ reject
        }
    }
  }

  def accept(mediaType: MediaType): Directive0 =
    mapInnerRoute { route ⇒
      headerValueByType[Accept]() {
        case Accept(mediaTypes) if mediaTypes exists { _ matches mediaType } ⇒ route
        case _ ⇒ reject
      }
    }

  def passSome[A](option: Option[A]): Directive1[A] =
    new Directive1[A] {
      def happly(inner: (A :: HNil) ⇒ Route) =
        option match {
          case Some(o) ⇒ inner(o :: HNil)
          case None ⇒ reject
        }
    }

  def passIf(condition: Boolean): Directive0 =
    mapInnerRoute { inner ⇒
      if (condition)
        inner
      else
        reject
    }

/*
  private type ParameterMap = Map[String, String]

  private def eatParameterOption(parameterMap: ParameterMap, key: String) =
    new Directive[ParameterMap :: Option[String] :: HNil] {
      override def happly(inner: (ParameterMap :: Option[String] :: HNil) ⇒ Route) =
        inner((parameterMap - key) :: parameterMap.get(key) :: HNil)
    }

  private def eatParameter(parameterMap: ParameterMap, key: String) =
    new Directive[ParameterMap :: String :: HNil] {
      override def happly(inner: (ParameterMap :: String :: HNil) ⇒ Route) =
        parameterMap.get(key) match {
          case Some(value) ⇒ inner((parameterMap - key) :: value :: HNil)
          case None ⇒ reject
        }
    }
*/
  def emptyParameterMap(parameterMap: Map[String, String]) =
    mapInnerRoute { route ⇒
      if (parameterMap.isEmpty) route
      else reject(ValidationRejection(s"Invalid parameters: ${parameterMap.keys mkString ", "}"))
    }
}
