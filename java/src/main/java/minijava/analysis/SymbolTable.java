package minijava.analysis;

import minijava.syntax.*;

import java.util.*;

public class SymbolTable {

  private final Map<String, ClassEntry> classes;

  public Set<String> getClasses() {
    return classes.keySet();
  }

  public ClassEntry getClassEntry(String className) {
    return classes.get(className);
  }

  static public class ClassEntry {

    private final String className;
    private ClassEntry superClass;
    // (Sorted) map of fields and methods defined in class
    private final LinkedHashMap<String, Type> fields = new LinkedHashMap<>();
    private final LinkedHashMap<String, MethodEntry> methods = new LinkedHashMap<>();

    private ClassEntry(String classId) {
      this.className = classId;
    }

    /**
     * @return the className
     */
    public String getClassName() {
      return className;
    }

    /**
     * @return the superClass
     */
    public ClassEntry getSuperClass() {
      return superClass;
    }

    public List<String> getFields() {
      return new LinkedList<>(fields.keySet());
    }

    @SuppressWarnings("WeakerAccess")
    public Type getFieldType(String fieldName) {
      return fields.get(fieldName);
    }

    public List<String> getMethods() {
      return new LinkedList<>(methods.keySet());
    }

    public MethodEntry getMethodEntry(String methodName) {
      return methods.get(methodName);
    }
  }

  static public class MethodEntry {

    private final ClassEntry definingClass;
    private final String methodName;
    private boolean throwsIOException;
    private final Type returnType;
    private final LinkedHashMap<String, Type> params = new LinkedHashMap<>();
    private final Map<String, Type> locals = new HashMap<>();

    private MethodEntry(ClassEntry definingClass, String methodId, Type returnType) {
      this.definingClass = definingClass;
      this.methodName = methodId;
      this.returnType = returnType;
    }

    public ClassEntry getDefiningClass() {
      return definingClass;
    }

    public String getMethodName() {
      return methodName;
    }

    @SuppressWarnings("WeakerAccess")
    public boolean throwsIOException() {
      return throwsIOException;
    }

    @SuppressWarnings("WeakerAccess")
    public Type getReturnType() {
      return returnType;
    }

    public List<String> getParams() {
      return new LinkedList<>(params.keySet());
    }

    @SuppressWarnings("WeakerAccess")
    public Type getParamType(String paramName) {
      return params.get(paramName);
    }

    public Set<String> getLocals() {
      return locals.keySet();
    }

    @SuppressWarnings("WeakerAccess")
    public Type getLocalType(String local) {
      return locals.get(local);
    }

  }

  @SuppressWarnings("SameParameterValue")
  static public class StaticMethodEntry extends MethodEntry {

    private StaticMethodEntry(ClassEntry definingClass, String methodId, Type returnType) {
      super(definingClass, methodId, returnType);
    }
  }

  public SymbolTable(Program p) throws TypeCheckException {

    classes = new HashMap<>();

    addMainClass(p.mainClass);
    for (ClassDeclaration c : p.classes) {
      addClass(c);
    }

    // link classes
    for (ClassDeclaration c : p.classes) {
      ClassEntry classEntry = classes.get(c.getClassName());
      assert (classEntry != null);
      if (c.getSuperName() != null) {
        ClassEntry superClassEntry = classes.get(c.getSuperName());
        if (superClassEntry == null) {
          throw new TypeCheckException("Unknown superclass " + c.getSuperName()
                  + " in declaration of class " + c.getClassName());
        } else {
          classEntry.superClass = superClassEntry;
        }
      } else {
        classEntry.superClass = classes.get("_Static");
      }
    }
  }

  private void addMainClass(MainClassDeclaration dm) {
    ClassEntry mainClass = new ClassEntry(dm.getClassName());
    MethodEntry mainMethod = new StaticMethodEntry(mainClass, "main", new TypeVoid());
    mainMethod.throwsIOException = dm.throwsIOException();
    Type stringArray = new TypeArray(new TypeClass("String"));
    mainMethod.params.put(dm.getMainArg(), stringArray);
    mainClass.methods.put("main", mainMethod);
    classes.put(dm.getClassName(), mainClass);
  }

  private void addClass(ClassDeclaration c) throws TypeCheckException {
    ClassEntry classEntry = new ClassEntry(c.getClassName());

    if (classes.put(c.getClassName(), classEntry) != null) {
      throw new TypeCheckException("Duplicate declaration of class "
              + c.getClassName() + ".");
    }

    for (VariableDeclaration field : c.getFields()) {
      if (classEntry.fields.put(field.getName(), field.getType()) != null) {
        throw new TypeCheckException("Duplicate declaration of field " + field.getName() + ".");
      }
    }

    for (MethodDeclaration method : c.getMethods()) {
      MethodEntry methodEntry = new MethodEntry(classEntry, method.getMethodName(), method.getReturnType());

      methodEntry.throwsIOException = method.throwsIOException();

      // parameters
      for (Parameter p : method.getParameters()) {
        methodEntry.params.put(p.getId(), p.getType());
      }

      // locals
      for (VariableDeclaration var : method.getLocalVars()) {
        if (methodEntry.params.get(var.getName()) != null) {
          throw new TypeCheckException("Local Variable '" + var.getName() + "' is already "
                  + "declared as a parameter.");
        }
        methodEntry.locals.put(var.getName(), var.getType());
      }

      if (classEntry.methods.put(method.getMethodName(), methodEntry) != null) {
        throw new TypeCheckException("Duplicate declaration of method "
                + method.getMethodName() + ".");
      }
    }
  }
}
