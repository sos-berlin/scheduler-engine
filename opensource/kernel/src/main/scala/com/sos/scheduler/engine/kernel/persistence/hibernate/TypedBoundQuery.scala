package com.sos.scheduler.engine.kernel.persistence.hibernate

import javax.persistence.EntityManager

case class TypedBoundQuery[A <: AnyRef](override val queryString: String, clas: Class[A], override val bindings: Seq[(String, Any)] = Nil)
    extends AbstractBoundQuery {

  def typedQuery(implicit em: EntityManager) = {
    val q = em.createQuery(queryString, clas)
    for ((name, value) <- bindings) q.setParameter(name, value)
    q
  }

  def getResultList(implicit em: EntityManager) = typedQuery(em).getResultList

  def ++(o: BoundQuery) = TypedBoundQuery(queryString + o.queryString, clas, bindings ++ o.bindings)
}

object TypedBoundQuery {
  def apply[A <: AnyRef](query: String, clas: Class[A], binding: (String, Any), bindings: (String, Any)*) =
    new TypedBoundQuery(query, clas, binding +: bindings)
}
