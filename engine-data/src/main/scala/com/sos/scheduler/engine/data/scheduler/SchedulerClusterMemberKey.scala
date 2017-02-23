package com.sos.scheduler.engine.data.scheduler

import com.sos.jobscheduler.data.scheduler.SchedulerId

final case class SchedulerClusterMemberKey(schedulerId: SchedulerId, clusterMemberId: ClusterMemberId)
