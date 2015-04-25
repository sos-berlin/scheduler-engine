package com.sos.scheduler.engine.persistence.entities

import com.sos.scheduler.engine.data.scheduler.VariablePersistentState
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class VariableEntityConverterTest extends FreeSpec {

  private object converter extends VariableEntityConverter

  "toEntityKey" in {
    assert(converter.toEntityKey("key") == "key")
  }

  "toEntity/toObject int" in {
    val persistentState = VariablePersistentState("NAME", VariablePersistentState.IntValue(123))
    val entity = converter.toEntity(persistentState)
    assert(entity.int == 123)
    assert(entity.string == null)
    assert(converter.toObject(entity) == persistentState)
  }

  "toEntity/toObject string" in {
    val persistentState = VariablePersistentState("NAME", VariablePersistentState.StringValue("TEST"))
    val entity = converter.toEntity(VariablePersistentState("NAME", VariablePersistentState.StringValue("TEST")))
    assert(entity.int == null)
    assert(entity.string == "TEST")
    assert(converter.toObject(entity) == persistentState)
  }
}
