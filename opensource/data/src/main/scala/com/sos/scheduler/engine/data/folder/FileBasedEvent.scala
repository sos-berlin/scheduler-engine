package com.sos.scheduler.engine.data.folder

import com.sos.scheduler.engine.data.event.KeyedEvent

trait FileBasedEvent extends KeyedEvent {
  type Key = TypedPath
  def key = typedPath

  def typedPath: TypedPath
}
