package eap.com.demo;

import android.app.Activity;
import android.os.Bundle;

import com.eap.nh.lib.NativeHook;

import org.chromium.net.CronetException;
import org.chromium.net.UrlRequest;
import org.chromium.net.UrlResponseInfo;

public class MainActivity extends Activity {

    static {
        System.loadLibrary("cronet.72.0.3626.0");
    }

    static {
        System.loadLibrary("nh_plt");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        CronetUtils.init(this.getApplication());

//        NativeHook.printDynamicLib("cronet.72.0.3626.0.so");

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
}
