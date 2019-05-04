package eap.com.demo;

import org.chromium.net.CronetException;
import org.chromium.net.UrlRequest;
import org.chromium.net.UrlResponseInfo;

import java.nio.ByteBuffer;

class MyUrlRequestCallback extends UrlRequest.Callback {
    private static final String TAG = "MyUrlRequestCallback";

    @Override
    public void onRedirectReceived(UrlRequest request, UrlResponseInfo info, String newLocationUrl) {
        android.util.Log.i(TAG, "onRedirectReceived method called.");
        // You should call the request.followRedirect() method to continue
        // processing the request.
        request.followRedirect();
    }

    @Override
    public void onResponseStarted(UrlRequest request, UrlResponseInfo info) {
        int httpStatusCode = info.getHttpStatusCode();
        android.util.Log.i(TAG, "onResponseStarted method called. httpStatusCode " + httpStatusCode);
        // You should call the request.read() method before the request can be
        // further processed. The following instruction provides a ByteBuffer object
        // with a capacity of 102400 bytes to the read() method.

        request.read(ByteBuffer.allocateDirect(102400));
    }

    @Override
    public void onReadCompleted(UrlRequest request, UrlResponseInfo info, ByteBuffer byteBuffer) {
        android.util.Log.i(TAG, "onReadCompleted method called.");
        // You should keep reading the request until there's no more data.
        request.read(ByteBuffer.allocateDirect(102400));
    }

    @Override
    public void onSucceeded(UrlRequest request, UrlResponseInfo info) {
        android.util.Log.i(TAG, "onSucceeded method called.");
    }

    @Override
    public void onFailed(UrlRequest urlRequest, UrlResponseInfo urlResponseInfo, CronetException e) {
        android.util.Log.i(TAG, "onFailed method called. Exception " + e);
    }
}