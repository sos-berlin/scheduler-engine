package com.sos.scheduler.engine.playground.zschimmer

import com.sos.scheduler.engine.kernel.util.Time
import com.sos.scheduler.engine.kernel.util.sync.{Rendezvous => JavaRendezvous}
import java.lang.Math.max
import scala.annotation.tailrec

/** Spielwiese.
 * Scala-Rendezvous könnte mit Thread und SleepingThreadTerminator zusammengebracht werden.
 * Der aufrufende Thread soll warten, bis der dienende Thread bereit ist.
 * Dabei Abbrüche berücksichtigen und nicht endlos warten. Außerdem von außen abbrechbar machen.
 * Eine Klasse, für die ein Rendezvous-Server auch ein Thread ist.
 * Vielleicht sind Scala-Actors die bessere Wahl. Die lassen sich bestimmt synchronisieren. Sind aber nicht typisiert.
 */
trait Rendezvous[ARG,RESULT] {
    private val j = new JavaRendezvous[ARG,RESULT]
    
    def serveCalls(f: => Unit) {
        try {
            j.beginServing()
            f
        }
        finally j.closeServing()
    }

    def acceptCall(f: ARG => RESULT) = acceptCallAwhile(Time.eternal)(f).get

    def acceptCallAwhile(timeout: Time)(f: ARG => RESULT): Option[RESULT] =
        enter(timeout) match {
            case None => None   // Timeout
            case Some(arg) =>
                try {
                    Some(leave(f(arg)))
                } catch {
                    case x: RuntimeException if j.isInRendezvous =>
                        j.leaveException(x)
                        None    //TODO Bei Exception None liefern wie bei Fristablauf?
                    case x: Throwable =>
                        if (j.isInRendezvous) j.leaveException(x)
                        throw x
                }
        }

    def callImpatient(arg: ARG, timeout: Time, repeatAfter: Time = Time.eternal)(timeoutCode: Time => Unit): RESULT =
        callImpatientWithVariableRepeat(arg, timeout) { t: Time =>
            timeoutCode(t)
            repeatAfter
        }

    def callImpatientWithVariableRepeat(arg: ARG, timeout: Time)(timeoutCode: Time => Time): RESULT = {
        val startTime = System.currentTimeMillis    //TODO In einem RepeatTimer auslagern
        j.asyncCall(arg)
        awaitResult(timeout) getOrElse {
            try {
                @tailrec def f(start: Long): RESULT = {
                    val elapsed = Time.ofMillis(start - startTime)
                    val nextTimeout = timeoutCode(elapsed)
                    val next = max(0, start + nextTimeout.getMillis)
                    awaitResult(Time.ofMillis(next - System.currentTimeMillis)) match {
                        case None => f(next)
                        case Some(o) => o
                    }
                }
                f(startTime + timeout.getMillis)
            }
            finally j.awaitResult   //TODO Bei einer Exception soll der call abgebrochen werden. Das Rendezvous ist dann nicht wiederholbar.
        }
    }

    private def awaitResult(t: Time) = Option(j.awaitResult(t))

    private def enter(t: Time) = Option(j.enter(t))

    private def leave(o: RESULT) = { j.leave(o);  o }
}
