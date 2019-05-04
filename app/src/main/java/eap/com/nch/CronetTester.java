package eap.com.nch;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.SystemClock;
import android.util.Log;

import org.chromium.net.CronetException;
import org.chromium.net.UrlRequest;
import org.chromium.net.UrlResponseInfo;

import java.util.concurrent.atomic.AtomicInteger;

public class CronetTester {
    private static final String TAG = "Tester";
    public static int sCount = 0;

    public static void test(final String url) {
        final AtomicInteger count = new AtomicInteger(0);
        HandlerThread handlerThread = new HandlerThread("test-" + url);
        handlerThread.start();
        final Handler handler = new Handler(handlerThread.getLooper());
        handler.post(new Runnable() {
            private long requestTimeStamp = 0;
            private long totalCost = 0;
            private Runnable runnable;

            @Override
            public void run() {
                runnable = this;
                requestTimeStamp = SystemClock.elapsedRealtime();
                CronetUtils.getHtml(url, new MyUrlRequestCallback() {
                    @Override
                    public void onSucceeded(UrlRequest request, UrlResponseInfo info) {
                        super.onSucceeded(request, info);
                        compute();
                    }

                    @Override
                    public void onFailed(UrlRequest urlRequest, UrlResponseInfo urlResponseInfo, CronetException e) {
                        super.onFailed(urlRequest, urlResponseInfo, e);
                    }

                    private void compute() {
                        long curr = SystemClock.elapsedRealtime();
                        if (count.incrementAndGet() == 1) {
                            Log.i(TAG, "CronetTester 第一次响应[" + url + "]耗时" + (curr - requestTimeStamp) + "ms");
                        }
                        totalCost += curr - requestTimeStamp;
                        if (count.get() % 10 == 0) {
                            Log.i(TAG, "CronetTester 请求[" + url + "]" + count.get() + "次，平均" + (totalCost / count.get()) + "ms");
                        }
                        handler.postDelayed(runnable, 500);
                    }
                });
            }
        });
    }
}
