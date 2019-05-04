package eap.com.nch;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;

import org.chromium.net.CronetException;
import org.chromium.net.UrlRequest;
import org.chromium.net.UrlResponseInfo;

public class MainActivity extends Activity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("cronet.72.0.3626.0");
        System.loadLibrary("origin_hook");
        System.loadLibrary("dest_hook");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        CronetUtils.init(this.getApplication());

        // Example of a call to a native method
//        TextView tv = (TextView) findViewById(R.id.sample_text);
//        tv.setText(stringFromJNI());

        Log.d("NativeHook", "compare(1, 2) = " + compare(1, 2));
        NativeHook.hook("liborigin_hook.so", "compare_ptr", "libdest_hook.so", "min");
        Log.d("NativeHook", "compare(1, 2) = " + compare(1, 2));
        NativeHook.soInfoPrint("AAA");

        CronetUtils.getHtml("https://www.baidu.com", new MyUrlRequestCallback() {
            @Override
            public void onSucceeded(UrlRequest request, UrlResponseInfo info) {
                super.onSucceeded(request, info);
            }

            @Override
            public void onFailed(UrlRequest urlRequest, UrlResponseInfo urlResponseInfo, CronetException e) {
                super.onFailed(urlRequest, urlResponseInfo, e);
            }
        });
    }

    public native int compare(int v1, int v2);

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native int compare1(int v1, int v2);

    public native int compare2(int v1, int v2);
}
