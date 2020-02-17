class Naming3 {
    public static void main (String[] argv) {
        System.out.println(new A().f(new B(), 3));
    }
}

class A {

    public int f (B b, int a) {
        return 2;
    }
}
