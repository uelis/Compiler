// Prints "0".  And nothing else!!!

class ShortCutAnd2 {
    public static void main (String[] argv) {
	System.out.println(new TestAnd().run(false));
    }
}

class TestAnd {

    public int run(boolean b) {
      	int result;
        b = b && this.sideEffect();
        if (b && this.sideEffect()) result = 1;
        else result = 0;
        while (b && this.sideEffect()) {}
      	return result;
    }

    public boolean sideEffect() {
	System.out.println(0-9999);
	System.out.println(0-9999);
	System.out.println(0-9999);
	System.out.println(0-9999);
	System.out.println(0-9999);
        return true;
    }

}
