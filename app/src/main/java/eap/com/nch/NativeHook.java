package eap.com.nch;

/**
 * Hook Native function/field
 *
 * @author yananh
 */
public class NativeHook {
    private static final int OK = 0;
    private static final int ERR = -1;

    static {
        System.loadLibrary("native_sym_hook");
        System.loadLibrary("native_plt_hook");
    }

    /**
     * 通过符号表查找对应的地址，并将制定的变量替换为并制定的符号(可能是函数指针或者变量的指针等)，将native的变量指向的内容替换为目标变量/方法
     *
     * @param originSoName     需要被替换的动态库
     * @param originFieldName  需要被替换的变量名称，<b>如果是C++，需要考虑overload会导致方法名与代码中定义的不同</b>
     * @param destSoName       需要hook的目标动态度
     * @param destVariableName 需要被替换的方法名称，<b>如果是C++，需要考虑overload会导致方法名与代码中定义的不同</b>
     * @return {@code #OK} 成功，否则失败
     */
    public static native int hook(String originSoName, String originFieldName, String destSoName, String destVariableName);

    public static native void soInfoPrint(String originSoName);
}

