package minijava.backend.i386;

import minijava.intermediate.Temp;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

@SuppressWarnings("WeakerAccess")
final class Registers {

  final static Temp eax = new Temp();
  final static Temp ebx = new Temp();
  final static Temp ecx = new Temp();
  final static Temp edx = new Temp();
  final static Temp esi = new Temp();
  final static Temp edi = new Temp();
  final static Temp ebp = new Temp();
  final static Temp esp = new Temp();
  final static List<Temp> all = Arrays.asList(eax, ebx, ecx, edx, esi, edi, ebp, esp);
  final static List<Temp> generalPurpose = Arrays.asList(ebx, esi, edi, edx, ecx, eax);
  final static List<Temp> callerSave = Arrays.asList(eax, ecx, edx);
  final static List<Temp> calleeSave = Arrays.asList(ebx, esi, edi);

  private final static Map<Temp, String> regNames;

  static {
    regNames = new HashMap<>();
    regNames.put(eax, "%eax");
    regNames.put(ebx, "%ebx");
    regNames.put(ecx, "%ecx");
    regNames.put(edx, "%edx");
    regNames.put(esi, "%esi");
    regNames.put(edi, "%edi");
    regNames.put(ebp, "%ebp");
    regNames.put(esp, "%esp");
  }

  static String nameOf(Temp t) {
    String name = regNames.get(t);
    if (name == null) {
      return t.toString();
    } else {
      return name;
    }
  }
}
