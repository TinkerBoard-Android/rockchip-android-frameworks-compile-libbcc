/*
 * Copyright 2010, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Bitcode compiler (bcc) for Android:
//    This is an eager-compilation JIT running on Android.

#define LOG_TAG "bcc"
#include <cutils/log.h>

#include <bcc/bcc.h>

#include "bcc_compiler.h"

#include <utils/StopWatch.h>


namespace llvm {
  class Module;
}


namespace bcc {

  struct BCCscript {
    //////////////////////////////////////////////////////////////////////////
    // Part I. Compiler
    //////////////////////////////////////////////////////////////////////////
    Compiler compiler;

    void registerSymbolCallback(BCCSymbolLookupFn pFn, BCCvoid *pContext) {
      compiler.registerSymbolCallback(pFn, pContext);
    }

    //////////////////////////////////////////////////////////////////////////
    // Part II. Logistics & Error handling
    //////////////////////////////////////////////////////////////////////////
    BCCscript() {
      bccError = BCC_NO_ERROR;
    }

    ~BCCscript() {
    }

    void setError(BCCenum error) {
      if (bccError == BCC_NO_ERROR && error != BCC_NO_ERROR) {
        bccError = error;
      }
    }

    BCCenum getError() {
      BCCenum result = bccError;
      bccError = BCC_NO_ERROR;
      return result;
    }

    BCCenum bccError;
  };


  extern "C" BCCscript *bccCreateScript() {
    return new BCCscript();
  }

  extern "C" BCCenum bccGetError(BCCscript *script) {
    return script->getError();
  }

  extern "C" void bccDeleteScript(BCCscript *script) {
    delete script;
  }

  extern "C" void bccRegisterSymbolCallback(BCCscript *script,
                                            BCCSymbolLookupFn pFn,
                                            BCCvoid *pContext) {
    script->registerSymbolCallback(pFn, pContext);
  }

  extern "C" int bccReadModule(BCCscript *script, BCCvoid *module) {
    return script->compiler.readModule(reinterpret_cast<llvm::Module*>(module));
  }

  extern "C" int bccReadBC(BCCscript *script,
                           const BCCchar *bitcode,
                           BCCint size,
                           const BCCchar *resName) {
    return script->compiler.readBC(bitcode, size, resName);
  }

  extern "C" void bccLinkBC(BCCscript *script,
                            const BCCchar *bitcode,
                            BCCint size) {
    script->compiler.linkBC(bitcode, size);
  }

  extern "C" void bccLoadBinary(BCCscript *script) {
    int result = script->compiler.loadCacheFile();
    if (result)
      script->setError(BCC_INVALID_OPERATION);
  }

  extern "C" void bccCompileBC(BCCscript *script) {
    {
#if defined(__arm__)
      android::StopWatch compileTimer("RenderScript compile time");
#endif
      int result = script->compiler.compile();
      if (result)
        script->setError(BCC_INVALID_OPERATION);
    }
  }

  extern "C" void bccGetScriptInfoLog(BCCscript *script,
                                      BCCsizei maxLength,
                                      BCCsizei *length,
                                      BCCchar *infoLog) {
    char *message = script->compiler.getErrorMessage();
    int messageLength = strlen(message) + 1;
    if (length)
      *length = messageLength;

    if (infoLog && maxLength > 0) {
      int trimmedLength = maxLength < messageLength ? maxLength : messageLength;
      memcpy(infoLog, message, trimmedLength);
      infoLog[trimmedLength] = 0;
    }
  }

  extern "C" void bccGetScriptLabel(BCCscript *script,
                                    const BCCchar *name,
                                    BCCvoid **address) {
    void *value = script->compiler.lookup(name);
    if (value)
      *address = value;
    else
      script->setError(BCC_INVALID_VALUE);
  }

  extern "C" void bccGetExportVars(BCCscript *script,
                                   BCCsizei *actualVarCount,
                                   BCCsizei maxVarCount,
                                   BCCvoid **vars) {
    script->compiler.getExportVars(actualVarCount, maxVarCount, vars);
  }

  extern "C" void bccGetExportFuncs(BCCscript *script,
                                    BCCsizei *actualFuncCount,
                                    BCCsizei maxFuncCount,
                                    BCCvoid **funcs) {
    script->compiler.getExportFuncs(actualFuncCount, maxFuncCount, funcs);
  }

  extern "C" void bccGetPragmas(BCCscript *script,
                                BCCsizei *actualStringCount,
                                BCCsizei maxStringCount,
                                BCCchar **strings) {
    script->compiler.getPragmas(actualStringCount, maxStringCount, strings);
  }

  extern "C" void bccGetFunctions(BCCscript *script,
                                  BCCsizei *actualFunctionCount,
                                  BCCsizei maxFunctionCount,
                                  BCCchar **functions) {
    script->compiler.getFunctions(actualFunctionCount,
                                  maxFunctionCount,
                                  functions);
  }

  extern "C" void bccGetFunctionBinary(BCCscript *script,
                                       BCCchar *function,
                                       BCCvoid **base,
                                       BCCsizei *length) {
    script->compiler.getFunctionBinary(function, base, length);
  }

  struct BCCtype {
    const Compiler *compiler;
    const llvm::Type *t;
  };

}  // namespace bcc
