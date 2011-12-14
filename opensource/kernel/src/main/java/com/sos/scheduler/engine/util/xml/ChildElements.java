package com.sos.scheduler.engine.util.xml;

import java.util.Iterator;
import org.w3c.dom.Element;

public class ChildElements implements Iterable<Element> {
    private final Element parent;

    public ChildElements(Element parent) {
        this.parent = parent;
    }

    @Override public final Iterator<Element> iterator() {
        return new SiblingElementIterator(parent.getFirstChild());
    }
}
