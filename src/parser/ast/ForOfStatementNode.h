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

#ifndef ForOfStatementNode_h
#define ForOfStatementNode_h

#include "ExpressionNode.h"
#include "StatementNode.h"

namespace Escargot {

class ForOfStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    ForOfStatementNode(Node *left, Node *right, Node *body)
        : StatementNode()
    {
        m_left = (ExpressionNode *)left;
        m_right = (ExpressionNode *)right;
        m_body = (StatementNode *)body;
    }

    virtual ~ForOfStatementNode()
    {
    }

    virtual ASTNodeType type() { return ASTNodeType::ForOfStatement; }
    virtual void generateStatementByteCode(ByteCodeBlock *codeBlock, ByteCodeGenerateContext *context)
    {
        Context *ctx = codeBlock->m_codeBlock->context();
        bool canSkipCopyToRegisterBefore = context->m_canSkipCopyToRegister;
        context->m_canSkipCopyToRegister = false;

        ByteCodeGenerateContext newContext(*context);

        newContext.getRegister(); // ExecutionResult of m_right should not overwrite any reserved value
        if (m_left->type() == ASTNodeType::VariableDeclaration)
            m_left->generateResultNotRequiredExpressionByteCode(codeBlock, &newContext);

        size_t baseCountBefore = newContext.m_registerStack->size();
        size_t rightIdx = m_right->getRegister(codeBlock, &newContext);
        m_right->generateExpressionByteCode(codeBlock, &newContext, rightIdx);


        // if (rightIdx[Symbol.iterator] == null) throw TypeError
        size_t iteratorIdx = newContext.getRegister();
        codeBlock->pushCode(LoadLiteral(ByteCodeLOC(m_loc.index), iteratorIdx, Value(ctx->vmInstance()->globalSymbols().iterator)), &newContext, this);
        codeBlock->pushCode(GetObject(ByteCodeLOC(m_loc.index), rightIdx, iteratorIdx, iteratorIdx), context, this);

        size_t literalIdx = newContext.getRegister();
        codeBlock->pushCode(LoadLiteral(ByteCodeLOC(m_loc.index), literalIdx, Value()), &newContext, this);

        size_t equalResultIndex = newContext.getRegister();
        codeBlock->pushCode(BinaryEqual(ByteCodeLOC(m_loc.index), literalIdx, iteratorIdx, equalResultIndex), &newContext, this);
        codeBlock->pushCode(JumpIfFalse(ByteCodeLOC(m_loc.index), equalResultIndex), &newContext, this);
        size_t jumpInitPos = codeBlock->lastCodePosition<JumpIfFalse>();
        newContext.giveUpRegister(); // equalResultIndex
        newContext.giveUpRegister(); // literalIdx

        codeBlock->pushCode(ThrowStaticErrorOperation(ByteCodeLOC(m_loc.index), ErrorObject::TypeError, errorMessage_NotIterable), context, this);


        // iteratorIdx = rightIdx[Symbol.iterator].call(this)
        size_t initPos = codeBlock->currentCodeSize();
        size_t callIdx = newContext.getRegister();
        codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), iteratorIdx, callIdx, ctx->staticStrings().call), context, this);
        codeBlock->pushCode(CallFunctionWithReceiver(ByteCodeLOC(m_loc.index), iteratorIdx, callIdx, rightIdx, 1, iteratorIdx), context, this);
        newContext.giveUpRegister(); // callIdx


        // condPos:
        // nextIdx = iteratorIdx.next();
        size_t condPos = codeBlock->currentCodeSize();
        size_t nextIdx = newContext.getRegister();
        codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), iteratorIdx, nextIdx, ctx->staticStrings().next), context, this);
        codeBlock->pushCode(CallFunctionWithReceiver(ByteCodeLOC(m_loc.index), iteratorIdx, nextIdx, rightIdx, 0, nextIdx), context, this);


        // if (nextIdx.done) goto exit
        size_t doneIdx = newContext.getRegister();
        codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), nextIdx, doneIdx, ctx->staticStrings().done), context, this);
        codeBlock->pushCode(JumpIfTrue(ByteCodeLOC(m_loc.index), doneIdx), &newContext, this);
        size_t jumpExitPos = codeBlock->lastCodePosition<JumpIfTrue>();
        newContext.giveUpRegister(); // doneIdx


        // valueIdx = nextId.value
        size_t valueIdx = newContext.getRegister();
        codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), nextIdx, valueIdx, ctx->staticStrings().value), context, this);
        m_left->generateStoreByteCode(codeBlock, &newContext, valueIdx, true);
        newContext.giveUpRegister(); // valueIdx
        newContext.giveUpRegister(); // nextIdx


        // loop body
        context->m_canSkipCopyToRegister = canSkipCopyToRegisterBefore;
        m_body->generateStatementByteCode(codeBlock, &newContext);

        newContext.giveUpRegister(); // iteratrIdx
        newContext.giveUpRegister(); // rightIdx

        ASSERT(newContext.m_registerStack->size() == baseCountBefore);

        newContext.giveUpRegister(); // first

        size_t forOfIndex = codeBlock->m_requiredRegisterFileSizeInValueSize;
        codeBlock->m_requiredRegisterFileSizeInValueSize++;


        // goto condPos
        codeBlock->pushCode(Jump(ByteCodeLOC(m_loc.index), condPos), &newContext, this);

        size_t forOfEnd = codeBlock->currentCodeSize();

        newContext.consumeBreakPositions(codeBlock, forOfEnd, context->m_tryStatementScopeCount);
        newContext.consumeContinuePositions(codeBlock, condPos, context->m_tryStatementScopeCount);
        newContext.m_positionToContinue = condPos;


        // TODO: handle IteratorClose for throw, break, return and contiune
        // call return prototype function in iterator when it exits with throw, break, return and continue


        // goto exit
        codeBlock->pushCode(Jump(ByteCodeLOC(m_loc.index)), &newContext, this);
        size_t jPos = codeBlock->lastCodePosition<Jump>();

        size_t exitPos = codeBlock->currentCodeSize();
        codeBlock->peekCode<JumpIfFalse>(jumpInitPos)->m_jumpPosition = initPos;
        codeBlock->peekCode<JumpIfTrue>(jumpExitPos)->m_jumpPosition = exitPos;
        codeBlock->peekCode<Jump>(jPos)->m_jumpPosition = codeBlock->currentCodeSize();

        newContext.propagateInformationTo(*context);
    }

protected:
    RefPtr<ExpressionNode> m_left;
    RefPtr<ExpressionNode> m_right;
    RefPtr<StatementNode> m_body;
};
}

#endif
