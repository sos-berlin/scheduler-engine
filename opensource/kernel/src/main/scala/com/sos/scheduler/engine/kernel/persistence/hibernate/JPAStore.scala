package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.scheduler.engine.data.base.HasKey
import javax.persistence.EntityManager

trait JPAStore[OBJ <: HasKey[KEY], KEY] {

  def fetch(key: KEY)(implicit em: EntityManager): OBJ =
    tryFetch(key) getOrElse { throw new NoSuchElementException(s"No record in database for key '$key'")}

  def tryFetch(key: KEY)(implicit em: EntityManager): Option[OBJ]

  def insert(o: OBJ)(implicit em: EntityManager)

  def store(o: OBJ)(implicit em: EntityManager)

  def delete(key: KEY)(implicit em: EntityManager)
}
