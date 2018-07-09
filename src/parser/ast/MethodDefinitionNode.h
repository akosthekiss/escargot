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

#ifndef MethodDefinitionNode_h
#define MethodDefinitionNode_h

#include "PropertyNode.h"

namespace Escargot {

class MethodDefinitionNode : public PropertyNode {
public:
    friend class ScriptParser;

    MethodDefinitionNode(Node* key, bool computed, Node* value, PropertyNode::Kind kind, bool isStatic)
        : PropertyNode(key, value, kind, computed)
    {
        m_isStatic = isStatic;
        m_name = nullptr;
    }

    virtual ~MethodDefinitionNode()
    {
    }

    virtual ASTNodeType type() { return ASTNodeType::MethodDefinition; }
    virtual void generateExpressionByteCode(ByteCodeBlock* codeBlock, ByteCodeGenerateContext* context, ByteCodeRegisterIndex dstRegister)
    {
        CodeBlock* blk = nullptr;
        size_t cnt = 0;
        for (size_t i = 0; i < context->m_codeBlock->asInterpretedCodeBlock()->childBlocks().size(); i++) {
            CodeBlock* c = context->m_codeBlock->asInterpretedCodeBlock()->childBlocks()[i];
            if (c->isFunctionExpression()) {
                if (cnt == context->m_feCounter) {
                    blk = c;
                    break;
                }
                cnt++;
            }
        }
        ASSERT(blk);
        if (name()) {
            blk->setFunctionName(name()->name());
        }
        if (context->m_isWithScope && !context->m_isEvalCode)
            blk->setInWithScope();
        codeBlock->pushCode(CreateFunction(ByteCodeLOC(m_loc.index), dstRegister, blk), context, this);
        context->m_feCounter++;
    }

    bool isStatic()
    {
        return m_isStatic;
    }

    void setName(IdentifierNode* name)
    {
        m_name = name;
    }

    IdentifierNode* name()
    {
        return m_name.get();
    }

protected:
    bool m_isStatic;
    RefPtr<IdentifierNode> m_name;
};
}

#endif
