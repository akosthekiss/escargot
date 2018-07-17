/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#if ESCARGOT_ENABLE_PROXY

#include "Escargot.h"
#include "GlobalObject.h"
#include "Context.h"
#include "VMInstance.h"
#include "ProxyObject.h"
#include "ArrayObject.h"
#include "JobQueue.h"
#include "SandBox.h"

namespace Escargot {

// $26.2.1 The Proxy Constructor
// https://www.ecma-international.org/ecma-262/8.0/#sec-proxycreate
static Value builtinProxyConstructor(ExecutionState& state, Value thisValue, size_t argc, Value* argv, bool isNewExpression)
{
    auto strings = &state.context()->staticStrings();

    if (!isNewExpression) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: calling a builtin Proxy constructor without new is forbidden");
        return Value();
    }

    Value target = argv[0];
    Value handler = argv[1];

    // 1. If Type(target) is not Object, throw a TypeError exception.
    if (!target.isObject()) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: \'target\' argument of Proxy must be an object");
        return Value();
    }

    // 2. If target is a Proxy exotic object and target.[[ProxyHandler]] is null, throw a TypeError exception.
    if (target.asObject()->isProxyObject()) {
        ProxyObject* exotic = target.asObject()->asProxyObject();
        if (!exotic->handler()) {
            ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: \'target\' Type Error");
            return Value();
        }
    }

    // 3. If Type(handler) is not Object, throw a TypeError exception.
    if (!handler.isObject()) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: \'handler\' argument of Proxy must be an object");
        return Value();
    }

    // 4. If handler is a Proxy exotic object and handler.[[ProxyHandler]] is null, throw a TypeError exception.
    if (handler.asObject()->isProxyObject()) {
        ProxyObject* exotic = handler.asObject()->asProxyObject();
        if (!exotic->handler()) {
            ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: \'handler\' Type Error");
            return Value();
        }
    }

    // 5. Let P be a newly created object.
    ProxyObject* P = new ProxyObject(state);

    // 6. Set P.[[ProxyTarget]] to target.
    P->setTarget(argv[0].asObject());

    // 7. Set P.[[ProxyHandler]] to handler.
    P->setHandler(argv[1].asObject());

    // 8. Return P.
    return P;
}

// https://www.ecma-international.org/ecma-262/8.0/index.html#sec-proxy-revocation-functions
static Value builtinProxyRevoke(ExecutionState& state, Value thisValue, size_t argc, Value* argv, bool isNewExpression)
{
    const StaticStrings* strings = &state.context()->staticStrings();
    if (isNewExpression) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: calling a builtin Proxy constructor with new is forbidden");
        return Value();
    }

    RevocableFunctionObject* F = (RevocableFunctionObject*)state.executionContext()->resolveCallee();

    // 1. Let p be F.[[RevocableProxy]].
    ProxyObject* p = F->revocable();

    // 2. If p is null, return undefined.
    if (p == nullptr) {
        return Value();
    }

    // 3. Set F.[[RevocableProxy]] to null.
    F->setRevocable(nullptr);

    // 4. Set p.[[ProxyTarget]] to null.
    p->setTarget(nullptr);

    // 5. Set p.[[ProxyHandler]] to null.
    p->setHandler(nullptr);

    // 6. Return undefined.
    return Value();
}

// https://www.ecma-international.org/ecma-262/8.0/index.html#sec-proxy.revocable
static Value builtinProxyRevocable(ExecutionState& state, Value thisValue, size_t argc, Value* argv, bool isNewExpression)
{
    const StaticStrings* strings = &state.context()->staticStrings();
    if (isNewExpression) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: calling a builtin Proxy constructor with new is forbidden");
        return Value();
    }
    // 1. Let p be ? ProxyCreate(target, handler).
    Value p = builtinProxyConstructor(state, thisValue, argc, argv, true);

    // 2. Let result be ObjectCreate(%ObjectPrototype%).
    Object* result = new Object(state);
    result->setPrototype(state, state.context()->globalObject()->objectPrototype());
    // 3. Perform CreateDataProperty(result, "proxy", p).
    result->defineOwnPropertyThrowsException(state, ObjectPropertyName(strings->proxy),
                                             ObjectPropertyDescriptor(p, (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));
    ProxyObject* P = p.asObject()->asProxyObject();
    // 4. Perform CreateDataProperty(result, "revoke", revoker).
    result->defineOwnPropertyThrowsException(state, ObjectPropertyName(strings->revoke),
                                             ObjectPropertyDescriptor(new RevocableFunctionObject(state, NativeFunctionInfo(strings->revoke, builtinProxyRevoke, 0, nullptr, NativeFunctionInfo::Strict), P),
                                                                      (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));

    return result;
}

void GlobalObject::installProxy(ExecutionState& state)
{
    const StaticStrings* strings = &state.context()->staticStrings();
    m_proxy = new FunctionObject(state, NativeFunctionInfo(strings->Proxy, builtinProxyConstructor, 2, [](ExecutionState& state, CodeBlock* codeBlock, size_t argc, Value* argv) -> Object* {
                                     return new Object(state);
                                 }),
                                 FunctionObject::__ForBuiltin__);
    m_proxy->markThisObjectDontNeedStructureTransitionTable(state);
    m_proxy->setPrototype(state, m_functionPrototype);
    // 26.2.2.1 Implement revocable
    m_proxy->defineOwnPropertyThrowsException(state, ObjectPropertyName(strings->revocable),
                                              ObjectPropertyDescriptor(new FunctionObject(state, NativeFunctionInfo(strings->revocable, builtinProxyRevocable, 2, nullptr, NativeFunctionInfo::Strict)),
                                                                       (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));

    defineOwnProperty(state, ObjectPropertyName(strings->Proxy),
                      ObjectPropertyDescriptor(m_proxy, (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));
}
}
#endif // ESCARGOT_ENABLE_PROXY
