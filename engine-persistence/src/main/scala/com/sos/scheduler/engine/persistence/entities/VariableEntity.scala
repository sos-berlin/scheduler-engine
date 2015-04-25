package com.sos.scheduler.engine.persistence.entities

import javax.persistence._

/**
 * JPA-Entity for a variable stored in a database.
 * <pre>
 * CREATE TABLE SCHEDULER_VARIABLES (
 *   NAME varchar(100) not null,
 *   WERT integer,
 *   TEXTWERT varchar(250),
 *   primary key (NAME))
 * </pre>
 */
@Entity
@Table(name = "SCHEDULER_VARIABLES")
class VariableEntity {

  @Column(name=""""NAME"""", nullable=false) @Id
  private[entities] var name: String = _

  @Column(name=""""WERT"""")
  private[entities] var int: java.lang.Integer = null

  @Column(name=""""TEXTWERT"""")
  private[entities] var string: String = _

  override def toString = "VariableEntity" + List(name, int, string).mkString("(", ",", ")")
}

object VariableEntity {
  val JobIdName = "spooler_job_id"
}
