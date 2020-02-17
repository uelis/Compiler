class Throws1 {
    public static void main (String[] argv) throws java.io.IOException {
        System.out.println(new A().f(3));
    }
}

class A {

    public int f (int b) {
        return new B().f(b);
    }
}

class B {

    public int f (int b) throws java.io.IOException {
        return b;
    }
}
