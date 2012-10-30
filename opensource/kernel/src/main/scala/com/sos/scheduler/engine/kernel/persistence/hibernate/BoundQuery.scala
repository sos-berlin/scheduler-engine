package com.sos.scheduler.engine.kernel.persistence.hibernate

import javax.persistence.{TypedQuery, EntityManager}

case class BoundQuery(queryString: String, bindings: Seq[(String, Any)] = Nil) {

  def typedQuery[A](implicit em: EntityManager, m: Manifest[A]) = {
    val q = em.createQuery(queryString, m.erasure).asInstanceOf[TypedQuery[A]]
    for ((name, value) <- bindings) q.setParameter(name, value)
    q
  }

  def query[A](implicit em: EntityManager) = {
    val q = em.createQuery(queryString)
    for ((name, value) <- bindings) q.setParameter(name, value)
    q
  }

  def getResultList[A](implicit em: EntityManager, m: Manifest[A]) = typedQuery(em, m).getResultList

  def executeUpdate[A]()(implicit em: EntityManager) = query(em).executeUpdate()

  def ++(o: BoundQuery) = BoundQuery(queryString + o.queryString, bindings ++ o.bindings)
}

object BoundQuery {
  def apply(query: String, binding: (String, Any), bindings: (String, Any)*) = new BoundQuery(query, binding +: bindings)
}
