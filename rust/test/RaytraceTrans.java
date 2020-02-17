// Equations are taken from
// http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtrace0.htm
// and I've looked at
// http://www.nobugs.org/developer/htrace/htrace.hs
// when writing this.

class RaytraceTrans {

  public static void main(String[] args) {
    System.out.println((new Raytracer()).raytrace());
  }
}

class Vector {

  int x;
  int y;
  int z;

  public int getX() {
    return x;
  }

  public int getY() {
    return y;
  }

  public int getZ() {
    return z;
  }

  public int setX(int x1) {
    x = x1;
    return x;
  }

  public int setY(int y1) {
    y = y1;
    return y;
  }

  public int setZ(int z1) {
    z = z1;
    return z;
  }
}

class Ray {

  Vector r0;
  Vector dir;

  public int init() {
    r0 = new Vector();
    dir = new Vector();
    return 0;
  }

  public Vector getBase() {
    return r0;
  }

  public Vector getDir() {
    return dir;
  }
}

class Scene {

  int sphereCount;
  int[] sphereX;
  int[] sphereY;
  int[] sphereZ;
  int[] sphereR;
  int[] sphereRed;
  int[] sphereGreen;
  int[] sphereBlue;
  int[] sphereRefl;
  int[] sphereTrans;
  int lightCount;
  int[] lightsX;
  int[] lightsY;
  int[] lightsZ;

  public int init(int f) {
	 // f is a scaling factor so that 
	 // 1*f represents 0.1 and -12*f represents -1.2
	 // (approximately)

    sphereCount = 4;
    sphereX = new int[sphereCount];
    sphereY = new int[sphereCount];
    sphereZ = new int[sphereCount];
    sphereR = new int[sphereCount];
    sphereRed = new int[sphereCount];
    sphereGreen = new int[sphereCount];
    sphereBlue = new int[sphereCount];
    sphereRefl = new int[sphereCount];
    sphereTrans = new int[sphereCount];

    // red sphere
    sphereX[0] = f*(0-7);
    sphereY[0] = 0;
    sphereZ[0] = f*6;
    sphereR[0] = f*11;
    sphereRed[0] = f*5;
    sphereGreen[0] = 0;
    sphereBlue[0] = 0;
    sphereRefl[0] = f*3;
    sphereTrans[0] = 0;

    // blue sphere
    sphereX[1] = f*12;
    sphereY[1] = f*3;
    sphereZ[1] = f*6;
    sphereR[1] = f*6;
    sphereRed[1] = 0;
    sphereGreen[1] = 0;
    sphereBlue[1] = f*4;
    sphereRefl[1] = f*6;
    sphereTrans[1] = 0;

    // green sphere
    sphereX[2] = f*5;
    sphereY[2] = f*7;
    sphereZ[2] = f*(0 - 4);
    sphereR[2] = f*5;
    sphereRed[2] = 0;
    sphereGreen[2] = f*3;
    sphereBlue[2] = 0;
    sphereRefl[2] = f*1;
    sphereTrans[2] = f*8;

    // white sphere
    sphereX[3] = f*0;
    sphereY[3] = f*50;
    sphereZ[3] = f*20;
    sphereR[3] = f*40;
    sphereRed[3] = f*5;
    sphereGreen[3] = f*5;
    sphereBlue[3] = f*5;
    sphereRefl[3] = 0;
    sphereTrans[3] = 0;

    lightCount = 2;
    lightsX = new int[lightCount];
    lightsY = new int[lightCount];
    lightsZ = new int[lightCount];

    lightsX[0] = 0 - f*10;
    lightsY[0] = 0 - f*50;
    lightsZ[0] = 0 - f*50;

    lightsX[1] = f*10;
    lightsY[1] = 0 - f*10;
    lightsZ[1] = 0 - f*50;
    return 0;
  }

  public int getSphereCount() {
    return sphereCount;
  }

  public int getSphereX(int i) {
    return sphereX[i];
  }

  public int getSphereY(int i) {
    return sphereY[i];
  }

  public int getSphereZ(int i) {
    return sphereZ[i];
  }

  public int getSphereR(int i) {
    return sphereR[i];
  }

  public int getSphereRed(int i) {
    return sphereRed[i];
  }

  public int getSphereGreen(int i) {
    return sphereGreen[i];
  }

  public int getSphereBlue(int i) {
    return sphereBlue[i];
  }

  public int getSphereRefl(int i) {
    return sphereRefl[i];
  }

  public int getSphereTrans(int i) {
    return sphereTrans[i];
  }

  public int getLightCount() {
    return lightCount;
  }

  public int getLightsX(int i) {
    return lightsX[i];
  }

  public int getLightsY(int i) {
    return lightsY[i];
  }

  public int getLightsZ(int i) {
    return lightsZ[i];
  }
}

class Raytracer {

  int factor;
  int sqrfactor;
  Scene scene;
  // return values from closestIntersection
  int intersectionTime;
  int intersectionSphere;
  // return values from reflectedRay
  Vector out;
  Vector normal;
  //fuer Division
  int[] powers;

  int red;
  int green;
  int blue;

  public int init() {
    int c;
    factor = 4096;
    sqrfactor = 64;
    powers = new int[64];
    scene = new Scene();
    c = scene.init(this.div(factor,10));
    out = new Vector();
    normal = new Vector();
    return 0;
  }

  public int mul(int x, int y) {
    return this.div(x * y, factor);
  }
  
  // binaere Division
  public int div(int dividend, int dividor) {
	 boolean pos;
    int j;
    int k;
    int b;
    int res;

	 // Wir rechnen mit negativen Zahlen, da es mehr negative int-Werte gibt
	 // als positive.
	 pos = (0 < dividend);
	 if (pos) {
		dividend = 0 - dividend;
	 } else {}

	 if (0 < dividor) {
		dividor = 0 - dividor;
	 } else {
		pos = !pos;
	 }

	 powers[0] = 1;
    k = 0;
	 b = 1;
	 while (dividend < dividor * powers[k]) {
		k = k + 1;
		b = 2 * b;
		powers[k] = b;
	 }

    res = 0;
	 while (0-1 < k && dividend < 0) {
		if (dividend < powers[k] * dividor + 1) {
		  dividend = dividend - powers[k] * dividor;
		  res = res + powers[k];
		} else {}
		k = k - 1;
	 }

	 if (!pos) {
		res = 0 - res;
	 } else {}
    return res;
  }
  
  // quick'n'dirty approximation of square root
  public int sqrt(int x) {
    int i;
    int i0;
    i = x;
    i0 =  i + 1;
    while (0 < i && i < i0) {
      i0 = i;
      i = this.div(i + this.div(x, i), 2);
    }
    return i;
  }

  public int sqMag(Vector v) {
    return this.mul(v.getX(), v.getX()) + this.mul(v.getY(), v.getY()) + this.mul(v.getZ(), v.getZ());
  }

  public int dot(Vector v1, Vector v2) {
    return this.mul(v1.getX(), v2.getX()) + this.mul(v1.getY(), v2.getY()) + this.mul(v1.getZ(), v2.getZ());
  }

  // Gives return value in intersectionTime and intersectionSphere 
  // (in order to avoid allocating a new object in the absence of GC)
  public int closestIntersection(Ray r) {
    int a;
    int b;
    int c;
    int discriminant;
    int t1;
    int i;
    int n;
    intersectionTime = 10000 * factor;
    intersectionSphere = 0 - 1;
    i = 0;
    n = scene.getSphereCount();
    while (i < n) {
      a = this.sqMag(r.getDir());
      b = 2 * (this.mul(r.getDir().getX(), (r.getBase().getX() - scene.getSphereX(i)))
              + this.mul(r.getDir().getY(), (r.getBase().getY() - scene.getSphereY(i)))
              + this.mul(r.getDir().getZ(), (r.getBase().getZ() - scene.getSphereZ(i))));
      c = this.mul((r.getBase().getX() - scene.getSphereX(i)), (r.getBase().getX() - scene.getSphereX(i)))
              + this.mul(r.getBase().getY() - scene.getSphereY(i), r.getBase().getY() - scene.getSphereY(i))
              + this.mul(r.getBase().getZ() - scene.getSphereZ(i), r.getBase().getZ() - scene.getSphereZ(i))
              - this.mul(scene.getSphereR(i), scene.getSphereR(i));
      discriminant = this.mul(b, b) - 4 * this.mul(a, c);
      if (!(discriminant < 0)) {
        t1 = this.div(0 - b + sqrfactor * this.sqrt(discriminant), 2);
        if (280 < t1 && t1 < intersectionTime) {
          intersectionTime = t1;
          intersectionSphere = i;
        } else {
        }
        t1 = this.div(0 - b - sqrfactor * this.sqrt(discriminant), 2);
        if (280 < t1 && t1 < intersectionTime) {
          intersectionTime = t1;
          intersectionSphere = i;
        } else {
        }
      } else {
      }
      i = i + 1;
    }
    return 0;
  }

  // Gives return value in out and normal
  // (in order to avoid allocating a new object in the absence of GC)
  public int reflectedRay(Ray in, int sphere) {
    int mag;
    int k;
    int d;
    d = normal.setX(in.getBase().getX() - scene.getSphereX(sphere));
    d = normal.setY(in.getBase().getY() - scene.getSphereY(sphere));
    d = normal.setZ(in.getBase().getZ() - scene.getSphereZ(sphere));
    mag = sqrfactor * this.sqrt(this.sqMag(normal));
    if (0 < mag) {
      d = normal.setX(this.div(normal.getX() * factor, mag));
      d = normal.setY(this.div(normal.getY() * factor, mag));
      d = normal.setZ(this.div(normal.getZ() * factor, mag));
    } else {
    }
    k = (0 - 2) * this.dot(normal, in.getDir());
    d = out.setX(this.mul(normal.getX(), k) + in.getDir().getX());
    d = out.setY(this.mul(normal.getY(), k) + in.getDir().getY());
    d = out.setZ(this.mul(normal.getZ(), k) + in.getDir().getZ());
    mag = sqrfactor * this.sqrt(this.sqMag(out));
    if (0 < mag) {
      d = out.setX(this.div(out.getX() * factor, mag));
      d = out.setY(this.div(out.getY() * factor, mag));
      d = out.setZ(this.div(out.getZ() * factor, mag));
    } else {
    }
    return 0;
  }

  public int traceOneRay(Ray ray, int level, int colourfactor, Ray intersection, Ray rayToLight) {
    int s;
    int i;
    int d;
    int c;
    int mag;
    int colourfactorRefraction;
    Ray refraction;

    if (level < 4) {
      c = this.closestIntersection(ray);
      if (intersectionSphere < 0) {
      } else {
        d = intersection.getBase().setX(ray.getBase().getX() + this.mul(intersectionTime, ray.getDir().getX()));
        d = intersection.getBase().setY(ray.getBase().getY() + this.mul(intersectionTime, ray.getDir().getY()));
        d = intersection.getBase().setZ(ray.getBase().getZ() + this.mul(intersectionTime, ray.getDir().getZ()));
        d = intersection.getDir().setX(ray.getDir().getX());
        d = intersection.getDir().setY(ray.getDir().getY());
        d = intersection.getDir().setZ(ray.getDir().getZ());
        c = this.reflectedRay(intersection, intersectionSphere);
        s = intersectionSphere;
        i = 0;
        while (i < scene.getLightCount()) {
          d = rayToLight.getBase().setX(intersection.getBase().getX());
          d = rayToLight.getBase().setY(intersection.getBase().getY());
          d = rayToLight.getBase().setZ(intersection.getBase().getZ());
          d = rayToLight.getDir().setX(scene.getLightsX(i) - intersection.getBase().getX());
          d = rayToLight.getDir().setY(scene.getLightsY(i) - intersection.getBase().getY());
          d = rayToLight.getDir().setZ(scene.getLightsZ(i) - intersection.getBase().getZ());
          mag = sqrfactor * this.sqrt(this.sqMag(rayToLight.getDir()));
          if (0 < mag) {
            d = rayToLight.getDir().setX(this.div(factor * rayToLight.getDir().getX(), mag));
            d = rayToLight.getDir().setY(this.div(factor * rayToLight.getDir().getY(), mag));
            d = rayToLight.getDir().setZ(this.div(factor * rayToLight.getDir().getZ(), mag));
            c = this.closestIntersection(rayToLight);
            c = this.dot(rayToLight.getDir(), normal);
            if (intersectionSphere < 0 && 0 < c) {
              red = red + this.mul(this.mul(c, colourfactor), scene.getSphereRed(s));
              green = green + this.mul(this.mul(c, colourfactor), scene.getSphereGreen(s));
              blue = blue + this.mul(this.mul(c, colourfactor), scene.getSphereBlue(s));
            } else {
            }
          } else {
          }
          i = i + 1;
        }
	colourfactorRefraction = this.mul(colourfactor, scene.getSphereTrans(s));
        colourfactor = this.mul(colourfactor, scene.getSphereRefl(s));

	refraction = new Ray();
        d = refraction.init();
        d = refraction.getBase().setX(intersection.getBase().getX());
        d = refraction.getBase().setY(intersection.getBase().getY());
        d = refraction.getBase().setZ(intersection.getBase().getZ());
        d = refraction.getDir().setX(ray.getDir().getX());
        d = refraction.getDir().setY(ray.getDir().getY());
        d = refraction.getDir().setZ(ray.getDir().getZ());

        d = ray.getBase().setX(intersection.getBase().getX());
        d = ray.getBase().setY(intersection.getBase().getY());
        d = ray.getBase().setZ(intersection.getBase().getZ());
        d = ray.getDir().setX(out.getX());
        d = ray.getDir().setY(out.getY());
        d = ray.getDir().setZ(out.getZ());
        level = level + 1;
        if (0 <colourfactor) {
  	  d = this.traceOneRay(ray, level, colourfactor, intersection, rayToLight);
        } else {}
        if (0 < colourfactorRefraction) {
          d = this.traceOneRay(refraction, level, colourfactorRefraction, intersection, rayToLight);
        } else {}
      }
    } else {
    }
    return 0;
  }

  public int raytrace() {

    Ray ray;
    Ray intersection;
    Ray rayToLight;
    int c;
    int d;
    int x;
    int y;

    c = this.init();
    ray = new Ray();
    c = ray.init();
    intersection = new Ray();
    c = intersection.init();
    rayToLight = new Ray();
    c = rayToLight.init();

    System.out.println(3);
    System.out.println(400);
    System.out.println(400);
    System.out.println(256);

    y = 0;
    while (y < 400) {
      x = 0;
      while (x < 400) {
        red = 0;
        green = 0;
        blue = 0;

        d = ray.getBase().setX(this.div(x * factor, 100) - 2 * factor);
        d = ray.getBase().setY(this.div(y * factor, 100) - 2 * factor);
        d = ray.getBase().setZ((0 - 2) * factor);
        d = ray.getDir().setX(0);
        d = ray.getDir().setY(0);
        d = ray.getDir().setZ(factor);

        d = this.traceOneRay(ray, 0, factor, intersection, rayToLight);

        if (!(red < 0)) {
          System.out.println(this.div(red * 256, factor));
        } else {
          System.out.println(0);
        }
        if (!(green < 0)) {
          System.out.println(this.div(green * 256, factor));
        } else {
          System.out.println(0);
        }
        if (!(blue < 0)) {
          System.out.println(this.div(blue * 256, factor));
        } else {
          System.out.println(0);
        }
        x = x + 1;
      }
      y = y + 1;
    }
    return 999;
  }
}
