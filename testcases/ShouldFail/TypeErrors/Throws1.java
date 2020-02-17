class Throws1 {
    public static void main (String[] argv) {
        System.out.println(new A().f(3));
    }
}

class A {

    public int f (int b) {
        return System.in.read();
    }
}
