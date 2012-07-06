package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter

import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected._
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting._

object Settings {
  // TODO Unter Unix auch Signal statt ExitCode prüfen!

  val list = List(
    Setting(Shell(ExitCode(0))) ->
        Expected(SuccessState),
    Setting(Shell(ExitCode(7))) ->
        Expected(InitialState, ErrorCode("SCHEDULER-280"), Warning("SCHEDULER-845"), JobIsStopped),
    Setting(Shell(ExitCode(7)), DontStopOnError) ->
        Expected(ErrorState, ErrorCode("SCHEDULER-280"), Warning("SCHEDULER-846")),

    Setting(Shell(ExitCode(0)), SpoolerProcessAfter(Returns(true))) ->
        Expected(SuccessState, SpoolerProcessAfterParameter(true)),
    Setting(Shell(ExitCode(0)), SpoolerProcessAfter(Returns(false))) ->
        Expected(ErrorState, SpoolerProcessAfterParameter(true)),

    Setting(Shell(ExitCode(0)), SpoolerProcessAfter(Returns(true), LogError("TEST-ERROR"))) ->
        Expected(SuccessState, SpoolerProcessAfterParameter(true), ErrorCode("TEST-ERROR"), JobIsStopped),
    Setting(Shell(ExitCode(0)), SpoolerProcessAfter(Throw)) ->
        Expected(InitialState, SpoolerProcessAfterParameter(true), ErrorCode("COM-80020009"), Warning("SCHEDULER-845"), ErrorCode("SCHEDULER-280"), JobIsStopped),
    Setting(Shell(ExitCode(7)), SpoolerProcessAfter(Returns(true))) ->
        Expected(SuccessState, SpoolerProcessAfterParameter(false), ErrorCode("SCHEDULER-280"), JobIsStopped),
    Setting(Shell(ExitCode(7)), SpoolerProcessAfter(Returns(false))) ->
        Expected(ErrorState, SpoolerProcessAfterParameter(false), ErrorCode("SCHEDULER-280"), JobIsStopped),

    Setting(SpoolerProcess(Returns(true))) ->
        Expected(SuccessState),
    Setting(SpoolerProcess(Returns(false))) ->
        Expected(ErrorState),

    Setting(SpoolerProcess(Returns(true)), SpoolerProcessAfter(Returns(true))) ->
        Expected(SuccessState, SpoolerProcessAfterParameter(true)),
    Setting(SpoolerProcess(Returns(true)), SpoolerProcessAfter(Returns(false))) ->
        Expected(ErrorState, SpoolerProcessAfterParameter(true)),
    Setting(SpoolerProcess(Returns(false)), SpoolerProcessAfter(Returns(true))) ->
        Expected(SuccessState, SpoolerProcessAfterParameter(false)),
    Setting(SpoolerProcess(Returns(false)), SpoolerProcessAfter(Returns(false))) ->
        Expected(ErrorState, SpoolerProcessAfterParameter(false)),

    // Eine Fehlermeldung führt zu: SCHEDULER-280  Process terminated with exit code 1 (0x1)
    Setting(SpoolerProcess(Returns(true), LogError("TEST-ERROR")), SpoolerProcessAfter(Returns(true))) ->
        Expected(SuccessState, SpoolerProcessAfterParameter(true), ErrorCode("TEST-ERROR"), Warning("SCHEDULER-280"), JobIsStopped),
    Setting(SpoolerProcess(Returns(true), LogError("TEST-ERROR")), DontStopOnError, SpoolerProcessAfter(Returns(true))) ->
        Expected(SuccessState, SpoolerProcessAfterParameter(true), ErrorCode("TEST-ERROR"), Warning("SCHEDULER-280")),
    Setting(SpoolerProcess(Returns(true), LogError("TEST-ERROR")), SpoolerProcessAfter(Returns(false))) ->
        Expected(ErrorState, SpoolerProcessAfterParameter(true), ErrorCode("TEST-ERROR"), Warning("SCHEDULER-280"), JobIsStopped),
    Setting(SpoolerProcess(Returns(true), LogError("TEST-ERROR")), DontStopOnError, SpoolerProcessAfter(Returns(false))) ->
        Expected(ErrorState, SpoolerProcessAfterParameter(true), ErrorCode("TEST-ERROR"), Warning("SCHEDULER-280")),

    //TODO Warum wird ohne Monitor COM-80020009 Z-JAVA-105, mit Monitor aber Z-JAVA-105 gemeldet?
    Setting(SpoolerProcess(Throw)) ->
        Expected(InitialState, ErrorCode("COM-80020009"), Warning("SCHEDULER-845"), Warning("SCHEDULER-280"), JobIsStopped),
    Setting(SpoolerProcess(Throw), DontStopOnError) ->
        Expected(ErrorState, ErrorCode("COM-80020009"), Warning("SCHEDULER-846"), Warning("SCHEDULER-280")),
    Setting(SpoolerProcess(Throw), SpoolerProcessAfter(Returns(true))) ->
        Expected(SuccessState, SpoolerProcessAfterParameter(false), ErrorCode("Z-JAVA-105"), Warning("SCHEDULER-280"), JobIsStopped),
    Setting(SpoolerProcess(Throw), DontStopOnError, SpoolerProcessAfter(Returns(true))) ->
        Expected(SuccessState, SpoolerProcessAfterParameter(false), ErrorCode("Z-JAVA-105"), Warning("SCHEDULER-280")),
    Setting(SpoolerProcess(Throw), SpoolerProcessAfter(Returns(false))) ->
        Expected(ErrorState, SpoolerProcessAfterParameter(false), ErrorCode("Z-JAVA-105"), Warning("SCHEDULER-280"), JobIsStopped),
    Setting(SpoolerProcess(Throw), DontStopOnError, SpoolerProcessAfter(Returns(false))) ->
        Expected(ErrorState, SpoolerProcessAfterParameter(false), ErrorCode("Z-JAVA-105"), Warning("SCHEDULER-280"))
  )
}