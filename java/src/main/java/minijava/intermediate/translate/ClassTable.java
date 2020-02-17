package minijava.intermediate.translate;

import minijava.analysis.SymbolTable;
import minijava.analysis.SymbolTable.ClassEntry;
import minijava.analysis.SymbolTable.MethodEntry;

import java.util.*;

class ClassTable {

  private final Map<String, List<String>> instanceVariables = new HashMap<>();
  private final Map<String, Map<String, MethodEntry>> instanceMethods = new HashMap<>();
  private final Map<String, List<String>> virtualMethods = new HashMap<>();

  ClassTable(SymbolTable symbolTable) {

    Set<MethodEntry> overriddenAndOverridingMethods = new HashSet<>();

    List<ClassEntry> sortedClasses = new LinkedList<>();
    for (String className : symbolTable.getClasses()) {
      ClassEntry classEntry = symbolTable.getClassEntry(className);
      LinkedList<ClassEntry> path = new LinkedList<>();
      for (ClassEntry c = classEntry; c != null && !sortedClasses.contains(c); c = c.getSuperClass()) {
        path.addFirst(c);
      }
      sortedClasses.addAll(path);
    }

    for (ClassEntry c : sortedClasses) {
      List<String> variables = new LinkedList<>();
      Map<String, MethodEntry> methods = new HashMap<>();
      if (c.getSuperClass() != null) {
        variables.addAll(instanceVariables.get(c.getSuperClass().getClassName()));
        methods.putAll(instanceMethods.get(c.getSuperClass().getClassName()));
      }
      variables.addAll(c.getFields());
      for (String m : c.getMethods()) {
        MethodEntry ownEntry = c.getMethodEntry(m);
        MethodEntry existingEntry = methods.put(m, ownEntry);
        if (existingEntry != null) {
          overriddenAndOverridingMethods.add(ownEntry);
          overriddenAndOverridingMethods.add(existingEntry);
        }
      }
      instanceVariables.put(c.getClassName(), variables);
      instanceMethods.put(c.getClassName(), methods);
    }

    for (ClassEntry c : sortedClasses) {
      List<String> virtual = new LinkedList<>();
      for (String m : c.getMethods()) {
        if (overriddenAndOverridingMethods.contains(c.getMethodEntry(m))) {
          virtual.add(m);
        }
      }
      virtualMethods.put(c.getClassName(), virtual);
    }
  }

  /**
   * Computes the list of all fields that an instance of this class
   * needs to store.
   */
  List<String> getInstanceVariables(String className) {
    return instanceVariables.get(className);
  }

  /**
   * Returns the MethodEntry of the method that is to be called if
   * {@code methodName} is called on an instance of class {@code className}.
   */
  MethodEntry getInstanceMethodEntry(String className, String methodName) {
    return instanceMethods.get(className).get(methodName);
  }

  /**
   * Returns a list of all methods that must be implemented using dynamic
   * dispatch, for example because it is overridden.
   */
  List<String> getVirtualMethods(String className) {
    return virtualMethods.get(className);
  }
}
