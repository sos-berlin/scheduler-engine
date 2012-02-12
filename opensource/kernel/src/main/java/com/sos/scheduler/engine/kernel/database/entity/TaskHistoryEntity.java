package com.sos.scheduler.engine.kernel.database.entity;

import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerId;

import java.util.Date;
import javax.persistence.*;
import static com.google.common.base.Strings.*;
import static javax.persistence.TemporalType.*;

@Entity
@Table(name="SCHEDULER_HISTORY")
public class TaskHistoryEntity {
    public static final String schedulerDummyJobPath = "(Spooler)";
    
    @Id private int id;
    @Column(name="spooler_id") private String schedulerId;
    @Column(name="cluster_member_id") private String clusterMemberId;
    @Column(name="job_name") private String jobPath;
    @Column(name="start_time") @Temporal(TIMESTAMP) private Date startTime;
    @Column(name="end_time") @Temporal(TIMESTAMP) private Date endTime;
    @Column(name="cause") private String cause;
    @Column(name="steps") private Integer steps;
    @Column(name="error_code") private String errorCode;
    @Column(name="error_text") private String errorText;

    public int getId() {
        return id;
    }

    public void setId(int id) {
        this.id = id;
    }

    public SchedulerId getSchedulerId() {
        return new SchedulerId(schedulerId.equals(DatabaseSubsystem.emptyIdInDatabase)? "" : schedulerId);
    }

    public void setSchedulerId(SchedulerId id) {
        this.schedulerId = DatabaseSubsystem.idForDatabase(id);
    }

    public String getClusterMemberId() {
        return nullToEmpty(clusterMemberId);
    }

    public void setClusterMemberId(String clusterMemberId) {
        this.clusterMemberId = emptyToNull(clusterMemberId);
    }

    public String getJobPath() {
        return jobPath.equals(schedulerDummyJobPath)? "" : "/" + jobPath;
    }

    public void setJobPath(String p) {
        this.jobPath = p.isEmpty()? schedulerDummyJobPath : p.replaceFirst("^/", "");
    }

    @Override public String toString() {
        return "schedulerId=" + getSchedulerId() +
                " clusterMemberId=" + getClusterMemberId() +
                " jobName=" + getJobPath();
    }

    public String getCause() {
        return cause;
    }

    public Integer getSteps() {
        return steps;
    }

    public String getErrorCode() {
        return errorCode;
    }

    public String getErrorText() {
        return errorText;
    }

    public Date getStartTime() {
        return startTime;
    }

    public Date getEndTime() {
        return endTime;
    }
}
