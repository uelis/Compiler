// Optimal paragraph formatting with dynamic programming
class Fmt {
    public static void main(String[] args) throws java.io.IOException {
        if (new Main().format()) {} else {}
    }
}

class ArrayList {
    int[] list;
    int size;

    public ArrayList init(int capacity) {
        list = new int[capacity];
        size = 0;
        return this;
    }

    public int size() {
        return size;
    }

    public ArrayList add(int i) {
        ArrayList l;
        if (size + 1 < list.length) {} else { l = this.grow(); }
        list[size] = i;
        size = size + 1;
        return this;
    }

    public int get(int i) {
        return list[i];
    }

    public ArrayList set(int i, int v) {
        list[i] = v;
        return this;
    }

    public ArrayList clear() {
        size = 0;
        return this;
    }

    public ArrayList grow() {
        int[] newlist;
        int i;

        newlist = new int[list.length * 2];
        i = 0;
        while (i < size) {
            newlist[i] = list[i];
            i = i + 1;
        }
        list = newlist;
        return this;
    }
}

class Paragraph {
    ArrayList chars;
    ArrayList words;
    ArrayList wordEnds;
    boolean complete;
    int lineLength;

    public Paragraph init() {
        lineLength = 80;
        chars = new ArrayList().init(100);
        words = new ArrayList().init(10);
        wordEnds = new ArrayList().init(10);
        return this;
    }

    public Paragraph format() {
        int[] lc;
        int[] c;
        int[] p;
        int i;
        int j;
        int l;

        if (0 < words.size()) {

        lc = new int[words.size() * words.size()]; // cost of lines with words i..j (incl).
        c = new int[words.size() + 1]; // c[i] = cost up to word i (exclusive)
        p = new int[words.size()]; // p[i] = j means that last line in solution for c[i] starts with word j

        i = 0;
        while (i < words.size()) {
            l = lineLength;
            j = i;
            while (j < words.size()){
                l = l - (wordEnds.get(j) - words.get(j) + 1);
                if (words.size() - 2 < j && 0 < l) {
                    if (j < i + 1 /* j  = i */) {
                        // penalise widows
                        lc[i * words.size() + j] = 10*10*10;
                    } else {
                        lc[i * words.size() + j] = 0;
                    }
                } else {
                    lc[i * words.size() + j] = l * l * l;
                }
                j = j + 1;
            }

            i = i + 1;
        }

        c[0] = 0;
        p[0] = 0;
        i = 0;
        while (i < words.size()) {
            c[i+1] = 0 - ((0 - 128)*256*256*256 + 1);
            j = 0;
            while (j < i + 1) {
                if (0-1 < lc[j * words.size() + i] &&
                    c[j] + lc[j * words.size() + i] < c[i+1]) {
                    c[i+1] = c[j] + lc[j * words.size() + i];
                    p[i] = j;
                } else {}
                j = j + 1;
            }
            i = i + 1;
        }

        i = this.print_formatted(p, words.size() - 1);
        } else {}
        return this;
    }

    public int print_formatted(int[] p, int j) {
        int i;
        int k;
        int u;
        int v;
        i = p[j];
        if (i < 1 && 0-1 < i) {
            k = 1;
        } else {
            k = this.print_formatted(p, i - 1) + 1;
        }
        u = i;
        while (i < j + 1) {
            v = words.get(i);
            while (v < wordEnds.get(i)) {
                System.out.write(chars.get(v));
                v = v + 1;
            }
            System.out.write(32);
            i = i + 1;
        }
        System.out.write(10);
        return k;
    }

    public Paragraph print() {
        int i;
        int j;
        int c;

        i = 0;
        while (i < words.size()) {
            j = words.get(i);
            while (j < wordEnds.get(i)) {
                System.out.write(chars.get(j));
                j = j + 1;
            }
            System.out.write(32);
            i = i + 1;
        }
        return this;
    }

    public int read(int c) throws java.io.IOException {
        chars = chars.clear();
        words = words.clear();
        wordEnds = wordEnds.clear();

        while (this.isWhiteSpace(c)) {
            c = System.in.read();
        }

        complete = false;
        while (!complete) {
            c = this.readLine(c);
        }
        return c;
    }

    public int readLine(int c) throws java.io.IOException {
        boolean isBlank;

        isBlank = true;
        while (!this.isEOL(c)) {
            if (!this.isWhiteSpace(c)) {
                isBlank = false;
                c = this.readWord(c);
            } else {
                c = System.in.read();
           }
        }
        if (isBlank) {
            complete = true;
        } else {}
        return c;
    }

    public int readWord(int c) throws java.io.IOException {

        words = words.add(chars.size());

        while (!this.isWhiteSpace(c) && !this.isEOL(c)) {
             chars = chars.add(c);
             c = System.in.read();
        }
        wordEnds = wordEnds.add(chars.size());
        return c;
    }

    public boolean isWhiteSpace(int c) {
        boolean r;
        r = false;
        r = !(!r && (!(!(c < 9) && !(9 < c))));
        r = !(!r && (!(!(c < 10) && !(10 < c))));
        r = !(!r && (!(!(c < 13) && !(13 < c))));
        r = !(!r && (!(!(c < 32) && !(32 < c))));
        return r;
    }

    public boolean isEOL(int c) {
        boolean r;
        r = false;
        r = !(!r && (!(!(c < 10) && !(10 < c))));
        r = !(!r && (0-1 < c));
       return r;
    }
}

class Main {

    public boolean format() throws java.io.IOException {
        Paragraph p;
        int c;

        p = new Paragraph().init();
        c = 0;
        while (0-1 < c) {
            c = System.in.read();
            c = p.read(c);
            // c is EOL or EOF
            p = p.format();
            System.out.write(10);
        }
        return false;
    }
}
