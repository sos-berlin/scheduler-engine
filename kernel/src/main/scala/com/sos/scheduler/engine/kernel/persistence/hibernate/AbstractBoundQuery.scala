package com.sos.scheduler.engine.kernel.persistence.hibernate

import javax.persistence.EntityManager

trait AbstractBoundQuery {

  val queryString: String
  val bindings: Seq[(String, Any)]

  final def query[A](implicit em: EntityManager) = {
    val q = em.createQuery(queryString)
    for ((name, value) <- bindings) q.setParameter(name, value)
    q
  }
}
