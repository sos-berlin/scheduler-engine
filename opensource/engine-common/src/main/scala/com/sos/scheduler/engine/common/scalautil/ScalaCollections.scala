package com.sos.scheduler.engine.common.scalautil

import scala.sys._

object ScalaCollections {
  implicit class RichTraversable[A](val delegate: Traversable[A]) extends AnyVal {

    def requireDistinct[K](key: A => K) = {
      (duplicates(key)) match {
        case o if o.nonEmpty => error("Unexpected duplicates: "+ o.keys.mkString(", "))
        case _ =>
      }
      delegate
    }

    /** Liefert die Duplikate, also Listenelemente, deren Schlüssel mehr als einmal vorkommt. */
    def duplicates[K](key: A => K) =
      delegate groupBy key filter { _._2.size > 1 }
  }
}
