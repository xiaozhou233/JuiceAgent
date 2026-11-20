import cn.xiaozhou233.juiceagent.api.JuiceAgent;

public class RedefineClassByName {
    public static void main(String[] args) {
        TestSupport.run("RedefineClassByName", () -> {
            TestSupport.bootstrap();

            Target.targetTest();

            byte[] bytes = TestSupport.readClass("Target");
            TestSupport.require(JuiceAgent.redefineClassByName("Target", bytes, bytes.length),
                    "redefineClassByName returned false");

            String out = TestSupport.capture(Target::targetTest);
            TestSupport.require(out.contains("Modified!"), "redefinition had no effect: " + out);
        });
    }
}
