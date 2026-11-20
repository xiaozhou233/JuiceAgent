import cn.xiaozhou233.juiceagent.api.JuiceAgent;

public class GetClassBytesByName {
    public static void main(String[] args) {
        TestSupport.run("GetClassBytesByName", () -> {
            TestSupport.bootstrap();

            // getClassBytesByName only resolves classes that are already loaded,
            // so make sure the target class is loaded before looking it up.
            Class.forName("Target");

            String className = "Target";
            byte[] bytes = JuiceAgent.getClassBytesByName(className);
            TestSupport.require(TestSupport.looksLikeClassFile(bytes),
                    "getClassBytesByName did not return a class file for " + className);

            System.out.println("Class: " + className);
            System.out.println("Bytes: OK (" + bytes.length + ")");
        });
    }
}
