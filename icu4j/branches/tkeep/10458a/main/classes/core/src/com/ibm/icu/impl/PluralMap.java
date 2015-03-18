/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.impl;

import java.util.Collections;
import java.util.EnumMap;
import java.util.HashMap;
import java.util.Map;

/**
 * PluralMap returns the appropriate value for a given plural variant.
 * Each PluralMap instance is immutable.
 * 
 * @param <T> the value type.
 *
 */
public final class PluralMap<T> {
    
    /**
     * The six plural variants.
     */
    public static enum Variant {
        OTHER("other"),
        ZERO("zero"),
        ONE("one"),
        TWO("two"),
        FEW("few"),
        MANY("many");
        
        private String name;
        
        Variant(String s) {
            this.name = s;
        }
        
        /**
         * The name of the variant.
         */
        public String getName() { return name; }
        
        /**
         * Returns the variant named name.
         * @throws IllegalArgumentException if name does not match a variant.
         */
        public static Variant valueOfName(String name) {
            Variant result = nameMap.get(name);
            if (result == null) {
                throw new IllegalArgumentException("No such name: " + name);
            }
            return result;
        }
        
        /**
         * Returns the variant named name.
         * If name does not match any variant, return defaultValue
         */
        public static Variant valueOfName(String name, Variant defaultValue) {
            Variant result = nameMap.get(name);
            return result != null ? result : defaultValue;
        }
        
        static final Map<String, Variant> nameMap = new HashMap<String, Variant>();
        static {
            for (Variant value : Variant.values()) {
                nameMap.put(value.getName(), value);
            }
        }
    }
    
    
    private EnumMap<Variant, T> map;
    
    /**
     * @param enumMap
     */
    private PluralMap(EnumMap<Variant, T> map) {
        this.map = map;
    }

    /**
     * Returns a new PluralMap with the same mappings as map. Makes a defensive copy of
     * map.
     * @throws IllegalArgumentException if map defines no OTHER variant.
     */
    public static <T> PluralMap<T> valueOf(Map<Variant, ? extends T> map) {
        validateMap(map);
        return new PluralMap<T>(new EnumMap<Variant, T>(map));
    }
    
    /**
     * Same as valueOf, but the map keys are the variant names.
     * @throws IllegalArgumentException if there is no OTHER variant or map has keys that
     *   correspond to no variant.
     */
    public static <T> PluralMap<T> valueOfNameMap(Map<String, ? extends T> map) {
        EnumMap<Variant, T> enumMap = newEnumMap();
        for (Map.Entry<String, ? extends T> entry : map.entrySet()) {
            enumMap.put(Variant.valueOfName(entry.getKey()), entry.getValue());
        }
        validateMap(enumMap);
        return new PluralMap<T>(enumMap);
    }

    /**
     * Gets the value for a particular variant. Falls back to the OTHER variant if map
     * used to create this instance provided no value for given variant. Guaranteed to return
     * a non null value.
     */
    public T get(Variant variant) {
        T result = map.get(variant);
        return result == null ? map.get(Variant.OTHER) : result;
    }
    
    /**
     * Gets the value for a particular variant. Falls back to the OTHER variant if map
     * used to create this instance provided no value for given variant or if variantName
     * corresponds to no real variant. Guaranteed to return a non null value.   
     */
    public T get(String variantName) {
        return get(Variant.valueOfName(variantName, Variant.OTHER));
    }
    
    /**
     * Returns a read-only view of the map this instance is using.
     */
    public Map<Variant, T> getMapView() {
        return Collections.unmodifiableMap(map);
    }
    
    public boolean equals(Object o) {
        if (o == this) {
            return true;
        }
        if (!(o instanceof PluralMap)) {
            return false;
        }
        PluralMap<?> rhs = (PluralMap<?>) o;
        return map.equals(rhs.map);
    }
    
    public int hashCode() {
        return (map.hashCode() & 0x739A421B);
    }

    /**
     * Convenience routine to avoid the pre JAVA 7 verboseness of creating generic maps.
     */
    public static <T> EnumMap<Variant, T> newEnumMap() {
        return new EnumMap<Variant, T>(Variant.class);
    }
    
    private static void validateMap(Map<Variant, ?> map) {
        if (map.get(Variant.OTHER) == null) {
            throw new IllegalArgumentException("Plural map must at have at least other variant.");
        }
    }

}
