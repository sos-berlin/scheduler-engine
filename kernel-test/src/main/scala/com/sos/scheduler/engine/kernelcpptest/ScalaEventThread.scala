package com.sos.scheduler.engine.kernelcpptest

import com.sos.scheduler.engine.kernel.event.Event
import com.sos.scheduler.engine.kernel.event.EventPredicate
import com.sos.scheduler.engine.kernel.main.event.EventThread
import com.sos.scheduler.engine.kernel.util.Time
import com.sos.scheduler.engine.kernel.main.SchedulerController

abstract class ScalaEventThread(controller: SchedulerController) extends EventThread(controller) {
    import ScalaEventThread._
    
    def filter(predicate: PartialFunction[Event,Boolean]) {
        setEventFilter(eventPredicateOfPartialFunction(predicate))
    }

    def expectEvent(timeout: Time)(predicate: PartialFunction[Event,Boolean]) {
        expectEvent(timeout, eventPredicateOfPartialFunction(predicate))
    }
}

object ScalaEventThread {
    def eventPredicateOfPartialFunction(predicate: PartialFunction[Event,Boolean]): EventPredicate =
        new EventPredicate {
            def apply(e: Event) = predicate.isDefinedAt(e) && predicate(e)    //Get das eleganter? getOrElse?
            override def toString = predicate.toString()    // Liefert nichtssagendes "<function1>", schön wäre der Quellcode
        }
}
