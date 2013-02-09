package com.sos.scheduler.engine.playground.plugins.jobnet.node

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.order.OrderState

final case class JobNode(entrance: Entrance, jobPath: JobPath, exit: Exit) extends Node

