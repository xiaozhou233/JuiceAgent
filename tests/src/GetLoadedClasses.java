import cn.xiaozhou233.juiceagent.api.JuiceAgent;

import java.io.File;
import java.util.Arrays;

public class GetLoadedClasses {
    public static void main(String[] args) {
        System.load(new File("resources/libagent.dll").getAbsolutePath());
        JuiceAgent.init("");

        System.out.println(Arrays.toString(JuiceAgent.getLoadedClasses()));
    }
}
