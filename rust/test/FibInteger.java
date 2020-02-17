class FibInteger {
  public static void main(String[] a){
    System.out.println(new FibClass().nfib(32).intValue());
  }
}

class FibClass {
  public Integer nfib (int num) {
    Integer res;
    if (num < 2) {
      res = new Integer().init(1);
    } else {
      res = new Integer().init(this.nfib(num-1).intValue() + this.nfib(num-2).intValue() + 1);
    }
    return res;
  }
}

class Integer {
  int val;

  public Integer init(int i) {
    val = i;
    return this;
  }

  public int intValue() {
    return val;
  }
}
