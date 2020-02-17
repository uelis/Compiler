class Arith {
  public static void main(String[] a) {
    System.out.println(new T().f(332432748));
  }
}

class T {
  public int f(int x) {
    int u;
    int v;
    int w;
    int z;
    u = x;
    v = (9*u) - (8*u) - (7*u) - (6*u) - (5*u) - (4*u) - (3*u) - (2*u) - (1*u) - (0*u)
      + (9*u) + (8*u) + (7*u) + (6*u) + (5*u) + (4*u) + (3*u) + (2*u) + (1*u) + (0*u)
      - (u*9) - (u*8) - (u*7) - (u*6) - (u*5) - (u*4) - (u*3) - (u*2) - (u*1) - (u*0)
      + (u*9) + (u*8) + (u*7) + (u*6) + (u*5) + (u*4) + (u*3) + (u*2) + (u*1) + (u*0);
    w = (u + (1*u)) + (u + (2*u)) +  (u + (3*u)) + (u + (4*u)) + (u + (5*u)) + (u + (6*u)) + (u + (7*u)) + (u + (8*u)) + (u + (9*u))
      - (u + (u*1)) + (u + (u*2)) +  (u + (u*3)) + (u + (u*4)) + (u + (u*5)) + (u + (u*6)) + (u + (u*7)) + (u + (u*8)) + (u + (u*9))
      * ((1*u) + v) + ((2*u) + v) +  ((3*u) + v) + ((4*u) + v) + ((5*u) + v) + ((6*u) + v) + ((7*u) + v) + ((8*u) + v) + ((9*u) + v)
      / (((u*1) + v) + ((u*2) + v) +  ((u*3) + v) + ((u*4) + v) + ((u*5) + v) + ((u*6) + v) + ((u*7) + v) + ((u*8) + v) + ((u*9) + v));
    w = w/2 + w/3 + w/4 + w/5 + (0-w)/2 + (0-w)/3 + (0-w)/4 + (0-w)/5;
    z = w + 1;
    z = z/2 + z/3 + z/4 + z/5 + (0-z)/2 + (0-z)/3 + (0-z)/4 + (0-z)/5;
    System.out.println(u);
    System.out.println(v);
    System.out.println(w);
    return z;
  }
}
