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
    enum Kind {
        Undefined,
        Access,
        Assign,
        Constructor
    };

    SuperNode(Kind kind = Undefined, bool inStatic = false)
        : Node()
    {
        m_kind = kind;
        m_inStatic = inStatic;
    }

    virtual ASTNodeType type() { return ASTNodeType::Super; }
    virtual void generateExpressionByteCode(ByteCodeBlock *codeBlock, ByteCodeGenerateContext *context, ByteCodeRegisterIndex dstRegister)
    {
        Context *ctx = codeBlock->m_codeBlock->context();

        ASSERT(kind() != Kind::Undefined);

        if (kind() == Kind::Constructor) {
            // super() this.__proto__.constructor.__proto__.call
            ASSERT(!m_inStatic);
            size_t reg = context->getRegister();
            codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), REGULAR_REGISTER_LIMIT, reg, ctx->staticStrings().__proto__), context, this);
            codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), reg, reg, ctx->staticStrings().constructor), context, this);
            codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), reg, reg, ctx->staticStrings().__proto__), context, this);
            codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), reg, dstRegister, ctx->staticStrings().call), context, this);
        } else if (kind() == Kind::Access) {
            //  ... = super.<property> or super.<function>()
            if (m_inStatic) {
                // super is this.__proto__
                codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), REGULAR_REGISTER_LIMIT, dstRegister, ctx->staticStrings().__proto__), context, this);
            } else {
                // super is this.__proto__.constructor.__proto__.prototype
                size_t reg = context->getRegister();
                codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), REGULAR_REGISTER_LIMIT, reg, ctx->staticStrings().__proto__), context, this);
                codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), reg, reg, ctx->staticStrings().constructor), context, this);
                codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), reg, reg, ctx->staticStrings().__proto__), context, this);
                codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), reg, dstRegister, ctx->staticStrings().prototype), context, this);
                context->giveUpRegister();
            }
        } else if (kind() == Kind::Assign) {
            // super.<property> = ...
            // super is this
            if (dstRegister != REGULAR_REGISTER_LIMIT) {
                codeBlock->pushCode(Move(ByteCodeLOC(m_loc.index), REGULAR_REGISTER_LIMIT, dstRegister), context, this);
            }
        } else {
            // Unknown kind
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    void setKind(Kind kind)
    {
        m_kind = kind;
    }

    Kind kind()
    {
        return m_kind;
    }

    bool inStatic()
    {
        return m_inStatic;
    }

public:
    Kind m_kind;
    bool m_inStatic;
};
}

#include "Node.h"
#endif
