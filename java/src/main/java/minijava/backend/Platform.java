package minijava.backend;

/**
 * Interface to access information about a target platform.
 */
@SuppressWarnings("SameReturnValue")
public interface Platform {

  /**
   * Machine word size, typically 4 on a 32 bit machine and 8 on a 64 bit machine.
   */
  int getWordSize();

}
