import cn.xiaozhou233.juiceagent.api.JuiceAgent;

public class playload {
    public static void entry() throws java.io.IOException {
        System.out.println("Playload loaded!");
        
        java.io.InputStream in = new java.io.FileInputStream("new_target.class");
        java.io.ByteArrayOutputStream out = new java.io.ByteArrayOutputStream();
        byte[] buf = new byte[4096];
        int len;
        while ((len = in.read(buf)) != -1) {
            out.write(buf, 0, len);
        }
        byte[] bytes = out.toByteArray();
        in.close();

        JuiceAgent.retransformClassByName(
                "target",
                bytes,
                bytes.length
        );

        System.out.println("Playload executed!");
    }
}