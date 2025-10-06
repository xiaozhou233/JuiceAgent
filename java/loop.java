public class loop {
    public static void main(String[] args) throws Exception {
        try {
            while (true) {
                try{
                Class clz = Class.forName("cn.xiaozhou233.Test");
                //invoke printTest()
                clz.getMethod("printTest").invoke(null);
                } catch (ClassNotFoundException e) {
                    System.out.println("Class not found");
                }
                System.out.println("Hello World");
                Thread.sleep(2000);
            }

        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}