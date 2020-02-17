class HeapSort {
  public static void main(String[] a) {
      System.out.write(new HS().init(10).print().sort().print().dummy());
  }
}

class Heap {

    int[] heap;
    int count;

    public Heap init(int[] a) {
        int i;
        heap = a;
        count = a.length;

        return this.heapify();
    }

    public int max() {
        return heap[0];
    }

    public int size() {
        return count;
    }

    public int pop() {
        Heap h;
        int max;

        max = heap[0];
        count = count - 1;
        h = this.swap(0, count)
                .sift_down(0);
        return max;
    }

    public Heap heapify() {
        int i;
        Heap d;

        i = count / 2 ;
        while (0-1 < i) {
            d = this.sift_down(i);
            i = i - 1;
        }
        return this;
    }

    public Heap sift_down(int i) {
        int left;
        int right;
        int largest;
        int c;
        Heap d;

        while (this.left(i) < count) {
            left = this.left(i);
            right = this.right(i);
            largest = i;

            if (left < count && heap[largest] < heap[left]) {
                largest = left;
            } else {}

            if (right < count && heap[largest] < heap[right]) {
                largest = right;
            } else {}

            if (!(!(largest < i) && !(i < largest))) {
                d = this.swap(largest, i);
                i = largest;
            } else {
                i = count;
            }
        }
        return this;
    }

    public Heap swap(int i, int j) {
        int c;
        c = heap[i];
        heap[i] = heap[j];
        heap[j] = c;
        return this;
    }

    public int parent(int i) {
        return (i - 1) / 2;
    }

    public int left(int i) {
        return 2 * i + 1;
    }

    public int right(int i) {
        return 2 * i + 2;
    }
}

// This class contains the array of integers and
// methods to initialize, print and sort the array
// using Heapsort
class HS {

  int[] number;
  int size;

  // Initialize array of integers
  public HS init(int sz) {
    size = sz;
    number = new int[sz];

    number[0] = 20;
    number[1] = 7;
    number[2] = 12;
    number[3] = 18;
    number[4] = 2;
    number[5] = 11;
    number[6] = 6;
    number[7] = 9;
    number[8] = 19;
    number[9] = 5;

    return this;
  }

  // Invoke the Initialization, Sort and Printing
  // Methods
  public HS sort() {
      Heap heap;
      int i;
      int c;

      heap = new Heap().init(number);
      while (0 < heap.size()) {
          i = heap.pop();
          number[heap.size()] = i;
      }
      return this;
  }

  // Print array of integers
  public HS print() {
    int j;
    j = 0;
    System.out.write(10);
    while (j < (size)) {
      System.out.println(number[j]);
      j = j + 1;
    }
    return this;
  }

    public int dummy() {
        return 10;
    }

}
