package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter

import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected._
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting._

object Settings {
  // TODO Unter Unix auch Signal statt ExitCode prüfen!

  val list = List[(Setting, Expected)](
    /*1*/ (Setting(Shell(ExitCode(0))),
        Expected(SuccessState)),
    /*2*/ (Setting(Shell(ExitCode(7))),
        Expected(InitialState, ErrorCode("SCHEDULER-280"), Warning("SCHEDULER-845"), JobIsStopped)),
        //Agent: Expected(ErrorState, ErrorCode("SCHEDULER-280"), JobIsStopped)),   // FIXME Agent should not behave differently. InitialState is expected
    /*3*/ (Setting(Shell(ExitCode(7)), DontStopOnError),
        Expected(ErrorState, ErrorCode("SCHEDULER-280"), Warning("SCHEDULER-846", Ignorable))),   // FIXME SCHEDULER-846 not via agent

    /*4*/ (Setting(Shell(ExitCode(0)), SpoolerProcessBefore(Returns(false))),
        Expected(InitialState, Warning("SCHEDULER-845"), ErrorCode("SCHEDULER-226"), JobIsStopped)),
    /*5*/ (Setting(Shell(ExitCode(0)), DontStopOnError, SpoolerProcessBefore(Returns(false))),
        Expected(ErrorState, Warning("SCHEDULER-846"), ErrorCode("SCHEDULER-226"))),
    /*6*/ (Setting(Shell(ExitCode(0)), SpoolerProcessAfter(Returns(true))),
        Expected(SuccessState, SpoolerProcessAfterParameter(true))),
    /*7*/ (Setting(Shell(ExitCode(0)), SpoolerProcessAfter(Returns(false))),
        Expected(ErrorState, SpoolerProcessAfterParameter(true))),

    /*8*/ (Setting(Shell(ExitCode(0)), SpoolerProcessAfter(Returns(true), LogError("TEST-ERROR"))),
        Expected(SuccessState, SpoolerProcessAfterParameter(true), ErrorCode("TEST-ERROR"), JobIsStopped)),
    /*9*/ (Setting(Shell(ExitCode(0)), SpoolerProcessAfter(Throw)),
        Expected(InitialState, SpoolerProcessAfterParameter(true), ErrorCode("COM-80020009"), Warning("SCHEDULER-845"), ErrorCode("SCHEDULER-280"), JobIsStopped)),
    /*10*/ (Setting(Shell(ExitCode(7)), SpoolerProcessAfter(Returns(true))),
        Expected(SuccessState, SpoolerProcessAfterParameter(false), ErrorCode("SCHEDULER-280"), JobIsStopped)),
    /*11*/ (Setting(Shell(ExitCode(7)), SpoolerProcessAfter(Returns(false))),
        Expected(ErrorState, SpoolerProcessAfterParameter(false), ErrorCode("SCHEDULER-280"), JobIsStopped)),

    /*12*/ (Setting(SpoolerProcess(Returns(true))),
        Expected(SuccessState)),
    /*13*/ (Setting(SpoolerProcess(Returns(false))),
        Expected(ErrorState)),

    /*14*/ (Setting(SpoolerProcess(Returns(true), LogError("TEST-ERROR"))),
        Expected(SuccessState, ErrorCode("TEST-ERROR"), Warning("SCHEDULER-280"), JobIsStopped)),

    /*15*/ (Setting(SpoolerProcess(Returns(true)), SpoolerProcessBefore(Returns(false))),
      Expected(ErrorState)),   // Not InitialState???

    /*16*/ (Setting(SpoolerProcess(Returns(true)), SpoolerProcessAfter(Returns(true))),
        Expected(SuccessState, SpoolerProcessAfterParameter(true))),
    /*17*/ (Setting(SpoolerProcess(Returns(true)), SpoolerProcessAfter(Returns(false))),
        Expected(ErrorState, SpoolerProcessAfterParameter(true))),
    /*18*/ (Setting(SpoolerProcess(Returns(false)), SpoolerProcessAfter(Returns(true))),
        Expected(SuccessState, SpoolerProcessAfterParameter(false))),
    /*19*/ (Setting(SpoolerProcess(Returns(false)), SpoolerProcessAfter(Returns(false))),
        Expected(ErrorState, SpoolerProcessAfterParameter(false))),

    // Eine Fehlermeldung führt zu: SCHEDULER-280  Process terminated with exit code 1 (0x1)
    /*20*/ (Setting(SpoolerProcess(Returns(true), LogError("TEST-ERROR")), SpoolerProcessAfter(Returns(true))),
        Expected(SuccessState, SpoolerProcessAfterParameter(true), ErrorCode("TEST-ERROR"), Warning("SCHEDULER-280"), JobIsStopped)),
    /*21*/ (Setting(SpoolerProcess(Returns(true), LogError("TEST-ERROR")), DontStopOnError, SpoolerProcessAfter(Returns(true))),
        Expected(SuccessState, SpoolerProcessAfterParameter(true), ErrorCode("TEST-ERROR"), Warning("SCHEDULER-280"))),
    /*22*/ (Setting(SpoolerProcess(Returns(true), LogError("TEST-ERROR")), SpoolerProcessAfter(Returns(false))),
        Expected(ErrorState, SpoolerProcessAfterParameter(true), ErrorCode("TEST-ERROR"), Warning("SCHEDULER-280"), JobIsStopped)),
    /*23*/ (Setting(SpoolerProcess(Returns(true), LogError("TEST-ERROR")), DontStopOnError, SpoolerProcessAfter(Returns(false))),
        Expected(ErrorState, SpoolerProcessAfterParameter(true), ErrorCode("TEST-ERROR"), Warning("SCHEDULER-280"))),

    //TODO Warum wird ohne Monitor COM-80020009 Z-JAVA-105, mit Monitor aber Z-JAVA-105 gemeldet?
    /*24*/ (Setting(SpoolerProcess(Throw)),
        Expected(InitialState, ErrorCode("COM-80020009"), Warning("SCHEDULER-845"), Warning("SCHEDULER-280"), JobIsStopped)),
    /*25*/ (Setting(SpoolerProcess(Throw), DontStopOnError),
        Expected(ErrorState, ErrorCode("COM-80020009"), Warning("SCHEDULER-846"), Warning("SCHEDULER-280"))),
    /*26*/ (Setting(SpoolerProcess(Throw), SpoolerProcessAfter(Returns(true))),
        Expected(SuccessState, SpoolerProcessAfterParameter(false), ErrorCode("Z-JAVA-105"), Warning("SCHEDULER-280"), JobIsStopped)),
    /*27*/ (Setting(SpoolerProcess(Throw), DontStopOnError, SpoolerProcessAfter(Returns(true))),
        Expected(SuccessState, SpoolerProcessAfterParameter(false), ErrorCode("Z-JAVA-105"), Warning("SCHEDULER-280"))),
    /*28*/ (Setting(SpoolerProcess(Throw), SpoolerProcessAfter(Returns(false))),
        Expected(ErrorState, SpoolerProcessAfterParameter(false), ErrorCode("Z-JAVA-105"), Warning("SCHEDULER-280"), JobIsStopped)),
    /*29*/ (Setting(SpoolerProcess(Throw), DontStopOnError, SpoolerProcessAfter(Returns(false))),
        Expected(ErrorState, SpoolerProcessAfterParameter(false), ErrorCode("Z-JAVA-105"), Warning("SCHEDULER-280")))
      )
}
