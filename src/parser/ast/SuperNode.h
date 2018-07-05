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

#ifndef SuperNode_h
#define SuperNode_h

namespace Escargot {

class SuperNode : public Node {
public:
    friend class ScriptParser;
    SuperNode(bool isCall = false)
        : Node()
    {
        m_isCall = isCall;
    }

    virtual ASTNodeType type() { return ASTNodeType::Super; }
    virtual void generateExpressionByteCode(ByteCodeBlock *codeBlock, ByteCodeGenerateContext *context, ByteCodeRegisterIndex dstRegister)
    {
        Context *ctx = codeBlock->m_codeBlock->context();


        if (m_isCall) {
            size_t reg = context->getRegister();
            codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), REGULAR_REGISTER_LIMIT, reg, ctx->staticStrings().__proto__), context, this);
            codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), reg, reg, ctx->staticStrings().constructor), context, this);
            codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), reg, reg, ctx->staticStrings().__proto__), context, this);
            codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), reg, dstRegister, ctx->staticStrings().call), context, this);
        } else {
            context->giveUpRegister();
            // TODO Cannot identify super class name or current class name. Cannot retreive from this because it has differrent value in static method and proto method.
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

public:
    bool m_isCall;
};
}

#include "Node.h"
#endif
