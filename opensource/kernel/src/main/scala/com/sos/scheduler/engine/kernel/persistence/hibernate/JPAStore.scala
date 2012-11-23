package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.scheduler.engine.data.base.HasKey
import javax.persistence.EntityManager

trait JPAStore[OBJ <: HasKey[KEY], KEY] {

  def tryFetch(key: KEY)(implicit em: EntityManager): Option[OBJ]

  def insert(o: OBJ)(implicit em: EntityManager)

  def store(o: OBJ)(implicit em: EntityManager)

  def delete(key: KEY)(implicit em: EntityManager)
}
