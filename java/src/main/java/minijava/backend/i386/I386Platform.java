package minijava.backend.i386;

import minijava.backend.Platform;

public class I386Platform implements Platform {

  @Override
  public int getWordSize() {
    return 4;
  }

}
