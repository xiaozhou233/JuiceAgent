import cn.xiaozhou233.juiceagent.api.JuiceAgent;

public class GetClassBytes {
    public static void main(String[] args) {
        TestSupport.run("GetClassBytes", () -> {
            TestSupport.bootstrap();

            byte[] bytes = JuiceAgent.getClassBytes(Target.class);
            TestSupport.require(TestSupport.looksLikeClassFile(bytes),
                    "getClassBytes did not return a class file");

            System.out.println("Class: " + Target.class.getName());
            System.out.println("Bytes: OK (" + bytes.length + ")");
        });
    }
}
