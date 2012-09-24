package com.sos.scheduler.engine.persistence.entities;

import com.sos.scheduler.engine.data.scheduler.ClusterMemberId;
import com.sos.scheduler.engine.data.scheduler.SchedulerId;

import javax.annotation.Nullable;
import javax.persistence.*;
import java.io.Serializable;
import java.util.Date;

import static com.google.common.base.Strings.isNullOrEmpty;
import static com.sos.scheduler.engine.persistence.SchedulerDatabases.schedulerIdFromDatabase;
import static javax.persistence.TemporalType.TIMESTAMP;

@Entity
@Table(name="SCHEDULER_JOBS")
@IdClass(JobEntity.PrimaryKey.class)
public class JobEntity {
    @Column(name="spooler_id") @Id private String schedulerId;
    @Column(name="cluster_member_id") @Id private String clusterMemberId;
    @Column(name="path") @Id private String jobPath;
    @Column(name="stopped") private boolean stopped;
    @Column(name="next_start_time") @Temporal(TIMESTAMP) private Date nextStartTime;

    public SchedulerId getSchedulerId() {
        return schedulerIdFromDatabase(schedulerId);
    }

    public ClusterMemberId getClusterMemberId() {
        return new ClusterMemberId(clusterMemberId.equals("-")? "" : clusterMemberId);
    }

    @Nullable public final String getJobPath() {
        return isNullOrEmpty(jobPath)? null : "/" + jobPath;
    }

    public final boolean isStopped() {
        return stopped;
    }

    @Nullable public final Date getNextStartTime() {
        return nextStartTime == null? null : (Date)nextStartTime.clone();
    }

    public static class PrimaryKey implements Serializable {
        @Column(name="spooler_id") private String schedulerId;
        @Column(name="cluster_member_id") private String clusterMemberId;
        @Column(name="path") private String jobPath;
    }
}
