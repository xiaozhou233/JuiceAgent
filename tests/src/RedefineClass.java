import cn.xiaozhou233.juiceagent.api.JuiceAgent;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;

public class RedefineClass {
    public static void main(String[] args) throws IOException {
        System.load(new File("resources/libagent.dll").getAbsolutePath());
        JuiceAgent.init("");

        Target.targetTest();

        byte[] bytes = Files.readAllBytes(new File("out/production/injection/Target.class").toPath());
        JuiceAgent.redefineClass(Target.class, bytes, bytes.length);

        Target.targetTest();

    }
}
