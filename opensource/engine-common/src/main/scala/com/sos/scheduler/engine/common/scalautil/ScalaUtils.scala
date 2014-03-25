package com.sos.scheduler.engine.common.scalautil

import scala.collection.immutable
import scala.reflect.ClassTag
import scala.collection.JavaConversions._

object ScalaUtils {
  // Warum ist das nicht in Scala enthalten?
  def implicitClass[A : ClassTag]: Class[A] =
    implicitly[ClassTag[A]].runtimeClass.asInstanceOf[Class[A]]

  implicit class RichTraversableOnce[A](val delegate: TraversableOnce[A]) extends AnyVal {
    def toImmutableSeq: immutable.Seq[A] =
      delegate match {
        case o: immutable.Seq[A] ⇒ o
        case _ ⇒ Vector() ++ delegate
      }

    def countEquals: Map[A, Int] =
      delegate.toTraversable groupBy identity map { case (k, v) ⇒ k -> v.size }

    def toKeyedMap[K](toKey: A ⇒ K): Map[K, A] =
      (delegate map { o ⇒ toKey(o) -> o }).toMap
  }

  implicit class RichArray[A](val delegate: Array[A]) extends AnyVal {
    def toImmutableSeq: immutable.Seq[A] =
      Vector() ++ delegate
  }

  implicit class RichJavaIterable[A](val delegate: java.lang.Iterable[A]) extends AnyVal {
    def toImmutableSeq: immutable.Seq[A] =
      Vector() ++ delegate
  }

  implicit class RichJavaIterator[A](val delegate: java.util.Iterator[A]) extends AnyVal {
    def toImmutableSeq: immutable.Seq[A] =
      Vector() ++ delegate
  }
}
