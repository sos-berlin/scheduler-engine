package com.sos.scheduler.engine.eventbus;

/** Marker-Interface für ein {@link Event}, dass synchron, mitten in der Programmausführung veröffentlicht wird,
 * weil es flüchtige Objekte enthält. */
public interface HotEvent extends Event {}
