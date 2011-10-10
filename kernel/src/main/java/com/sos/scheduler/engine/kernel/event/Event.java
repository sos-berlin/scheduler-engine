package com.sos.scheduler.engine.kernel.event;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;

@ForCpp
// Abstrakte Klasse statt Interface, damit C++/Java-Generator Event als Oberklasse akzeptiert.
public abstract class Event {}
