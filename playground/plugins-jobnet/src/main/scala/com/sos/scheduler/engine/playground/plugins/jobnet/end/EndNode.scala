package com.sos.scheduler.engine.playground.plugins.jobnet.end

import com.sos.scheduler.engine.playground.plugins.jobnet.node.{Entrance, Node}

final case class EndNode(entrance: Entrance) extends Node {
  def exit = EndExit
}
