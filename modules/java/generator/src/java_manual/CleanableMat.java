package org.opencv.core;

public abstract class CleanableMat implements AutoCloseable {

    protected CleanableMat(long obj) {
        if (obj == 0)
            throw new UnsupportedOperationException("Native object address is NULL");

        nativeObj = obj;
    }

    @Override
    public void close() {
        n_delete(nativeObj);
    }

    private static native void n_delete(long nativeObj);

    public final long nativeObj;
}
