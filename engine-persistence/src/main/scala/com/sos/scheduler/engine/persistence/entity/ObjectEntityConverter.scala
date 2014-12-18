package com.sos.scheduler.engine.persistence.entity

trait ObjectEntityConverter[OBJ <: AnyRef, KEY, E <: AnyRef] {
  protected def toEntity(o: OBJ): E
  protected def toEntityKey(key: KEY): AnyRef
  protected def toObject(e: E): OBJ
}
