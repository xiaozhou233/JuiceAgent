import cn.xiaozhou233.juiceagent.api.JuiceAgent;

public class GetClassByName {
    public static void main(String[] args) {
        TestSupport.run("GetClassByName", () -> {
            TestSupport.bootstrap();

            Class<?> clazz = JuiceAgent.getClassByName("java.lang.String");
            TestSupport.require(clazz != null, "getClassByName returned null");
            TestSupport.require(clazz == String.class, "resolved a different Class instance");

            System.out.println("Class: " + clazz);
            System.out.println("Name:  " + clazz.getName());
        });
    }
}
