import cn.xiaozhou233.juiceagent.api.JuiceAgent;

import java.io.File;
import java.util.Arrays;

public class GetClassByName {
    public static void main(String[] args) {
        System.load(new File("resources/libagent.dll").getAbsolutePath());
        JuiceAgent.init("");

        Class<?> clazz = JuiceAgent.getClassByName("java.lang.String");

        System.out.println(clazz);
        System.out.println(clazz.getName());
        System.out.println(Arrays.toString(clazz.getDeclaredMethods()));
    }
}
