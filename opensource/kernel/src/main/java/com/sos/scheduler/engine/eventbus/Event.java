package com.sos.scheduler.engine.eventbus;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import org.codehaus.jackson.annotate.JsonTypeInfo;

// Abstrakte Klasse statt Interface, damit C++/Java-Generator Event als Oberklasse akzeptiert.
@ForCpp
@JsonTypeInfo(use = JsonTypeInfo.Id.NAME, include = JsonTypeInfo.As.PROPERTY, property = "TYPE")
public interface Event {}
