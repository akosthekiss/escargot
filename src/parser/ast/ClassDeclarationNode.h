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

#ifndef ClassDeclarationNode_h
#define ClassDeclarationNode_h

#include "StatementNode.h"
#include "IdentifierNode.h"
#include "ClassBodyNode.h"
#include "ClassExpressionNode.h"

namespace Escargot {

class ClassDeclarationNode : public StatementNode {
public:
    friend class ScriptParser;
    ClassDeclarationNode(IdentifierNode* id, Node* superClass, ClassBodyNode* body)
    {
        m_node = adoptRef(new ClassExpressionNode(id, superClass, body, true));
    }

    virtual ~ClassDeclarationNode()
    {
    }

    virtual ASTNodeType type() { return ASTNodeType::ClassDeclaration; }
    virtual void generateStatementByteCode(ByteCodeBlock* codeBlock, ByteCodeGenerateContext* context)
    {
        if (!id()) {
            // TODO: It will be handled with export & import
            RELEASE_ASSERT_NOT_REACHED();
        }
        RefPtr<AssignmentExpressionSimpleNode> assign = adoptRef(new AssignmentExpressionSimpleNode(id(), m_node.get()));
        assign->m_loc = loc();
        assign->generateResultNotRequiredExpressionByteCode(codeBlock, context);
        assign->giveupChildren();
    }

    inline IdentifierNode* id()
    {
        return m_node->id();
    }

    inline Node* superClass()
    {
        return m_node->superClass();
    }

    inline ClassBodyNode* body()
    {
        return m_node->body();
    }

protected:
    RefPtr<ClassExpressionNode> m_node;
};
}

#endif
