package sos.spooler;

public interface Bean<T extends HasBean> {
    T getDelegate();
}
