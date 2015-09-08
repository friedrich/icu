/*
*******************************************************************************
* Copyright (C) 2015, International Business Machines
* Corporation and others.  All Rights Reserved.
*******************************************************************************
* SharedObject.java, ported from sharedobject.h/.cpp
*
* C++ version created on: 2013dec19
* created by: Markus W. Scherer
*/

package com.ibm.icu.impl;


import com.ibm.icu.util.Freezable;
import com.ibm.icu.util.ICUCloneNotSupportedException;

/**
 * FreezableBase is a base class for classes implementing freezable.
 * <p>
 * Classes with only primitive or immutable fields that extends FreezableBase need only 
 * call <code>checkThawed()</code> at the beginning of each setter method to correctly
 * implement freezable.
 * <p>
 * For subclasses that have mutable fields, FreezableBase specifies a standard for
 * correctly implementing freezable.
 * <p>
 * FreezableBase subclasses with nested FreezableBase fields will use copy-on-write semantics
 * to create fewer objects on the heap than a typical object that implements Freezable directly.
 * <p>
 * FreezableBase adds on the following features to the Freezable interface:<br>
 * <ul>
 *    <li>In most cases cloneAsThawed does only a shallow copy. It accomplishes this by freezing any
 *        nested FreezableBase fields when cloning and ensuring that other Freezable objects
 *        are always frozen.</li>
 *   <li>getMutableXXX() methods to modify the contents of nested FreezableBase fields in place using copy-on-write</li>
 * </ul>
 * <br><br>
 * getMutableXXX() methods:<br>
 * <br>
 * A getMutableXXX() method returns a reference whereby the caller can safely modify
 * the corresponding FreezableBase field in place. Think of a getMutableXXX() method as
 * a C++ non-const getter method that returns a non-const pointer.
 
 * <br><br>
 * Writing your own FreezableBase subclass:<br>
 * <br>
 * To write your own FreezableBase class consider the fields/attributes of the
 * class. Field types fall into the following categories.<br>
 * <ol>
 *   <li>Primitive types and immutable types</li>
 *   <li>FreezableBase subclasses</li>
 *   <li>Freezable implementations that are not FreezableBase</li>
 *   <li>Other mutable types</li>
 * </ol><br>
 * <p>
 * The code below is a sample subclass of FreezableBase that has fields falling into each
 * of the four categories above.<br>
 * <p>
 * <pre>
 * public class MyClass extends FreezableBase<MyClass> {
 *   
 *   private int primitive = 0;
 *   private ImmutablePoint point = ImmutablePoint.valueOf(2, 3);
 *   private FreezableBaseClass value = new FreezableBaseClass().freeze();
 *   private FreezableBaseClass optionalValue = null;
 *   private FreezableClass freezable = new FreezableClass().freeze();
 *   private MutableClass pojo = new MutableClass();
 *   
 *   public void doSomeMutations() {
 *       // Throw an exception if this object is frozen.
 *       checkThawed();
 *       
 *       // Modify a primitive.
 *       primitive = 3;
 *       
 *       // Modify a FreezableBase field. Always thaw a FreezableBase field first before modifying it.
 *       // Note that if value is already unfrozen, thaw returns it unchanged.
 *       value = thaw(value);
 *       value.setFoo(7);
 *       
 *       // Modify an optional FreezableBase.
 *       if (optionalValue != null) {
 *           optionalValue = thaw(optionalValue);
 *       } else {
 *           optionalValue = new FreezableBaseClass();
 *       }
 *       optionalValue.setFoo(11);
 *       
 *       // Modify a Freezable field. Freezable fields must ALWAYS be frozen within
 *       // a FreezableBase subclass. 
 *       FreezableClass copy = freezable.cloneAsThawed();
 *       copy.doSomeMutation();
 *       freezable = copy.freeze();
 *       
 *       // Modify a plain old mutable object in place as these are deep
 *       // copied during cloning. 
 *       // If this class were never to modify this field in place, then there
 *       // is no need to override <code>cloneAsThawed()</code> to deep copy this field.
 *       pojo.doSomeMutation();
 *   }
 *   
 *   public int getPrimitive() { return primitive; }
 *   
 *   public void setPrimitive(int i) {
 *       // Throw an exception if this object is frozen.
 *       checkThawed();
 *       primitive = i;
 *   }
 *   
 *   public void getPoint() { return point; }
 *   public void setPoint(ImmutablePoint p) {
 *       // throw exception if this object is frozen.
 *       checkThawed();
 *       point = p;
 *   }
 *   
 *   // getXXX methods on FreezableBase fields return a direct reference to that field.
 *   // If this object is frozen, the returned field is frozen; if this object
 *   // is not frozen, the returned field may or may not be frozen.
 *   // Therefore, a caller should use getMutableXXX to modify a FreezableBase
 *   // field in place and should use getXXX only to get a read-only view
 *   // of the corresponding field.
 *   public FreezableBaseClass getValue() { return value; }
 *   
 *   // getMutableXXX methods guarantee that their corresponding FreezableBase
 *   // field is unfrozen and that the caller can modify it through the
 *   // returned reference. This guarantee holds only until this object is
 *   // frozen or cloned. getMutableXXX methods only work if this object is
 *   // not frozen.
 *   public FreezableBaseClass getMutableValue() {
 *       // Be sure this object is not frozen
 *       checkThawed();
 *       value = thaw(value);
 *       return value;
 *   }
 *   
 *   // Setters of FreezableBase fields should do defensive copying by cloning
 *   public void setValue(FreezableBaseClass v) {
 *       // Be sure this object is not frozen.
 *       checkThawed();
 *       this.value = v.clone();
 *   }
 *   
 *   // Handle optional FreezableBase fields the same way ordinary FreezableFields
 *   public FreezableBaseClass getOptionalValue() { return optionalValue; }
 *    
 *   public void setOptionalValue(FreezableBaseClass v) {
 *       checkThawed();
 *       this.optionalValue = v == null ? null : v.clone().freeze();
 *   }
 *   
 *   // getXXX methods for Freezable fields return a direct reference to that field.
 *   // The returned Freezable field is ALWAYS frozen even if this object is not frozen.
 *   // This is because FreezableBase.freeze() must be thrad-safe, yet the general
 *   // contract of Freezable does not mandate this constraint.
 *   public FreezableClass getFreezable() { return freezable; }
 *   
 *   // Setters of Freezable fields must always freeze their parameter
 *   // after defensive copying
 *   public void setFreezable(FreezableClass f) {
 *       checkThawed();
 *       // If this field were optional, would have to check for null.
 *       this.freezable = f.clone().freeze();
 *   }
 *   
 *   // A plain old mutable field getter must always return either clone of the
 *   // field or an umodifiable view of the field. If this method ever returned a
 *   // direct reference to its field even while this object is unfrozen,
 *   // the caller could continue to use that reference to make changes even after
 *   // this object is later frozen.
 *   public MutableClass getPojo() {
 *     return pojo.clone();
 *   }
 *   
 *   // A setter of a plain old mutable field always makes a defensive copy.
 *   public void setPojo(MutableClass o) {
 *       checkThawed();
 *       pojo = (MutableClass) o.clone();
 *   }
 *   
 *   // If MyClass contained no ordinary mutable fields, or it never mutated its ordinary mutable fields in place,
 *   // then it would not need to override cloneAsThawed.
 *   public MyClass cloneAsThawed() {
 *      MyClass result = super.cloneAsThawed();
 *      // Clone only the plain old mutable fields that this class mutates in place.
 *      // the base class handles the rest.
 *      result.pojo = result.pojo.clone();
 *      return result;
 *   }
 *   
 *   // Subclass overrides only if it has nested FreezableBase fields that are never null.
 *   protected void freezeFreezableBaseFields() {
 *       value.freeze();
 *       
 *       // no freezing of optionalValue because it can be null.
 *       
 *       // No freezing ordinary Freezable fields in here.
 *       // Doing so may cause data races.
 *   }
 *   
 * }
 * </pre>
 * 
 * @param <T> The subclass of FreezableBase.
 */
public abstract class FreezableBase<T extends FreezableBase<T>> implements Freezable<T>, Cloneable {

    /** Initially not frozen */
    public FreezableBase() {}

    /** 
     * clone this object.
     */
    @SuppressWarnings("unchecked")
    @Override
    public final T clone() {
        if (isFrozen()) {
            return (T)this;
        }
        return cloneAsThawed();
    }
    
    /**
     * Returns whether or not this object is frozen according to the contract of Freezable.
     */
    public final boolean isFrozen() {
        return bFrozen;
    }
    
    /**
     * Returns a thawed clone according to the contract of the Freezable interface.
     */
    @SuppressWarnings("unchecked")
    public T cloneAsThawed() {
        FreezableBase<T> c;
        try {
            c = (FreezableBase<T>)super.clone();
        } catch (CloneNotSupportedException e) {
            // Should never happen.
            throw new ICUCloneNotSupportedException(e);
        }
        c.bFrozen = false;
        c.freezeFreezableBaseFields();
        return (T) c;   
    }
    
    /**
     * Freezes this object and returns this.
     */
    @SuppressWarnings("unchecked")
    public final T freeze() { 
        if (!bFrozen) {
            bFrozen = true;
            freezeFreezableBaseFields();
        }
        return (T) this;
    }
    
    /**
     * freeze() and cloneAsThawed() call this method to freeze any fields that
     * extend FeezableBase that are never null.
     * Subclasses that have such fields must override this method to freeze
     * those fields. Subclasses should override this method to do nothing more than
     * freeze FreezableBase fields. Mutating other state may create data races.
     */
    protected void freezeFreezableBaseFields() {
        // Default implementation assumes there are no
        // fields that extend FreezableBase
    }
    
    /**
     * Call first thing in a mutating method such as a setXXX method to
     * ensure this object is thawed.
     */
    protected final void checkThawed() {
        if (isFrozen()) {
            throw new UnsupportedOperationException("Cannot modify a frozen object");
        }
    } 
    
    /**
     * Call from a mutating method to thaw a FreezableBase field so that it can be
     * modified like this.<br>
     * <pre>
     *   fieldToBeMutated = thaw(fieldToBeMutated);
     * </pre>
     * <br>
     * If fieldToBeMutated is frozen, thaw returns a thawed clone of it; if
     * fieldToBeMutated is not frozen, thaw returns fieldToBeMutated unchanged.
     * fieldToBeMutated must be non-null.
     */
    @SuppressWarnings("unchecked")
    protected static <U extends FreezableBase<U>> U thaw(U fieldToBeMutated) {
        if (!fieldToBeMutated.isFrozen()) {
            return fieldToBeMutated;
        }
        return fieldToBeMutated.cloneAsThawed();
    }

    private volatile boolean bFrozen = false;
}

