package com.sos.scheduler.engine.eventbus;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;

@ForCpp
// Abstrakte Klasse statt Interface, damit C++/Java-Generator Event als Oberklasse akzeptiert.
public interface Event {}
