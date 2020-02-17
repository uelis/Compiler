class PrintArgs {
  public static void main (String[] argv) {
    int i;
    int n;
    n = argv.length;
    i = 0;
    while (i < n) {
      System.out.println (argv[i]);
      i = i+1;
    }
  }
}
