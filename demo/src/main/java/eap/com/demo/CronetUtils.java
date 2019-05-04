package eap.com.demo;

import android.content.Context;
import android.text.TextUtils;
import android.util.Log;

import org.chromium.net.CronetEngine;
import org.chromium.net.UploadDataProviders;
import org.chromium.net.UrlRequest;

import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

public class CronetUtils {
    private static final String TAG = "CronetUtils";

    private static CronetUtils sInstance;

    private static CronetEngine mCronetEngine;
    private static Executor mExecutor = Executors.newCachedThreadPool();

    static {
        System.loadLibrary("cronet.72.0.3626.0");
    }

    public static void init(Context context) {
        if (mCronetEngine == null) {
            CronetEngine.Builder builder = new CronetEngine.Builder(context);
            builder.enableHttpCache(CronetEngine.Builder.HTTP_CACHE_IN_MEMORY, 100 * 1024)
                    .enableHttp2(false)
                    .enableQuic(false);
//                    .enableBrotli(true)
//                    .enableSdch(true);
            mCronetEngine = builder.build();
        }
    }

    public static void getHtml(String url, UrlRequest.Callback callback) {
        startWithURL(url, callback);
    }


    private static void startWithURL(String url, UrlRequest.Callback callback) {
        startWithURL(url, callback, null);
    }

    private static void startWithURL(String url, UrlRequest.Callback callback, String postData) {
        Log.i("MyUrlRequestCallback", "startWithURL " + url );
        UrlRequest.Builder requestBuilder = mCronetEngine.newUrlRequestBuilder(url, callback, mExecutor);
        if (!TextUtils.isEmpty(postData)) {
            requestBuilder.setHttpMethod("POST");
            requestBuilder.addHeader("Content-Type", "application/x-www-form-urlencoded");
            requestBuilder.setUploadDataProvider(UploadDataProviders.create(postData.getBytes()), mExecutor);
        }
        requestBuilder.build().start();
    }

    private static void applyPostDataToUrlRequestBuilder(UrlRequest.Builder builder, Executor executor, String postData) {
        if (postData != null && postData.length() > 0) {
            builder.setHttpMethod("POST");
            builder.addHeader("Content-Type", "application/x-www-form-urlencoded");
            builder.setUploadDataProvider(UploadDataProviders.create(postData.getBytes()), executor);
        }
    }
}
