import cn.xiaozhou233.juiceagent.api.JuiceAgent;

import java.util.Arrays;

public class GetLoadedClasses {
    public static void main(String[] args) {
        TestSupport.run("GetLoadedClasses", () -> {
            TestSupport.bootstrap();

            Class<?>[] classes = JuiceAgent.getLoadedClasses();
            TestSupport.require(classes != null && classes.length > 0, "getLoadedClasses returned nothing");
            TestSupport.require(Arrays.asList(classes).contains(String.class),
                    "java.lang.String is not among the loaded classes");

            System.out.println("Loaded: " + classes.length + " classes");
        });
    }
}
