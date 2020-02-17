class FactorialMem {
  public static void main(String[] a){
    System.out.println(new F().compute(10));
  }
}

class F {

  int n;
  int nfactorial;

  public int compute(int num){
    int num_aux ;
    if (num < 1)
      num_aux = 1 ;
    else 
      num_aux = num * (this.compute(num-1)) ;
    return num_aux ;
  }

}
