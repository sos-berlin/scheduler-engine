package com.sos.scheduler.engine.persistence.entities

import com.sos.scheduler.engine.data.scheduler.VariablePersistentState
import com.sos.scheduler.engine.persistence.entity.ObjectEntityConverter

trait VariableEntityConverter extends ObjectEntityConverter[VariablePersistentState, String, VariableEntity] {

  final def toObject(e: VariableEntity) =
    VariablePersistentState(
      name = e.name,
      value = if (e.int != null) VariablePersistentState.IntValue(e.int) else VariablePersistentState.StringValue(e.string))

  final def toEntity(o: VariablePersistentState) = {
    val k = toEntityKey(o.name)
    val e = new VariableEntity
    e.name = o.name
    o.value match {
      case VariablePersistentState.IntValue(int) ⇒ e.int = int
      case VariablePersistentState.StringValue(string) ⇒ e.string = string
    }
    e
  }

  final def toEntityKey(key: String) = key
}
