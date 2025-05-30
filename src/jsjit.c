/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * Recursive descent JIT
 * ----------------------------------------------------------------------------
 */

#ifdef ESPR_JIT

#include "jsjit.h"
#include "jsjitc.h"
#include "jsinteractive.h"
#include "jswrapper.h"

#define JSP_ASSERT_MATCH(TOKEN) { assert(0+lex->tk==(TOKEN));jslGetNextToken(); } // Match where if we have the wrong token, it's an internal error
#define JSP_MATCH_WITH_RETURN(TOKEN, RETURN_VAL) if (!jslMatch((TOKEN))) return RETURN_VAL;
#define JSP_MATCH_WITH_CLEANUP(TOKEN, CLEANUP_CODE) { if (!jslMatch((TOKEN))) { CLEANUP_CODE; return; } }
#define JSP_MATCH(TOKEN) if (!jslMatch((TOKEN))) return; // Match where the user could have given us the wrong token
#define JSJ_PARSING (!(execInfo.execute&EXEC_EXCEPTION))

// ----------------------------------------------------------------------------
void jsjUnaryExpression();
void jsjAssignmentExpression();
void jsjExpression();
void jsjStatement();
void jsjBlockOrStatement();
// ----------------------------------------------------------------------------
// These are helper functions that get called FROM the JITed code

/// Look up 'parent.a[index]'. Utility function called from JIT code
uint64_t _jsjxObjectLookup(JsVar *index, JsVar *parent, JsVar *a) {
  JsVar *resultParent = jsvSkipNameWithParent(a,true,parent);
  jsvUnLock2(a, parent);
  JsVar *resultA = 0;
  if (resultParent)
    resultA = jspGetVarNamedField(resultParent, index, true);
  if (!resultA) {
    if (jsvHasChildren(resultParent)) {
      // if no child found, create a pointer to where it could be
      // as we don't want to allocate it until it's written
      resultA = jsvCreateNewChild(resultParent, index, 0);
    } else {
      jsExceptionHere(JSET_ERROR, "Field or method %q does not already exist, and can't create it on %t", index, resultParent);
    }
  }
  jsvUnLock(index);
  return ((uint64_t)(size_t)resultA) | (((uint64_t)(size_t)resultParent)<<32);
}

// Like jspeFunctionCall but we unlock ALL the vars supplied
NO_INLINE JsVar *_jsjxFunctionCallAndUnLock(JsVar *functionName, JsVar *thisArg, bool isParsing, int argCount, JsVar **argPtr) {
  JsVar *function = jsvSkipName(functionName);
  JsVar *r = jspeFunctionCall(function, functionName, thisArg, isParsing, argCount, argPtr);
  jsvUnLockMany(argCount, argPtr);
  jsvUnLock3(function, functionName, thisArg);
  return r;
}

// Call jsvReplaceWithOrAddToRoot but unlock (and skip names on) the second argument
NO_INLINE JsVar *_jsxAssignment(JsVar *dst, JsVar *src) {
  src = jsvSkipNameAndUnLock(src);
  jsvReplaceWithOrAddToRoot(dst, src);
  jsvUnLock(src);
  return dst;
}

// Handle postfix inc and dec nicely - without us having to add a bunch of extra code. var is unlocked automatically, the result is returned
NO_INLINE JsVar *_jsxPostfixIncDec(JsVar *var, char op) {
  JsVar *one = jsvNewFromInteger(1);
  JsVar *oldValue = jsvAsNumberAndUnLock(jsvSkipName(var)); // keep the old value (but convert to number)
  JsVar *res = jsvMathsOpSkipNames(var, one, op);
  jsvReplaceWith(var, res);
  jsvUnLock3(var,res,one);
  return oldValue; // return the number from before we incremented
}

// Handle prefix inc and dec nicely - without us having to add a bunch of extra code
NO_INLINE JsVar *_jsxPrefixIncDec(JsVar *var, char op) {
  JsVar *one = jsvNewFromInteger(1);
  JsVar *res = jsvMathsOpSkipNames(var, one, op);
  jsvReplaceWith(var, res);
  jsvUnLock2(res, one);
  return var; // return the number from before we incremented
}

// Handle +=/-=/etc nicely - without us having to add a bunch of extra code (unlocks rhs)
NO_INLINE JsVar *_jsxMathAssignment(JsVar *var, JsVar *rhs, char op) {
  JsVar *res = jsvMathsOpSkipNames(var, rhs, op);
  jsvReplaceWith(var, res);
  jsvUnLock2(res, rhs);
  return var; // return the number from before we incremented
}

NO_INLINE JsVar *_jsxMathsOpSkipNamesAndUnLock(JsVar *a, JsVar *b, int op) {
  JsVar *r = jsvMathsOpSkipNames(a,b,op);
  jsvUnLock2(a,b);
  return r;
}

// Add a variable to the current scope (eg VAR statement), and return it
NO_INLINE JsVar *_jsxAddVar(const char *name) {
  JsVar *scope = jspeiGetTopScope();
  JsVar *a = jsvFindOrAddChildFromString(scope, name);
  jsvUnLock(scope);
  return a;
}

// Assign to a variable for its initial assignment (VAR/LET/CONST) and unlock it
NO_INLINE void _jsxVarInitialAssign(JsVar *a, bool isConstant, JsVar *initialValue) {
  if (!a) return; // no memory
  if (initialValue) {
    initialValue = jsvSkipNameAndUnLock(initialValue);
    jsvReplaceWith(a, initialValue);
  }
  if (isConstant)
    a->flags |= JSV_CONSTANT;
  jsvUnLock2(a, initialValue);
}

// When parsing {a:5} this is called to add each new object element. It unlocks 'varName' and 'value'
NO_INLINE void _jsxObjectNewElement(JsVar *object, JsVar *varName, JsVar *value) {
  JsVar *contentsName = jsvFindChildFromVar(object, varName, true);
  if (contentsName)
    jsvUnLock(jsvSetValueOfName(contentsName, value));
  jsvUnLock2(varName, value);
}

// When parsing [1,2,3] this is called to add each new array item. It unlocks 'value'
NO_INLINE void _jsxArrayNewElement(JsVar *array, uint32_t index, JsVar *value) {
  JsVar *indexName = jsvMakeIntoVariableName(jsvNewFromInteger(index),  value);
  if (indexName) { // could be out of memory
    jsvAddName(array, indexName);
    jsvUnLock(indexName);
  }
  jsvUnLock(value);
}

// Return a locked 'this' variable
NO_INLINE JsVar *_jsxGetThis() {
  return jsvLockAgain( execInfo.thisVar ? execInfo.thisVar : execInfo.root );
}
// ----------------------------------------------------------------------------

void jsjPopAsVar(int reg) {
  JsjValueType varType = jsjcPop(reg);
  jsjcConvertToJsVar(reg, varType);
}

void jsjPopNoName(int reg) {
  if (jsjcGetTopType()==JSJVT_JSVAR_NO_NAME) {
    // if we know we don't have a name here, we can skip jsvSkipNameAndUnLock
    jsjPopAsVar(reg);
    return;
  }
  jsjPopAsVar(0); // a -> r0
  jsjcCall(jsvSkipNameAndUnLock);
  if (reg != 0) jsjcMov(reg, 0);
}

void jsjPopAsBool(int reg) {
  // FIXME handle int/bool differently?
  jsjPopNoName(0);
  jsjcCall(jsvGetBoolAndUnLock); // optimisation: we should know if we have a var or a name here, so can skip jsvSkipNameAndUnLock sometimes
  if (reg != 0) jsjcMov(reg, 0);
}

void jsjPopAndUnLock() {
  jsjPopAsVar(0); // a -> r0
  // optimisation: if item on stack is NOT a variable, no need to covert+unlock!
  jsjcCall(jsvUnLock); // we're throwing this away now - unlock
}

// Write the code to create variable 'var' in register 'reg'. Clobbers r0-r3
void jsjJsVar(int reg, JsVar *var) {
  if (jsvIsString(var)) {
    int len = jsjcLiteralString(1, var, false);
    jsjcLiteral32(0, len);
    jsjcCall(jsvNewStringOfLength);
    if (reg) jsjcMov(reg, 0);
  } else if (jsvIsSimpleInt(var)) {
    jsjcLiteral32(0, (uint32_t)jsvGetInteger(var));
    jsjcCall(jsvNewFromInteger);
    if (reg) jsjcMov(reg, 0);
  } else {
    jsjcLiteral32(reg, 0);
    jsExceptionHere(JSET_ERROR, "jsjJsVar for %t not implemented yet", var);
  }
}

/// Code to add at the beginning of the function
void jsjFunctionStart() {
  jsjcDebugPrintf("; Function start\n");
  jsjcPushAll(); // Function start - push all registers since we're not meant to mess with r4..r7
}

/// Code to add right at the end of the function (or when we return)
void jsjFunctionReturn(bool isReturnStatement) {
  jsjcDebugPrintf("; Function return\n");
  int oldStackDepth = jit.stackDepth;
  if (jit.varCount) {
    jsjcMov(4, 0); // save r0 (return value)
    jsjcMov(1, JSJAR_SP);
    jsjcLiteral32(0, jit.stackDepth);
    for (int i=0;i<jit.stackDepth;i++) // we don't want to be trying to unlock ints!
      assert(jit.typeStack[i]==JSJVT_JSVAR || jit.typeStack[i]==JSJVT_JSVAR_NO_NAME);
    jsjcCall(jsvUnLockMany);
    jsjcAddSP(4*jit.varCount); // pop off anything on the stack
    jsjcMov(0, 4); // restore r0
  }
  // actual stack depth is stackDepth but at this point varCount==stackDepth we hope
  // and if not an assert will catch us
  jsjcPopAllAndReturn(); // pop r4...r7
  // If it's a return, put stack depth back where it was
  // so it's correct for the rest of the code
  if (isReturnStatement)
    jit.stackDepth = oldStackDepth;
}

/* Called when we encounter an ID. This checks if it's in our 'jit.vars'
list and if not either creates (creationOp==LEX_R_VAR/LET/CONST) or
tries to find it (creationOp==LEX_ID) it in our global scope.
hasInitialiser=true if an initial value is already on the stack */
void jsjFactorIDAndUnLock(JsVar *name, LEX_TYPES creationOp) {
  const int VARINDEX_MASK = 0xFFFF; // mask to return the actual var index
  const int VARINDEX_NO_NAME = 0x10000; // flag set if we're sure there is no name
  // search for var in our list...
  JsVar *varIndex = jsvFindChildFromVar(jit.vars, name, true/*addIfNotFound*/);
  JsVar *varIndexVal = jsvSkipName(varIndex);
  if (jit.phase == JSJP_SCAN && jsvIsUndefined(varIndexVal)) {
    // We don't have it yet - create a var list entry
    JsjValueType varType = JSJVT_JSVAR;
    // Now add the code which will create the variable right at the start of the file
    if (creationOp==LEX_ID) { // Just a normal ID
      // See if it's a builtin function, if builtinFunction!=0
      char tokenName[JSLEX_MAX_TOKEN_LENGTH];
      jsvGetString(name, tokenName, sizeof(tokenName));
      JsVar *builtin = jswFindBuiltInFunction(0, tokenName);
      if (jsvIsNativeFunction(builtin)) { // it's a built-in function - just create it in place rather than searching
        jsjcDebugPrintf("; Native Function %j\n", name);
        jsjcLiteral32(0, builtin->varData.native.ptr);
        jsjcLiteral16(1, false, builtin->varData.native.argTypes);
        jsjcCall(jsvNewNativeFunction); // JsVar *jsvNewNativeFunction(void (*ptr)(void), unsigned short argTypes)
        varType = JSJVT_JSVAR_NO_NAME;
      } else if (jsvIsPin(builtin)) { // it's a built-in pin - just create it in place rather than searching
        jsjcDebugPrintf("; Native Pin %j\n", name);
        jsjcLiteral32(0, jsvGetInteger(builtin));
        jsjcCall(jsvNewFromPin); // JsVar *jsvNewNativeFunction(void (*ptr)(void), unsigned short argTypes)
        varType = JSJVT_JSVAR_NO_NAME;
      } else { // it's not a builtin function - just search for the variable the normal way
        jsjcDebugPrintf("; Find Variable %j\n", name);
        jsjcLiteralString(0, name, true); // null terminated string in r0
        jsjcCall(jspGetNamedVariable); // Find the var in the current scopes (always returns something even if it's jsvNewChild)
      }
      jsvUnLock(builtin);
    } else if (creationOp==LEX_R_VAR || creationOp==LEX_R_LET || creationOp==LEX_R_CONST) {
      jsjcDebugPrintf("; Variable Decl %j\n", name);
      jsjcLiteralString(0, name, true); // null terminated string in r0
      // _jsxAddVar(r0:name)
      jsjcCall(_jsxAddVar); // add the variable
    } else assert(0);
    jsjcPush(0, varType); // Push the value onto the stack (which will end up being our vars list after SCAN phase)
    // Now add the index to our list
    int varIndexNumber = jit.varCount++;
    if (varType == JSJVT_JSVAR_NO_NAME)
      varIndexNumber |= VARINDEX_NO_NAME; // if we're sure there's no name
    varIndexVal = jsvNewFromInteger(varIndexNumber);
    jsvSetValueOfName(varIndex, varIndexVal);
  }
  // Now, we have the var already - just reference it
  int varIndexI = jsvGetIntegerAndUnLock(varIndexVal);
  if (jit.phase == JSJP_EMIT) {
    JsjValueType varType = JSJVT_JSVAR;
    if (varIndexI & VARINDEX_NO_NAME) // decode varType from the flags
      varType = JSJVT_JSVAR_NO_NAME;
    varIndexI &= VARINDEX_MASK;
    jsjcDebugPrintf("; Reference var %j\n", name);
    jsjcLoadImm(0, JSJAR_SP, (jit.stackDepth - (varIndexI+1)) * 4);
    jsjcCall(jsvLockAgain);
    jsjcPush(0, varType); // Push, with the type we got from the varIndex flags
  }
  jsvUnLock2(varIndex, name);
}

void jsjFactorObject() {
  if (jit.phase == JSJP_EMIT) {
    jsjcDebugPrintf("; New Object\n");
    // create the object
    jsjcCall(jsvNewObject);
    jsjcMov(4, 0); // Store it in r4
  }
  /* JSON-style object definition */
  JSP_ASSERT_MATCH('{');
  while (JSJ_PARSING && lex->tk != '}') {
    JsVar *varName = 0;
    // we only allow strings or IDs on the left hand side of an initialisation
    if (jslIsIDOrReservedWord()) {
      varName = jslGetTokenValueAsVar();
      jslGetNextToken(); // skip over current token
    } else if (
        lex->tk==LEX_STR ||
        lex->tk==LEX_FLOAT ||
        lex->tk==LEX_INT) {
      JsVar *jspeFactor(); // use the main parser to get these into a var for us!
      varName = jspeFactor();
    } else {
      JSP_ASSERT_MATCH(LEX_ID); // fail nicely
    }
    JSP_MATCH_WITH_CLEANUP(':', jsvUnLock(varName));
    jsjAssignmentExpression();
    if (jit.phase == JSJP_EMIT) {
      varName = jsvAsArrayIndexAndUnLock(varName);
      jsjcDebugPrintf("; New Object field %j\n", varName);
      jsjPopNoName(5); // r2 = array item
      jsjJsVar(1, varName); // r1 = index
      jsjcMov(2, 5); // r2 = array item
      jsjcMov(0, 4); // r0 = array
      jsjcCall(_jsxObjectNewElement);
    }
    jsvUnLock(varName);
    // no need to clean here, as it will definitely be used
    if (lex->tk != '}') JSP_MATCH(',');
  }
  JSP_ASSERT_MATCH('}');
  if (jit.phase == JSJP_EMIT) {
    // push the finished object on the stack
    jsjcPush(4, JSJVT_JSVAR_NO_NAME);
  }
}

void jsjFactorArray() {
  uint32_t idx = 0; // current array index

  if (jit.phase == JSJP_EMIT) {
    // create the array
    jsjcCall(jsvNewEmptyArray);
    jsjcMov(4, 0); // Store it in r4
  }

  /* JSON-style array */
  JSP_ASSERT_MATCH('[');
  while (JSJ_PARSING && lex->tk != ']') {
    if (lex->tk != ',') { // #287 - [,] and [1,2,,4] are allowed
      jsjAssignmentExpression();
      if (jit.phase == JSJP_EMIT) {
        jsjPopNoName(2); // r2 = array item
        jsjcLiteral32(1, idx); // r1 = index
        jsjcMov(0, 4); // r0 = array
        jsjcCall(_jsxArrayNewElement);
      }
    }
    if (lex->tk != ']') JSP_MATCH(',');
    idx++;
  }
  JSP_MATCH(']');
  if (jit.phase == JSJP_EMIT) {
    jsjcMov(0, 4); // r0 = array
    jsjcLiteral32(1, idx); // r1 = array size
    jsjcLiteral32(2, 0); // r1 = truncate = false
    jsjcCall(jsvSetArrayLength);
    // push the finished array on
    jsjcPush(4, JSJVT_JSVAR_NO_NAME);
  }
}

void jsjFactor() {
  if (lex->tk==LEX_ID) {
    JsVar *name = jslGetTokenValueAsVar();
    JSP_ASSERT_MATCH(LEX_ID);
    jsjFactorIDAndUnLock(name, LEX_ID);
  } else if (lex->tk==LEX_INT) {
    int64_t v = jsvGetLongIntegerAndUnLock(jslGetTokenValueAsVar());
    JSP_ASSERT_MATCH(LEX_INT);
    if (jit.phase == JSJP_EMIT) {
      if (v>>32) {
        jsjcLiteral64(0, (uint64_t)v);
        jsjcCall(jsvNewFromLongInteger);
      } else {
        jsjcLiteral32(0, (uint32_t)v);
        jsjcCall(jsvNewFromInteger);
      }
      jsjcPush(0, JSJVT_JSVAR_NO_NAME); // a value, not a NAME, FIXME - push an int and convert later
    }
  } else if (lex->tk==LEX_FLOAT) {
    double v = stringToFloat(jslGetTokenValueAsString());
    JSP_ASSERT_MATCH(LEX_FLOAT);
    if (jit.phase == JSJP_EMIT) {
      jsjcLiteral64(0, *((uint64_t*)&v));
      jsjcCall(jsvNewFromFloat);
      jsjcPush(0, JSJVT_JSVAR_NO_NAME); // a value, not a NAME
    }
  } else if (lex->tk=='(') {
    JSP_ASSERT_MATCH('(');
    // Just parse a normal expression (which can include commas)
    jsjExpression();
    // FIXME: Arrow functions??
    JSP_MATCH(')');
  } else if (lex->tk==LEX_R_TRUE || lex->tk==LEX_R_FALSE) {
    if (jit.phase == JSJP_EMIT) {
      jsjcLiteral32(0, lex->tk==LEX_R_TRUE);
      jsjcCall(jsvNewFromBool);
      jsjcPush(0, JSJVT_JSVAR_NO_NAME); // a value, not a NAME
    }
    JSP_ASSERT_MATCH(lex->tk);
  } else if (lex->tk==LEX_R_NULL) {
    JSP_ASSERT_MATCH(LEX_R_NULL);
    if (jit.phase == JSJP_EMIT) {
      jsjcLiteral32(0, JSV_NULL);
      jsjcCall(jsvNewWithFlags);
      jsjcPush(0, JSJVT_JSVAR_NO_NAME); // a value, not a NAME
    }
  } else if (lex->tk==LEX_R_UNDEFINED) {
    JSP_ASSERT_MATCH(LEX_R_UNDEFINED);
    if (jit.phase == JSJP_EMIT) {
      jsjcLiteral32(0, 0);
      jsjcPush(0, JSJVT_JSVAR_NO_NAME); // a value, not a NAME
    }
  } else if (lex->tk==LEX_STR) {
    JsVar *a = jslGetTokenValueAsVar();
    JSP_ASSERT_MATCH(LEX_STR);
    if (jit.phase == JSJP_EMIT) {
      jsjJsVar(0, a);
      jsjcPush(0, JSJVT_JSVAR_NO_NAME); // a value, not a NAME
    }
    jsvUnLock(a);
  } else if (lex->tk=='{') {
    jsjFactorObject();
  } else if (lex->tk=='[') {
    jsjFactorArray();
  /*} else if (lex->tk==LEX_R_FUNCTION) {
    if (!jspCheckStackPosition()) return 0;
    JSP_ASSERT_MATCH(LEX_R_FUNCTION);
    return jsjFunctionDefinition(true);*/
  } else if (lex->tk==LEX_R_THIS) {
    JSP_ASSERT_MATCH(LEX_R_THIS);
    if (jit.phase == JSJP_EMIT) {
      jsjcCall(_jsxGetThis);
      jsjcPush(0, JSJVT_JSVAR_NO_NAME); // a value, not a NAME
    }
  /*} else if (lex->tk==LEX_R_DELETE) {
    if (!jspCheckStackPosition()) return 0;
    return jsjFactorDelete();
  } else if (lex->tk==LEX_R_TYPEOF) {
    if (!jspCheckStackPosition()) return 0;
    return jsjFactorTypeOf();
  */} else if (lex->tk==LEX_R_VOID) {
    JSP_ASSERT_MATCH(LEX_R_VOID);
    if (jit.phase == JSJP_EMIT) {
      jsjUnaryExpression();
      jsjcCall(jsvUnLock);
      jsjcLiteral32(0, 0);
      jsjcPush(0, JSJVT_JSVAR_NO_NAME); // a value, not a NAME
    }
  } else JSP_MATCH(LEX_EOF);
}

// Parse ./[] - return true if the parent of the current item is currently on the stack
bool jsjFactorMember() {
  bool parentOnStack = false;
  while ((lex->tk=='.' || lex->tk=='[') && JSJ_PARSING) {
    if (lex->tk == '.') { // ------------------------------------- Record Access
      JSP_ASSERT_MATCH('.');
      if (jslIsIDOrReservedWord()) {
        if (jit.phase == JSJP_EMIT) {
          JsVar *a = jslGetTokenValueAsVar();
          jsjcLiteralString(0, a, true); // null terminated
          jsvUnLock(a);
          // r0 = string pointer
          jsjcCall(jsvNewFromString);
          // r0 = index (as JsVar)
        }
        jslGetNextToken(); // skip over current token (we checked above that it was an ID or reserved word)
      } else {
        // incorrect token - force a match fail by asking for an ID
        JSP_MATCH_WITH_RETURN(LEX_ID, false); // if we fail we're stopping compilation anyway
      }
    } else if (lex->tk == '[') { // ------------------------------------- Array Access
      JSP_ASSERT_MATCH('[');
      jsjAssignmentExpression();
      if (jit.phase == JSJP_EMIT) {
        jsjPopNoName(0);
        jsjcCall(jsvAsArrayIndexAndUnLock);
      }
      JSP_MATCH_WITH_RETURN(']', false); // if we fail we're stopping compilation anyway
      // r0 = index
    } else {
      assert(0);
    }
    if (jit.phase == JSJP_EMIT) {
      // r0 currently = index
      if (parentOnStack) jsjPopAsVar(1); // r1 = parent
      else jsjcLiteral32(1, 0);
      jsjPopAsVar(2); // r2 = the variable itself
      jsjcCall(_jsjxObjectLookup); // (a,parent) = _jsjxObjectLookup(index, parent, a)
      jsjcPush(0, JSJVT_JSVAR); // a
      jsjcPush(1, JSJVT_JSVAR); // parent
      parentOnStack = true;
    }
  }
  return parentOnStack;
}

void jsjFactorFunctionCall() {
  jsjFactor();
  bool parentOnStack = jsjFactorMember();
  // FIXME: what about 'new'?

  while (lex->tk=='(' /*|| (isConstructor && JSP_SHOULD_EXECUTE))*/ && JSJ_PARSING) {
    /* Right now, the parent (if parentOnStack) is on the stack,
     follow by the function...

     NOW PARSE OUR ARGUMENTS

     * Push each new argument onto the stack (it grows down)
     * Args are in the wrong order, so we emit code to swap around the args in the array
     * At the end, SP = what we need as 'argPtr' for jspeFunctionCall

     optimisation: If we knew how many args we had ahead of time, we could subtract that
     from the stack pointer, save it, and then instead of pushing onto the stack we could
     just write direct to the correct address.
     */
    int argCount = 0;
    JSP_MATCH('(');
    DEBUG_JIT_EMIT("; FUNCTION CALL arguments\n");
    while (JSJ_PARSING && lex->tk!=')' && lex->tk!=LEX_EOF) {
      argCount++;
      jsjAssignmentExpression();
      if (jit.phase == JSJP_EMIT) {
        // Ensure params are always just JsVar (no int/etc or even names)
        if (jsjcGetTopType() != JSJVT_JSVAR_NO_NAME) {
          jsjPopNoName(0);
          jsjcPush(0, JSJVT_JSVAR_NO_NAME); // push argument to stack
        }
      }
      if (lex->tk!=')') JSP_MATCH(',');
    }
    JSP_MATCH(')');
    if (jit.phase == JSJP_EMIT) {
      // Stack looks like this now:
      //  <top of stack> argN, ... arg2, arg1, funcName, [funcParent], <rest of stack>
      DEBUG_JIT("; FUNCTION CALL argPtr\n");
      jsjcMov(7, JSJAR_SP); // r7 = argPtr
      jsjcPush(7, JSJVT_INT); // argPtr (5th arg - on stack)
      // Args are in the wrong order - we have to swap them around if we have >1!
      if (argCount>1) {
        DEBUG_JIT("; FUNCTION CALL reverse arguments\n");
        for (int i=0;i<argCount/2;i++) {
          int a1 = i*4;
          int a2 = (argCount-(i+1))*4;
          jsjcLoadImm(0, 7, a1); // r0 = memory[argPtr+a1]
          jsjcLoadImm(1, 7, a2); // ...
          jsjcStoreImm(0, 7, a2);
          jsjcStoreImm(1, 7, a1);
        }
      }
      // Stack looks like this now:
      //  <top of stack> arg1, arg2, ... argN, funcName, [funcParent], <rest of stack>
      DEBUG_JIT("; FUNCTION CALL jspeFunctionCall\n");
      // Get function var and parent (r7 == SP)

      if (parentOnStack) { // parent
        jsjcLoadImm(0, 7, 4*(argCount+1)); // r0 = funcName
        jsjcLoadImm(1, 7, 4*(argCount));
      } else { // no parent
        jsjcLoadImm(0, 7, 4*argCount); // r0 = funcName
        jsjcLiteral32(1, 0);
      }
      jsjcLiteral32(2, 0); // isParsing = false
      jsjcLiteral32(3, argCount); // argCount 4th arg
      jsjcCall(_jsjxFunctionCallAndUnLock); // a = _jsjxFunctionCallAndUnLock(funcName, thisArg/parent, isParsing, argCount, argPtr[on stack]);
      DEBUG_JIT("; FUNCTION CALL cleanup stack\n");
      jsjcAddSP(4*(2+argCount+(parentOnStack?1:0))); // pop off argPtr + all the arguments + funcName + parent
      parentOnStack = false;
      jsjcPush(0, JSJVT_JSVAR); // push return value from jspeFunctionCall (FIXME: can we be sure this isn't a NAME so use JSJVT_JSVAR_NO_NAME? I think so)
      DEBUG_JIT("; FUNCTION CALL end\n");
      // 'parent', 'funcName' and all args are unlocked by _jsjxFunctionCallAndUnLock
    }
    // Check for another '.'/etc after this function
    parentOnStack = jsjFactorMember();
  }
  if ((jit.phase == JSJP_EMIT) && parentOnStack) {
    jsjPopAsVar(0); // remove parent from the stack and unlock it
    jsjcCall(jsvUnLock);
  }
}

void __jsjPostfixExpression() {
  while (lex->tk==LEX_PLUSPLUS || lex->tk==LEX_MINUSMINUS) {
    int op = lex->tk; // POSFIX expression =>  i++, i--
    JSP_ASSERT_MATCH(op);
    if (jit.phase == JSJP_EMIT) {
      jsjPopAsVar(0); // old value -> r0
      jsjcLiteral32(1, op==LEX_PLUSPLUS ? '+' : '-'); // add the operation
      jsjcCall(_jsxPostfixIncDec); // JsVar *_jsxPostfixIncDec(JsVar *var, char op)
      jsjcPush(0, JSJVT_JSVAR_NO_NAME); // push result (value BEFORE we inc/dec)
    }
  }
}

void jsjPostfixExpression() {
  if (lex->tk==LEX_PLUSPLUS || lex->tk==LEX_MINUSMINUS) {
    // PREFIX expression =>  ++i, --i
    int op = lex->tk;
    JSP_ASSERT_MATCH(op);
    jsjPostfixExpression(); // recurse to get our var...
    if (jit.phase == JSJP_EMIT) {
      jsjPopAsVar(0); // old value -> r0
      jsjcLiteral32(1, op==LEX_PLUSPLUS ? '+' : '-'); // add the operation
      jsjcCall(_jsxPrefixIncDec); // JsVar *_jsxPrefixIncDec(JsVar *var, char op)
      jsjcPush(0, JSJVT_JSVAR); // push result (value AFTER we inc/dec) - this is STILL a NAME
    }
  } else
    jsjFactorFunctionCall();
  __jsjPostfixExpression();
}

void jsjUnaryExpression() {
  if (lex->tk=='!' || lex->tk=='~' || lex->tk=='-' || lex->tk=='+') {
    int op = lex->tk;
    JSP_ASSERT_MATCH(op);
    jsjUnaryExpression();
    if (jit.phase == JSJP_EMIT) {
      jsjPopNoName(0); // value -> r0 (but ensure it's not a name)
      if (op=='!') { // logical not
        jsjcCall(jsvGetBoolAndUnLock);
        jsjcMVN(0,0); // ~
        jsjcLiteral32(1, 1);
        jsjcAND(0,1); // &1   -> convert it back to a boolean
        jsjcCall(jsvNewFromBool);
      } else if (op=='~') { // bitwise not
        jsjcCall(jsvGetIntegerAndUnLock);
        jsjcMVN(0,0); // ~
        jsjcCall(jsvNewFromInteger);
      } else if (op=='-') { // unary minus
        jsjcCall(jsvNegateAndUnLock);
      } else if (op=='+') { // unary plus (convert to number)
        jsjcCall(jsvAsNumberAndUnLock);
      } else assert(0);
      jsjcPush(0, JSJVT_JSVAR_NO_NAME); // a value, not a NAME
    }
  } else
    jsjPostfixExpression();
}

// Get the precedence of a BinaryExpression - or return 0 if not one
unsigned int jsjGetBinaryExpressionPrecedence(int op) {
  switch (op) {
  case LEX_OROR: return 1; break;
  case LEX_ANDAND: return 2; break;
  case '|' : return 3; break;
  case '^' : return 4; break;
  case '&' : return 5; break;
  case LEX_EQUAL:
  case LEX_NEQUAL:
  case LEX_TYPEEQUAL:
  case LEX_NTYPEEQUAL: return 6;
  case LEX_LEQUAL:
  case LEX_GEQUAL:
  case '<':
  case '>':
  case LEX_R_INSTANCEOF: return 7;
  case LEX_R_IN: return (execInfo.execute&EXEC_FOR_INIT)?0:7;
  case LEX_LSHIFT:
  case LEX_RSHIFT:
  case LEX_RSHIFTUNSIGNED: return 8;
  case '+':
  case '-': return 9;
  case '*':
  case '/':
  case '%': return 10;
  default: return 0;
  }
}

void __jsjBinaryExpression(unsigned int lastPrecedence) {
  /* This one's a bit strange. Basically all the ops have their own precedence, it's not
   * like & and | share the same precedence. We don't want to recurse for each one,
   * so instead we do this.
   *
   * We deal with an expression in recursion ONLY if it's of higher precedence
   * than the current one, otherwise we stick in the while loop.
   */
  unsigned int precedence = jsjGetBinaryExpressionPrecedence(lex->tk);
  while (precedence && precedence>lastPrecedence) {
    int op = lex->tk;
    if (op==LEX_R_IN) {
      JSP_MATCH(LEX_EOF); // not supported yet
      return;
    }
    JSP_ASSERT_MATCH(op);
    // if we have short-circuit ops, then if we know the outcome
    // we don't bother to execute the other op. Even if not
    // we need to tell mathsOp it's an & or |
    if (op==LEX_ANDAND || op==LEX_OROR) {

      if (jit.phase == JSJP_EMIT) {
        DEBUG_JIT("; shortcitcuit (&&,||)  - first arg just parsed\n");
        DEBUG_JIT("; shortcitcuit compare\n");
        jsjPopNoName(0); // value -> r0 (but ensure it's not a name)
        jsjcPush(0, JSJVT_JSVAR_NO_NAME); // put value back
        jsjcCall(jsvGetBool); // now we have it as a boolean in r0
        jsjcCompareImm(0,  0); // compare with 0
      }
      // parse second argument
      JsVar *oldBlock = jsjcStartBlock();
      if (jit.phase == JSJP_EMIT) {
        DEBUG_JIT("; shortcitcuit parse second block\n");
        jsjPopAndUnLock(); // throw away result from first block!
      }
      jsjUnaryExpression();
      __jsjBinaryExpression(precedence);
      JsVar *secondBlock = jsjcStopBlock(oldBlock);
      if (jit.phase == JSJP_EMIT) {
        DEBUG_JIT("; shortcitcuit jump\n");
        // if false, jump after true block (if an 'else' we need to jump over the jsjcBranchRelative
        jsjcBranchConditionalRelative((op==LEX_ANDAND) ? JSJAC_EQ : JSJAC_NE, jsvGetStringLength(secondBlock), JSJC_NONE);
        DEBUG_JIT("; shortcitcuit second block\n");
        jsjcEmitBlock(secondBlock);
        DEBUG_JIT("; shortcitcuit end\n");
      }
      jsvUnLock(secondBlock);
    } else { // else it's a more 'normal' logical expression - just use Maths
      jsjUnaryExpression();
      __jsjBinaryExpression(precedence);
     /*
      if (op==LEX_R_IN) {
        JsVar *av = jsvSkipName(a); // needle
        JsVar *bv = jsvSkipName(b); // haystack
        if (jsvHasChildren(bv)) { // search keys, NOT values
          av = jsvAsArrayIndexAndUnLock(av);
          JsVar *varFound = jspGetVarNamedField( bv, av, true);
          jsvUnLock2(a,varFound);
          a = jsvNewFromBool(varFound!=0);
        } else { // else maybe it's a fake object...
          const JswSymList *syms = jswGetSymbolListForObjectProto(bv);
          if (syms) {
            JsVar *varFound = 0;
            char nameBuf[JSLEX_MAX_TOKEN_LENGTH];
            if (jsvGetString(av, nameBuf, sizeof(nameBuf)) < sizeof(nameBuf))
              varFound = jswBinarySearch(syms, bv, nameBuf);
            bool found = varFound!=0;
            jsvUnLock2(a, varFound);
            if (!found && jsvIsArrayBuffer(bv)) {
              JsVarFloat f = jsvGetFloat(av); // if not a number this will be NaN, f==floor(f) fails
              if (f==floor(f) && f>=0 && f<jsvGetArrayBufferLength(bv))
                found = true;
            }
            a = jsvNewFromBool(found);
          } else { // not built-in, just assume we can't do it
            jsExceptionHere(JSET_ERROR, "Can't use 'in' operator to search a %t", bv);
            jsvUnLock(a);
            a = 0;
          }
        }
        jsvUnLock2(av, bv);
      } else */if (jit.phase == JSJP_EMIT) {  // --------------------------------------------- NORMAL
        jsjPopAsVar(1); // b -> r1
        jsjPopAsVar(0); // a -> r0
        jsjcLiteral8(2, op);
        jsjcCall(_jsxMathsOpSkipNamesAndUnLock); // unlocks arguments
        jsjcPush(0, JSJVT_JSVAR_NO_NAME); // push result - a value, not a NAME
      }
    }
    precedence = jsjGetBinaryExpressionPrecedence(lex->tk);
  }
}

void jsjBinaryExpression() {
  jsjUnaryExpression();
  __jsjBinaryExpression(0);
}

void jsjConditionalExpression() {
  jsjBinaryExpression();
  if (lex->tk=='?') {
    JSP_ASSERT_MATCH('?');
    if (jit.phase == JSJP_EMIT) {
      /* we handle the condition here because it means the stack level is
      then the same when we capture the true/false blocks as when we emit them */
      DEBUG_JIT("; ternary condition\n");
      jsjPopAsBool(0);
      jsjcCompareImm(0, 0);
    }
    DEBUG_JIT_EMIT("; capture ternary true block\n");
    JsVar *oldBlock = jsjcStartBlock();
    jsjAssignmentExpression();
    if (jit.phase == JSJP_EMIT) jsjPopNoName(0); // we pop to r0 here so we can push after and avoid confusing the stack size checker
    JsVar *trueBlock = jsjcStopBlock(oldBlock);
    JSP_MATCH(':');
    DEBUG_JIT_EMIT("; capture ternary false block\n");
    oldBlock = jsjcStartBlock();
    jsjAssignmentExpression();
    if (jit.phase == JSJP_EMIT) jsjPopNoName(0); // we pop to r0 here so we can push after and avoid confusing the stack size checker
    JsVar *falseBlock = jsjcStopBlock(oldBlock);
    // true block has a jump at the end which depends on the length of the false block!
    int trueBlockLen = jsvGetStringLength(trueBlock) + jsjcGetBranchRelativeLength(jsvGetStringLength(falseBlock));
    if (jit.phase == JSJP_EMIT) {
      DEBUG_JIT("; ternary jump after condition\n");
      // if false, jump after true block (if an 'else' we need to jump over the jsjcBranchRelative
      jsjcBranchConditionalRelative(JSJAC_EQ, trueBlockLen, JSJC_NONE);
      DEBUG_JIT("; ternary true block\n");
      jsjcEmitBlock(trueBlock);
      jsjcBranchRelative(jsvGetStringLength(falseBlock), JSJC_NONE); // jump over false block
      DEBUG_JIT("; ternary false block\n");
      jsjcEmitBlock(falseBlock);
      DEBUG_JIT("; ternary end\n");
      jsjcPush(0, JSJVT_JSVAR); // push the result (LHS) back on
    }
    jsvUnLock2(trueBlock,falseBlock);
  }
}

NO_INLINE void jsjAssignmentExpression() {
  // parse LHS
  jsjConditionalExpression();
  if (!JSJ_PARSING) return;
  if (lex->tk=='=' || lex->tk==LEX_PLUSEQUAL || lex->tk==LEX_MINUSEQUAL ||
      lex->tk==LEX_MULEQUAL || lex->tk==LEX_DIVEQUAL || lex->tk==LEX_MODEQUAL ||
      lex->tk==LEX_ANDEQUAL || lex->tk==LEX_OREQUAL ||
      lex->tk==LEX_XOREQUAL || lex->tk==LEX_RSHIFTEQUAL ||
      lex->tk==LEX_LSHIFTEQUAL || lex->tk==LEX_RSHIFTUNSIGNEDEQUAL) {

    int op = lex->tk;
    JSP_ASSERT_MATCH(op);

    jsjAssignmentExpression();
    if (jit.phase == JSJP_EMIT) {
      jsjPopAsVar(1); // pop RHS to r1
      jsjPopAsVar(0); // pop LHS to r0
      if (op=='=') {
        // this is like jsvReplaceWithOrAddToRoot but it unlocks the RHS for us
        jsjcCall(_jsxAssignment); // JsVar *_jsxAssignment(JsVar *dst, JsVar *src)
      } else {
        if (op==LEX_PLUSEQUAL) op='+';
        else if (op==LEX_MINUSEQUAL) op='-';
        else if (op==LEX_MULEQUAL) op='*';
        else if (op==LEX_DIVEQUAL) op='/';
        else if (op==LEX_MODEQUAL) op='%';
        else if (op==LEX_ANDEQUAL) op='&';
        else if (op==LEX_OREQUAL) op='|';
        else if (op==LEX_XOREQUAL) op='^';
        else if (op==LEX_RSHIFTEQUAL) op=LEX_RSHIFT;
        else if (op==LEX_LSHIFTEQUAL) op=LEX_LSHIFT;
        else if (op==LEX_RSHIFTUNSIGNEDEQUAL) op=LEX_RSHIFTUNSIGNED;
        else assert(0);
        jsjcLiteral8(2, op);
        jsjcCall(_jsxMathAssignment); // JsVar *_jsxMathAssignment(JsVar *var, JsVar *rhs, char op)
      }
      jsjcPush(0, JSJVT_JSVAR); // push the result (LHS) back on
    }
  }
}

// ',' is allowed to add multiple expressions, this is not allowed in jsjAssignmentExpression
void jsjExpression() {
  while (JSJ_PARSING) {
    jsjAssignmentExpression();
    if (lex->tk!=',') return;
    // if we get a comma, we just unlock this data and parse the next bit...
    if (jit.phase == JSJP_EMIT)
      jsjPopAndUnLock();
    JSP_ASSERT_MATCH(',');
  }
}

// Returns true if isFunctionRoot and we had a RETURN statement (so we definitely already returned;
bool jsjBlockNoBrackets() {
  // if isFunctionRoot we're at the root scope
  bool hadReturn = false;
  while (lex->tk && lex->tk!='}' && JSJ_PARSING) {
    if (lex->tk == LEX_R_RETURN)
      hadReturn = true;
    jsjStatement();
  }
  return hadReturn;
}

void jsjBlock() {
  JSP_MATCH('{');
  jsjBlockNoBrackets();
  JSP_MATCH('}');
}

void jsjStatementVar() {
  assert(lex->tk==LEX_R_VAR || lex->tk==LEX_R_LET || lex->tk==LEX_R_CONST);
  // FIXME: Ignore block scoping for now
  LEX_TYPES declType = lex->tk;
  jslGetNextToken();
  bool hasComma = true; // for first time in loop
  while (hasComma && lex->tk == LEX_ID && JSJ_PARSING) {
    // Get the name
    JsVar *name = jslGetTokenValueAsVar();
    JSP_ASSERT_MATCH(LEX_ID);
    bool hasInitialiser = lex->tk == '=';
    /* create the variable locally, and in our var table. If we're emitting now
    and there's no initial value, we don't need to do anything */
    if (hasInitialiser || jit.phase != JSJP_EMIT)
      jsjFactorIDAndUnLock(name, declType);
    if (hasInitialiser) { // sort out initialiser
      DEBUG_JIT_EMIT("; Variable's initialiser\n");
      JSP_ASSERT_MATCH('=');
      jsjAssignmentExpression();
      if (jit.phase == JSJP_EMIT) {
        // _jsxVarInitialAssign(r0:var, r1:isConstant, r2:initialValue)
        jsjcLiteral8(1, (declType==LEX_R_CONST)?1:0); // r1 -> if we're a constant
        jsjPopAsVar(2); // r2 -> initial value
        jsjPopAsVar(0); // r0 -> variable (from jsjFactorIDAndUnLock)
        jsjcCall(_jsxVarInitialAssign); // set the var's initial value
      }
    }
    hasComma = lex->tk == ',';
    if (hasComma) JSP_ASSERT_MATCH(',');
  }
}

void jsjStatementIf() {
  JSP_ASSERT_MATCH(LEX_R_IF);
  DEBUG_JIT_EMIT("; IF condition\n");
  JSP_MATCH('(');
  jsjExpression();
  if (jit.phase == JSJP_EMIT) {
    jsjPopAsBool(0);
    jsjcCompareImm(0, 0);
  }
  JSP_MATCH(')');

  DEBUG_JIT_EMIT("; capture IF true block\n");
  JsVar *oldBlock = jsjcStartBlock();
  jsjBlockOrStatement();
  JsVar *trueBlock = jsjcStopBlock(oldBlock);
  JsVar *falseBlock = 0;

  if (lex->tk==LEX_R_ELSE) {
    JSP_ASSERT_MATCH(LEX_R_ELSE);
    DEBUG_JIT_EMIT("; capture IF false block\n");
    oldBlock = jsjcStartBlock();
    jsjBlockOrStatement();
    falseBlock = jsjcStopBlock(oldBlock);
  }
  if (jit.phase == JSJP_EMIT) {
    DEBUG_JIT("; IF jump after condition\n");
    // if false, jump after true block (if an 'else' we need to jump over the jsjcBranchRelative
    // true block has a jump at the end (if an 'else') and the size of that jump instr can change
    int trueBlockLen = jsvGetStringLength(trueBlock) + (falseBlock?jsjcGetBranchRelativeLength(jsvGetStringLength(falseBlock)):0);
    jsjcBranchConditionalRelative(JSJAC_EQ, trueBlockLen, JSJC_NONE);
    DEBUG_JIT("; IF true block\n");
    jsjcEmitBlock(trueBlock);
    if (falseBlock) {
      jsjcBranchRelative(jsvGetStringLength(falseBlock), JSJC_NONE); // jump over false block
      DEBUG_JIT("; IF false block\n");
      jsjcEmitBlock(falseBlock);
    }
    DEBUG_JIT("; IF end\n");
  }
  jsvUnLock2(trueBlock,falseBlock);
}

void jsjStatementFor() {
  JSP_ASSERT_MATCH(LEX_R_FOR);
  JSP_MATCH('(');
  // we could have 'for (;;)' - so don't munch up our semicolon if that's all we have
  // Parse initialiser - we always run this so march right in and create code
  DEBUG_JIT_EMIT("; FOR initialiser\n");
  if (lex->tk != ';')
    jsjStatement();
  JSP_MATCH(';');
  // Condition - we run this first time, so we go straight through here, but save the position so we can jump back here
  // after the main loop
  int codePosCondition = jsjcGetByteCount();
  DEBUG_JIT_EMIT("; FOR condition\n");
  if (lex->tk != ';') {
    jsjExpression(); // condition
    if (jit.phase == JSJP_EMIT) {
      jsjPopAsBool(0);
      jsjcCompareImm(0, 0);
    }
    // We add a jump to the end after we've parsed everything and know the size
  }
  JSP_MATCH(';');
  DEBUG_JIT_EMIT("; Parsing FOR Iterator block\n");
  JsVar *oldBlock = jsjcStartBlock();
  if (lex->tk != ')')  { // we could have 'for (;;)'
    jsjExpression(); // iterator
    if (jit.phase == JSJP_EMIT) {
      jsjPopAndUnLock();
    }
  }
  JsVar *iteratorBlock = jsjcStopBlock(oldBlock);
  JSP_MATCH(')'); // FIXME: clean up on exit
  // Now parse the actual code to execute
  DEBUG_JIT_EMIT("; Parsing FOR Main block\n");
  oldBlock = jsjcStartBlock();
  jsjBlockOrStatement();
  JsVar *mainBlock = jsjcStopBlock(oldBlock);
  DEBUG_JIT_EMIT("; Branch OVER main block to END\n");
  // Now figure out the jump length and jump (if condition is false)
  if (jit.phase == JSJP_EMIT) {
    jsjcBranchConditionalRelative(JSJAC_EQ, jsvGetStringLength(iteratorBlock) + jsvGetStringLength(mainBlock) + 4, JSJC_FORCE_4BYTE);
    DEBUG_JIT_EMIT("; FOR Main block\n");
    jsjcEmitBlock(mainBlock);
    DEBUG_JIT_EMIT("; FOR Iterator block\n");
    jsjcEmitBlock(iteratorBlock);
    // after the iterator, jump back to condition
    DEBUG_JIT_EMIT("; FOR jump back to condition\n");
    jsjcBranchRelative(codePosCondition - (jsjcGetByteCount()+4), JSJC_FORCE_4BYTE);
    DEBUG_JIT_EMIT("; FOR end\n");
  }
  jsvUnLock2(mainBlock, iteratorBlock);
}

void jsjStatementDoOrWhile(bool isWhile) {
  int codePosStart = jsjcGetByteCount();
  if (isWhile) { // while loop
    JSP_ASSERT_MATCH(LEX_R_WHILE);
    DEBUG_JIT_EMIT("; WHILE condition\n");
    JSP_MATCH('(');
    jsjExpression();
    if (jit.phase == JSJP_EMIT) {
      // do this here so our stack counter stays at the right level
      jsjPopAsBool(0);
      jsjcCompareImm(0, 0);
    }
    JSP_MATCH(')');
    DEBUG_JIT_EMIT("; Parsing WHILE main block\n");
    JsVar *oldBlock = jsjcStartBlock();
    jsjBlockOrStatement();
    JsVar *mainBlock = jsjcStopBlock(oldBlock);
    if (jit.phase == JSJP_EMIT) {
      DEBUG_JIT_EMIT("; WHILE condition jump\n");
      jsjcBranchConditionalRelative(JSJAC_EQ, jsvGetStringLength(mainBlock) + 4, JSJC_FORCE_4BYTE);
      DEBUG_JIT_EMIT("; WHILE Main block\n");
      jsjcEmitBlock(mainBlock);
      DEBUG_JIT_EMIT("; WHILE jump back to condition\n");
      jsjcBranchRelative(codePosStart - (jsjcGetByteCount()+4), JSJC_FORCE_4BYTE);
    }
    jsvUnLock(mainBlock);
  } else { // do..while loop
    JSP_ASSERT_MATCH(LEX_R_DO);
    DEBUG_JIT_EMIT("; DO Main block\n");
    jsjBlockOrStatement();
    JSP_ASSERT_MATCH(LEX_R_WHILE);
    DEBUG_JIT_EMIT("; DO condition\n");
    JSP_MATCH('(');
    jsjExpression();
    JSP_MATCH(')');
    if (jit.phase == JSJP_EMIT) {
      jsjPopAsBool(0);
      jsjcCompareImm(0, 0);
      jsjcBranchConditionalRelative(JSJAC_NE, codePosStart - (jsjcGetByteCount()+4), JSJC_FORCE_4BYTE);
    }
  }
}

void jsjStatement() {
  if (lex->tk==LEX_ID ||
      lex->tk==LEX_INT ||
      lex->tk==LEX_FLOAT ||
      lex->tk==LEX_STR ||
      lex->tk==LEX_TEMPLATE_LITERAL ||
      lex->tk==LEX_REGEX ||
      lex->tk==LEX_R_NEW ||
      lex->tk==LEX_R_NULL ||
      lex->tk==LEX_R_UNDEFINED ||
      lex->tk==LEX_R_TRUE ||
      lex->tk==LEX_R_FALSE ||
      lex->tk==LEX_R_THIS ||
      lex->tk==LEX_R_DELETE ||
      lex->tk==LEX_R_TYPEOF ||
      lex->tk==LEX_R_VOID ||
      lex->tk==LEX_R_SUPER ||
      lex->tk==LEX_PLUSPLUS ||
      lex->tk==LEX_MINUSMINUS ||
      lex->tk=='!' ||
      lex->tk=='-' ||
      lex->tk=='+' ||
      lex->tk=='~' ||
      lex->tk=='[' ||
      lex->tk=='(') {
    /* Execute a simple statement that only contains basic arithmetic... */
    jsjExpression();
    if (jit.phase == JSJP_EMIT)
      jsjPopAndUnLock();
  } else if (lex->tk=='{') {
    /* A block of code */
    jsjBlock();
  } else if (lex->tk==';') {
    JSP_ASSERT_MATCH(';');/* Empty statement - to allow things like ;;; */
  } else if (lex->tk==LEX_R_VAR ||
            lex->tk==LEX_R_LET ||
            lex->tk==LEX_R_CONST) {
    return jsjStatementVar();
  } else if (lex->tk==LEX_R_IF) {
    return jsjStatementIf();
  } else if (lex->tk==LEX_R_DO || lex->tk==LEX_R_WHILE) {
    return jsjStatementDoOrWhile(lex->tk==LEX_R_WHILE);
  } else if (lex->tk==LEX_R_FOR) {
    return jsjStatementFor();
  /*} else if (lex->tk==LEX_R_TRY) {
    return jsjStatementTry();*/
  } else if (lex->tk==LEX_R_RETURN) {
    JSP_ASSERT_MATCH(LEX_R_RETURN);
    if (lex->tk != ';' && lex->tk != '}') {
      jsjExpression();
      DEBUG_JIT_EMIT("; RETURN r0\n");
      if (jit.phase == JSJP_EMIT) jsjPopNoName(0); // a -> r0, we only want the value, so skip the name if there was one
    } else {
      DEBUG_JIT_EMIT("; RETURN undefined\n");
      if (jit.phase == JSJP_EMIT) jsjcLiteral32(0, 0);
    }
    if (jit.phase == JSJP_EMIT) jsjFunctionReturn(true/*isReturnStatement*/);
/*} else if (lex->tk==LEX_R_THROW) {
  } else if (lex->tk==LEX_R_FUNCTION) {
  } else if (lex->tk==LEX_R_CONTINUE) {
    JSP_ASSERT_MATCH(LEX_R_CONTINUE);
  } else if (lex->tk==LEX_R_BREAK) {
    JSP_ASSERT_MATCH(LEX_R_BREAK);
    if (JSP_SHOULD_EXECUTE) {
  } else if (lex->tk==LEX_R_SWITCH) {
    return jsjStatementSwitch();*/
  } else JSP_MATCH(LEX_EOF);
}

void jsjBlockOrStatement() {
  if (lex->tk=='{') {
    jsjBlock();
  } else {
    jsjStatement();
    if (lex->tk==';') JSP_ASSERT_MATCH(';');
    // FIXME pop?
  }
}

JsVar *jsjParseFunction() {
  JsExecFlags oldExec = execInfo.execute;
  jsjcStart();
  // FIXME: I guess we need to create a function execution scope and unpack parameters?
  // Maybe we could use jspeFunctionCall to do all this for us (not creating a native function but a 'normal' one
  // with native function code...
  // Function init code
  jsjFunctionStart();
  // Parse the function
  size_t codeStartPosition = lex->tokenStart; // otherwise we include 'jit' too!
  jit.phase = JSJP_SCAN; DEBUG_JIT("; ============ SCAN PHASE\n");
  jsjBlockNoBrackets();
  if (JSJ_PARSING) { // if no error, re-parse and create code
    jslSeekTo(codeStartPosition);
    jit.phase = JSJP_EMIT; DEBUG_JIT("; ============ EMIT PHASE\n");
    bool hadReturnStatement = jsjBlockNoBrackets(true);
    // if this block had a return in it (eg not behind 'if'/etc), hadReturnStatement=true
    // if so, we can skip adding a return statement
    if (!hadReturnStatement) {
      DEBUG_JIT_EMIT("; END of function - return undefined\n");
      jsjcLiteral32(0, 0);
      jsjFunctionReturn(false/*isReturnStatement*/);
    } else {
      jit.stackDepth -= jit.varCount; // jsjFunctionReturn would have pulled these off the stack anyway
    }
  }
  JsVar *v = jsjcStop();
  JsVar *exception = jspGetException();
  execInfo.execute = oldExec; // restore exec state
  if (!exception) return v;
  // We had an error - don't return half-complete code
  jsiConsolePrintf("JIT %v\n", exception);
  if (jsvIsObject(exception)) {
    JsVar *stackTrace = jsvObjectGetChildIfExists(exception, "stack");
    if (stackTrace) {
      jsiConsolePrintStringVar(stackTrace);
      jsvUnLock(stackTrace);
    }
  }
  jsvUnLock2(exception, v);
  return 0;
}

JsVar *jsjEvaluateVar(JsVar *str) {
  JsLex lex;
  assert(jsvIsString(str));
  JsLex *oldLex = jslSetLex(&lex);
  jslInit(str);
  jsjcStart();
  // Function init code
  jsjFunctionStart();
  // Parse the expression
  JslCharPos codeStartPosition;
  jslCharPosFromLex(&codeStartPosition);
  jit.phase = JSJP_SCAN;
  jsjExpression();
  if (JSJ_PARSING) { // if no error, re-parse and create code
    jslSeekToP(&codeStartPosition);
    jslCharPosFree(&codeStartPosition);
    jit.phase = JSJP_EMIT;
    jsjExpression();
    jsjPopNoName(0); // a -> r0, we only want the value, so skip the name if there was one
    jsjFunctionReturn(false/*isReturnStatement*/);
  }
  JsVar *v = jsjcStop();
  jslKill();
  jslSetLex(oldLex);
  return v;
}

JsVar *jsjEvaluate(const char *str) {
  JsVar *evCode;
  evCode = jsvNewNativeString((char*)str, strlen(str));
  if (!evCode) return 0;
  JsVar *v = 0;
  if (!jsvIsMemoryFull())
    v = jsjEvaluateVar(evCode);
  jsvUnLock(evCode);

  return v;
}

#endif /*#ifdef ESPR_JIT*/
