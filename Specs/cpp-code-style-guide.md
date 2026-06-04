# C++ Code Style Guide

This document defines the code style conventions used in the Horizon project.

**Version:** v1.0.0

**Last Updated:** 2026-05-01

---

## Contents

1. [Naming](#1-naming)
   - [1.1 File Names](#11-file-names)
   - [1.2 Struct and Class Names](#12-struct-and-class-names)
   - [1.3 Function Names](#13-function-names)
   - [1.4 Variable Names](#14-variable-names)
   - [1.5 Enumerator Names](#15-enumerator-names)
   - [1.6 Macro Names](#16-macro-names)
   - [1.7 Namespace Names](#17-namespace-names)
2. [Comments](#2-comments)
   - [2.1 File Comments](#21-file-comments)
   - [2.2 Type Comments](#22-type-comments)
   - [2.3 Function Comments](#23-function-comments)
   - [2.4 Variable Comments](#24-variable-comments)
   - [2.5 Implementation Comments](#25-implementation-comments)
   - [2.6 TODO Comments](#26-todo-comments)
   - [2.7 Deprecation Comments](#27-deprecation-comments)
3. [Formatting](#3-formatting)
   - [3.1 Indentation](#31-indentation)
   - [3.2 Braces](#32-braces)
   - [3.3 Line Length](#33-line-length)
   - [3.4 Vertical Spacing](#34-vertical-spacing)
4. [C++ Characteristic](#4-c-characteristic)
   - [4.1 Attribute](#41-attribute)

---

## 1. Naming

**Avoid abbreviations.** Use descriptive, full words. Clarity takes priority over brevity.

### 1.1 File Names

Header files: `UpperCamelCase.h`
Implementation files: `UpperCamelCase.cpp`

### 1.2 Struct and Class Names

Use **upper camel case** for all struct and class names.
Prefix interfaces with a descriptor word (e.g., `I`) is not used, describe the interface's role instead.

### 1.3 Function Names

Use **upper camel case** for all function names.
- Getter functions: `Get` + noun (e.g., `GetName`, `GetType`, `GetInstance`).
- Setter functions: `Set` + noun (e.g., `SetObjectName`).
- Boolean functions: `Is` + adjective/noun or `Has` + noun (e.g., `IsRegistered`).
- Factory/destruction functions: `Create` + noun / `Destroy` + noun (e.g., `CreateBuffer`, `DestroyBuffer`).

### 1.4 Variable Names

Use **lower camel case** for all variables.

### 1.5 Enumerator Names

Use `enum class` (strongly typed enums).

### 1.6 Macro Names

Use **UPPERCASE_WITH_UNDERSCORES**.

### 1.7 Namespace Names

All project code resides in the `Horizon` namespace.

---

## 2. Comments

**All comments must be in English.**

### 2.1 File Comments

Third-party library files must retain original license headers and author attributions.

### 2.2 Type Comments

Use Doxygen-style block comments for classes and structs. Include a brief description and reference links where relevant.

For simple structs without complex documentation, inline comments on members are acceptable.

### 2.3 Function Comments

Document public and interface functions with Doxygen-style comments. Include parameter descriptions and return value where non-obvious.

### 2.4 Variable Comments

Use inline comments for member variables that require explanation.

### 2.5 Implementation Comments

Use `//` for single-line or inline implementation notes. Explain **why**, not **what**.

### 2.6 TODO Comments

Use `@todo` to mark known work that should be done later. Include a brief description of what needs to be done.


### 2.7 Deprecation Comments

Use `@deprecated` in Doxygen comments to mark deprecated interfaces. Explain the replacement and, if applicable, the removal timeline.

---

## 3. Formatting

### 3.1 Indentation

**4 spaces** per indentation level. Do not use tabs.
Configure your editor to expand tabs to spaces.

### 3.2 Braces

Opening brace on the **same line** as the declaration/statement, with a single space before it.
Closing brace on its own line, aligned with the opening statement.
Always use braces, even for single-line bodies.

### 3.3 Line Length

### 3.4 Vertical Spacing

Separate function implementations with a **single blank line**.
Do **not** add a blank line between the function signature and the opening brace.
Do **not** add a blank line at the end of a file.

## 4. C++ Characteristic

### 4.1 Attribute

- Use `[[deprecated]]` to mark functions that are deprecated.
