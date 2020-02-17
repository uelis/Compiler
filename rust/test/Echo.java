class IO {

  public static void main(String[] a) throws java.io.IOException {
    System.out.write((new Echo()).run());
  }
}

class Echo {

  public int run() throws java.io.IOException {
    int i;
    i = System.in.read();
    while (0 - 1 < i) {
      System.out.write(i);
      i = System.in.read();
    }
    return 10;
  }
}


