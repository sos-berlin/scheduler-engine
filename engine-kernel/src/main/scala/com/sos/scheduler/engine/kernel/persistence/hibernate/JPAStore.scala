package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.jobscheduler.base.utils.HasKey
import javax.persistence.EntityManager

trait JPAStore[A <: HasKey] {

  def fetch(key: A#Key)(implicit em: EntityManager): A =
    tryFetch(key) getOrElse { throw new NoSuchElementException(s"No record in database for key '$key'")}

  def tryFetch(key: A#Key)(implicit em: EntityManager): Option[A]

  def insert(o: A)(implicit em: EntityManager): Unit

  def store(o: A)(implicit em: EntityManager): Unit

  def delete(key: A#Key)(implicit em: EntityManager): Unit
}
