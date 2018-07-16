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
#include "ProxyObject.h"
#include "Context.h"
#include "JobQueue.h"
#include "interpreter/ByteCodeInterpreter.h"

namespace Escargot {

ProxyObject::ProxyObject(ExecutionState& state)
    : Object(state)
    , m_target(nullptr)
    , m_handler(nullptr)
{
}

bool ProxyObject::preventExtensions(ExecutionState& state)
{
    auto strings = &state.context()->staticStrings();
    // 1. If handler is null, throw a TypeError exception.
    if (this->handler() == nullptr) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy handler can be null.");
        return false;
    }

    // 2. Let handler be O.[[ProxyHandler]].
    Value handler(this->handler());

    // 3. Assert: Type(handler) is Object.
    ASSERT(handler.isObject());

    //4. Let target be O.[[ProxyTarget]].
    Value target(this->target());

    //5. Let trap be ? GetMethod(handler, "preventExtensions").
    ObjectPropertyName name = ObjectPropertyName(state, strings->preventExtensions.string());
    ObjectGetResult trapResult = handler.asObject()->get(state, name);

    if (trapResult.hasValue()) {
        Value trap = trapResult.value(state, handler);
        // 6. If trap is undefined, then
        if (trap.isUndefined()) {
            // a. Return ? target.[[PreventExtensions]]().
            return target.asObject()->preventExtensions(state);
        }

        // 7. Let booleanTrapResult be ToBoolean(? Call(trap, handler, « target »)).
        Value arguments[] = { target };
        Value booleanTrapResult = FunctionObject::call(state, trap, handler, 1, arguments);

        // 8. If booleanTrapResult is true, then
        if (booleanTrapResult.isTrue() || !booleanTrapResult.isUndefinedOrNull()) {
            // a. Let targetIsExtensible be ? target.[[IsExtensible]]().
            bool extensibleTarget = target.asObject()->isExtensible(state);
            // b. If targetIsExtensible is true, throw a TypeError exception.
            if (extensibleTarget) {
                ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy Type Error");
                return false;
            }
            // 9. Return booleanTrapResult.
            return true;
        } else {
            // 9. Return booleanTrapResult.
            return false;
        }
    } else {
        return target.asObject()->preventExtensions(state);
    }

    return false;
}

bool ProxyObject::hasProperty(ExecutionState& state, const ObjectPropertyName& propertyName)
{
    auto strings = &state.context()->staticStrings();

    // 1. If handler is null, throw a TypeError exception.
    if (this->handler() == nullptr) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy handler can be null.");
        return false;
    }

    // 2. Let handler be O.[[ProxyHandler]].
    Value handler(this->handler());

    // 3. Assert: Type(handler) is Object.
    ASSERT(handler.isObject());

    // 4. Let target be O.[[ProxyTarget]].
    Value target(this->target());

    // 5. Let trap be ? GetMethod(handler, "has").
    ObjectPropertyName name = ObjectPropertyName(state, strings->has.string());
    ObjectGetResult trapResult = handler.asObject()->get(state, name);

    if (trapResult.hasValue()) {
        Value trap = trapResult.value(state, handler);
        // 6. If trap is undefined, then
        if (trap.isUndefined()) {
            // a. Return ? target.[[HasProperty]](P).
            return target.asObject()->hasProperty(state, propertyName);
        }

        if (trap.isFunction()) {
            // 7. Let booleanTrapResult be ToBoolean(? Call(trap, handler, « target, P »)).
            Value P = propertyName.toPlainValue(state);
            Value arguments[] = { target, P };
            Value booleanTrapResult = FunctionObject::call(state, trap, handler, 2, arguments);

            // 8. If booleanTrapResult is false, then
            if (booleanTrapResult.isFalse() || booleanTrapResult.isUndefinedOrNull()) {
                // a. Let targetDesc be ? target.[[GetOwnProperty]](P).
                ObjectGetResult targetDesc = target.asObject()->getOwnProperty(state, propertyName);
                // b. If targetDesc is not undefined, then
                if (targetDesc.hasValue()) {
                    // i. If targetDesc.[[Configurable]] is false, throw a TypeError exception.
                    if (!targetDesc.isConfigurable()) {
                        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy Type Error");
                        return false;
                    }
                    // ii. Let extensibleTarget be ? IsExtensible(target).
                    bool extensibleTarget = target.asObject()->isExtensible(state);

                    // iii. If extensibleTarget is false, throw a TypeError exception.
                    if (!extensibleTarget) {
                        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy Type Error");
                        return false;
                    }
                }
                // 10. Return booleanTrapResult.
                return false;
            } else {
                // 10. Return booleanTrapResult.
                return true;
            }
        }
    } else {
        return target.asObject()->hasProperty(state, propertyName);
    }

    return false;
}

bool ProxyObject::isExtensible(ExecutionState& state)
{
    auto strings = &state.context()->staticStrings();

    // 1. If handler is null, throw a TypeError exception.
    if (this->handler() == nullptr) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy handler can be null.");
        return false;
    }

    // 2. Let handler be O.[[ProxyHandler]].
    Value handler(this->handler());

    // 3. Assert: Type(handler) is Object.
    ASSERT(handler.isObject());

    // 4. Let target be O.[[ProxyTarget]].
    Value target(this->target());

    // 5. Let trap be ? GetMethod(handler, "isExtensible").
    ObjectPropertyName name = ObjectPropertyName(state, strings->isExtensible.string());
    ObjectGetResult trapResult = handler.asObject()->get(state, name);

    if (trapResult.hasValue()) {
        Value trap = trapResult.value(state, handler);
        // 6. If trap is undefined, then Return ? target.[[IsExtensible]]().
        if (trap.isUndefined()) {
            return target.asObject()->isExtensible(state);
        }
        if (trap.isFunction()) {
            // 7. Let booleanTrapResult be ToBoolean(? Call(trap, handler, « target »)).
            Value arguments[] = { target };
            Value booleanTrapResult = FunctionObject::call(state, trap, handler, 1, arguments);
            bool trapResult = true;
            if (booleanTrapResult.isFalse() || booleanTrapResult.isUndefinedOrNull()) {
                trapResult = false;
            }

            // 8. Let targetResult be ? target.[[IsExtensible]]().
            bool targetResult = target.asObject()->isExtensible(state);

            // 9. If SameValue(booleanTrapResult, targetResult) is false, throw a TypeError exception.
            if (trapResult != targetResult) {
                ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy Type Error");
                return false;
            }

            // 10. Return booleanTrapResult.
            return trapResult;
        } else {
            ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy handler's getPrototypeOf trap wasn't undefined, null, or callable");
            return false;
        }
    } else {
        return target.asObject()->isExtensible(state);
    }

    return false;
}

// https://www.ecma-international.org/ecma-262/8.0/#sec-proxy-object-internal-methods-and-internal-slots-setprototypeof-v
void ProxyObject::setPrototype(ExecutionState& state, const Value& value)
{
    auto strings = &state.context()->staticStrings();

    // 1. Assert: Either Type(V) is Object or Type(V) is Null.
    ASSERT(value.isObject() || value.isNull());

    // 2. If handler is null, throw a TypeError exception.
    if (this->handler() == nullptr) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy handler can be null.");
        return;
    }

    // 3. Let handler be O.[[ProxyHandler]].
    Value handler(this->handler());

    // 4. Assert: Type(handler) is Object.
    ASSERT(handler.isObject());

    // 5. Let target be O.[[ProxyTarget]].
    Value target(this->target());

    // 6. Let trap be ? GetMethod(handler, "setPrototypeOf").
    ObjectPropertyName name = ObjectPropertyName(state, strings->setPrototypeOf.string());
    ObjectGetResult trapResult = handler.asObject()->get(state, name);

    if (trapResult.hasValue()) {
        Value trap = trapResult.value(state, handler);
        // 7. If trap is undefined, then Return ? target.[[SetPrototypeOf]](V).
        if (trap.isUndefined()) {
            target.asObject()->setPrototype(state, value);
        }

        // 8. Let booleanTrapResult be ToBoolean(? Call(trap, handler, « target, V »)).
        Value arguments[] = { target, value };
        Value booleanTrapResult = FunctionObject::call(state, trap, handler, 2, arguments);

        // 9. If booleanTrapResult is false, throw a TypeError exception.
        if (booleanTrapResult.isFalse() || booleanTrapResult.isUndefinedOrNull()) {
            ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy Type Error : trap returned false");
            return;
        }

        // 10. Let extensibleTarget be ? IsExtensible(target).
        bool extensibleTarget = target.asObject()->isExtensible(state);

        // 11. If extensibleTarget is true, return true.
        if (extensibleTarget) {
            return;
        }

        // 12. Let targetProto be ? target.[[GetPrototypeOf]]().
        Value targetProto = target.asObject()->getPrototype(state);

        // 13. If SameValue(V, targetProto) is false, throw a TypeError exception.
        if (value != targetProto) {
            ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy Type Error");
            return;
        }

        // 14. Return true.
        return;
    }

    target.asObject()->setPrototype(state, value);
}

Object* ProxyObject::getPrototypeObject(ExecutionState& state)
{
    if (!this->target()) {
        return nullptr;
    }

    Value result = getPrototype(state);
    if (result.isObject()) {
        return result.asObject();
    }

    return nullptr;
}

// https://www.ecma-international.org/ecma-262/8.0/index.html#sec-proxy-object-internal-methods-and-internal-slots-getprototypeof
Value ProxyObject::getPrototype(ExecutionState& state)
{
    auto strings = &state.context()->staticStrings();

    // 1. If handler is null, throw a TypeError exception.
    if (this->handler() == nullptr) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy handler can be null.");
        return Value();
    }

    // 2. Let handler be O.[[ProxyHandler]].
    Value handler(this->handler());

    // 3. Assert: Type(handler) is Object.
    ASSERT(handler.isObject());

    // 4. Let target be O.[[ProxyTarget]].
    Value target(this->target());

    // 5. Let trap be ? GetMethod(handler, "getPrototypeOf").
    ObjectPropertyName name = ObjectPropertyName(state, strings->getPrototypeOf.string());
    ObjectGetResult trapResult = handler.asObject()->get(state, name);

    if (trapResult.hasValue()) {
        Value trap = trapResult.value(state, handler);
        // 6. If trap is undefined, then
        if (trap.isUndefined()) {
            // a. Return ? target.[[GetPrototypeOf]]().
            return target.asObject()->getPrototype(state);
        }
        // 7. Let handlerProto be ? Call(trap, handler, « target »).
        if (trap.isFunction()) {
            Value arguments[] = { target };
            Value handlerProto = FunctionObject::call(state, trap, handler, 1, arguments);
            // 8. If Type(handlerProto) is neither Object nor Null, throw a TypeError exception.
            if (!handlerProto.isObject() && !handlerProto.isNull()) {
                ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy Type Error");
                return Value();
            }
            // 9. Let extensibleTarget be ? IsExtensible(target).
            bool extensibleTarget = target.asObject()->isExtensible(state);
            // 10. If extensibleTarget is true, return handlerProto.
            if (extensibleTarget) {
                return handlerProto;
            }

            // 11. Let targetProto be ? target.[[GetPrototypeOf]]().
            Value targetProto = target.asObject()->getPrototype(state);
            // 12. If SameValue(handlerProto, targetProto) is false, throw a TypeError exception.
            if (handlerProto != targetProto) {
                ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy Type Error");
                return Value();
            }

            // 13. Return handlerProto.
            return handlerProto;
        } else {
            ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy handler's getPrototypeOf trap wasn't undefined, null, or callable");
            return Value();
        }
    } else {
        return target.asObject()->getPrototype(state);
    }

    return Value();
}

// https://www.ecma-international.org/ecma-262/8.0/index.html#sec-proxy-object-internal-methods-and-internal-slots-get-p-receiver
ObjectGetResult ProxyObject::get(ExecutionState& state, const ObjectPropertyName& propertyName)
{
    auto strings = &state.context()->staticStrings();

    // 1. If handler is null, throw a TypeError exception.
    if (this->handler() == nullptr) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy handler can be null.");
        return ObjectGetResult();
    }

    // 2. Let handler be O.[[ProxyHandler]].
    Value handler(this->handler());

    // 3. Let target be O.[[ProxyTarget]].
    Value target(this->target());

    Value trapResult;
    if (handler.isObject()) {
        Object* obj = handler.asObject();
        ObjectPropertyName get = ObjectPropertyName(state, strings->get.string());
        // 4. Let trap be ? GetMethod(handler, "get").
        ObjectGetResult ret = obj->get(state, get);
        if (ret.hasValue()) {
            Value trap = ret.value(state, handler);
            // 5. If trap is undefined, then Return ? target.[[Get]](P, Receiver).
            if (trap.isUndefined()) {
                return target.asObject()->get(state, propertyName);
            }

            // 6. Let trapResult be ? Call(trap, handler, « target, P, Receiver »).
            if (trap.isFunction()) {
                Value prop = propertyName.toPlainValue(state);
                Value arguments[] = { target, prop, handler };
                trapResult = FunctionObject::call(state, trap, m_handler, 3, arguments);
            } else {
                ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy handler's get trap wasn't undefined, null, or callable");
            }
        } else {
            return target.asObject()->get(state, propertyName);
        }

        // 7. Let targetDesc be ? target.[[GetOwnProperty]](P).
        ObjectGetResult targetDesc = target.asObject()->getOwnProperty(state, propertyName);

        // 8. If targetDesc is not undefined, then
        if (targetDesc.hasValue()) {
            // a. If IsDataDescriptor(targetDesc) is true and targetDesc.[[Configurable]] is false and targetDesc.[[Writable]] is false,
            Value v = targetDesc.value(state, target);
            if (targetDesc.isDataProperty() && !targetDesc.isConfigurable() && !targetDesc.isWritable()) {
                // If SameValue(trapResult, targetDesc.[[Value]]) is false
                if (trapResult != targetDesc.value(state, target)) {
                    ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy Type Error.");
                }
            }
            // b. If IsAccessorDescriptor(targetDesc) is true and targetDesc.[[Configurable]] is false and targetDesc.[[Get]] is undefined,
            if (!targetDesc.isConfigurable() && (!targetDesc.jsGetterSetter()->hasGetter() || targetDesc.jsGetterSetter()->getter().isUndefined())) {
                // i. If trapResult is not undefined, throw a TypeError exception.
                if (!trapResult.isUndefined()) {
                    ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy Type Error.");
                }
            }
        }
    } else {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy Type Error.");
    }

    // 9. Return trapResult.
    return ObjectGetResult(trapResult, true, true, true);
}

// https://www.ecma-international.org/ecma-262/8.0/index.html#sec-proxy-object-internal-methods-and-internal-slots-set-p-v-receiver
bool ProxyObject::set(ExecutionState& state, const ObjectPropertyName& propertyName, const Value& v, const Value& receiver)
{
    auto strings = &state.context()->staticStrings();

    // 1. If handler is null, throw a TypeError exception.
    if (this->handler() == nullptr) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy Type Error.");
        return false;
    }

    // 2. Let handler be O.[[ProxyHandler]].
    Value handler(this->handler());

    // 3. Let target be O.[[ProxyTarget]].
    Value target(this->target());

    if (handler.isObject()) {
        Object* obj = handler.asObject();
        ObjectPropertyName set = ObjectPropertyName(state, strings->set.string());
        // 4. Let trap be ? GetMethod(handler, "set").
        ObjectGetResult ret = obj->get(state, set);
        if (ret.hasValue()) {
            Value trap = ret.value(state, handler);
            // 5. If trap is undefined, then Return ? target.[[Set]](P, V, Receiver).
            if (trap.isUndefined()) {
                return target.asObject()->set(state, propertyName, v, receiver);
            }

            // 6. Let booleanTrapResult be ToBoolean(? Call(trap, handler, « target, P, V, Receiver »)).
            if (trap.isFunction()) {
                Value prop = propertyName.toPlainValue(state);
                Value arguments[] = { target, prop, v, receiver };
                Value booleanTrapResult = FunctionObject::call(state, trap, handler, 4, arguments);
                // 7. If booleanTrapResult is false, return false.
                if (booleanTrapResult.isFalse() || booleanTrapResult.isUndefinedOrNull()) {
                    return false;
                }
            } else {
                ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy handler's set trap wasn't undefined, null, or callable");
                return false;
            }

            // 8. Let targetDesc be ? target.[[GetOwnProperty]](P).
            ObjectGetResult targetDesc = target.asObject()->getOwnProperty(state, propertyName);
            if (targetDesc.hasValue()) {
                // a. If IsDataDescriptor(targetDesc) is true and targetDesc.[[Configurable]] is false and targetDesc.[[Writable]] is false, then
                if (targetDesc.isDataProperty() && !targetDesc.isConfigurable() && !targetDesc.isWritable()) {
                    // i. If SameValue(V, targetDesc.[[Value]]) is false, throw a TypeError exception.
                    if (v != targetDesc.value(state, target)) {
                        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy Type Error.");
                        return false;
                    }
                }

                // b. If IsAccessorDescriptor(targetDesc) is true and targetDesc.[[Configurable]] is false, then
                if (!targetDesc.isConfigurable()) {
                    // i. If targetDesc.[[Set]] is undefined, throw a TypeError exception.
                    if ((!targetDesc.jsGetterSetter()->hasGetter() || targetDesc.jsGetterSetter()->getter().isUndefined())) {
                        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy Type Error.");
                        return false;
                    }
                }
            }
        } else {
            return target.asObject()->set(state, propertyName, v, receiver);
        }
    } else {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, strings->Proxy.string(), false, String::emptyString, "%s: Proxy Type Error.");
        return false;
    }

    return true;
}

void ProxyObject::enumeration(ExecutionState& state, bool (*callback)(ExecutionState& state, Object* self, const ObjectPropertyName&, const ObjectStructurePropertyDescriptor& desc, void* data), void* data, bool shouldSkipSymbolKey)
{
    if (!this->target()) {
        return;
    }

    Object* target = this->target();
    target->enumeration(state, callback, data, shouldSkipSymbolKey);
}
}

#endif // ESCARGOT_ENABLE_PROXY
