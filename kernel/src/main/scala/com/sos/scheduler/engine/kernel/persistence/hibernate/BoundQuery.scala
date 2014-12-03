package com.sos.scheduler.engine.kernel.persistence.hibernate

import javax.persistence.EntityManager

final case class BoundQuery(override val queryString: String, override val bindings: Seq[(String, Any)] = Nil)
    extends AbstractBoundQuery {

  def executeUpdate()(implicit em: EntityManager) = query(em).executeUpdate()

  def ++(o: BoundQuery) = BoundQuery(queryString + o.queryString, bindings ++ o.bindings)
}

object BoundQuery {
  def apply(query: String, binding: (String, Any), bindings: (String, Any)*) = new BoundQuery(query, binding +: bindings)
}
