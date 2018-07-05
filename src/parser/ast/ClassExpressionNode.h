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

#ifndef ClassExpressionNode_h
#define ClassExpressionNode_h

#include "runtime/VMInstance.h"
#include "parser/esprima_cpp/esprima.h"
#include "ProgramNode.h"
#include "LiteralNode.h"
#include "SequenceExpressionNode.h"
#include "ClassBodyNode.h"

namespace Escargot {

class ClassExpressionNode : public ExpressionNode {
public:
    friend class ScriptParser;
    ClassExpressionNode(IdentifierNode* id, Node* superClass, ClassBodyNode* body)
        : ExpressionNode()
    {
        m_id = id;
        m_superClass = superClass;
        m_body = body;
        m_targetId = nullptr;
    }

    virtual ~ClassExpressionNode()
    {
    }

    virtual ASTNodeType type() { return ASTNodeType::ClassExpression; }
    void generateEmptyConstructor(ByteCodeBlock* codeBlock, ByteCodeGenerateContext* context, ByteCodeRegisterIndex dstRegister)
    {
        Context* ctx = codeBlock->m_codeBlock->context();
        InterpretedCodeBlock* currentBlk = context->m_codeBlock->asInterpretedCodeBlock();
        InterpretedCodeBlock* blk = new InterpretedCodeBlock(ctx, currentBlk->script(), id() ? id() : targetId(), superClass() ? true : false, currentBlk, ExtendedNodeLOC(0, 0, m_loc.index));
        codeBlock->pushCode(CreateFunction(ByteCodeLOC(m_loc.index), dstRegister, blk), context, this);
    }

    void definePropertyMethod(ByteCodeBlock* codeBlock, ByteCodeGenerateContext* context, MethodDefinitionNode* m, String* keyString, ByteCodeRegisterIndex targetRegister, ByteCodeRegisterIndex valueRegister)
    {
        Context* ctx = codeBlock->m_codeBlock->context();
        codeBlock->m_literalData.pushBack(keyString);
        if (m && m->kind() == PropertyNode::Kind::Get) {
            size_t propertyRegister = context->getRegister();
            codeBlock->pushCode(LoadLiteral(ByteCodeLOC(m->m_loc.index), propertyRegister, keyString), context, this);
            codeBlock->pushCode(ObjectDefineGetter(ByteCodeLOC(m->m_loc.index), targetRegister, propertyRegister, valueRegister), context, this);
            context->giveUpRegister();
        } else if (m && m->kind() == PropertyNode::Kind::Set) {
            size_t propertyRegister = context->getRegister();
            codeBlock->pushCode(LoadLiteral(ByteCodeLOC(m->m_loc.index), propertyRegister, keyString), context, this);
            codeBlock->pushCode(ObjectDefineSetter(ByteCodeLOC(m->m_loc.index), targetRegister, propertyRegister, valueRegister), context, this);
            context->giveUpRegister();

        } else {
            size_t defineRegister = context->getRegister();
            size_t keyRegister = context->getRegister();
            size_t descriptReigster = context->getRegister();
            size_t funcRegister = context->getRegister();
            size_t objectRegister = context->getRegister();

            codeBlock->pushCode(Move(ByteCodeLOC(m_loc.index), targetRegister, defineRegister), context, this);
            codeBlock->pushCode(GetGlobalObject(ByteCodeLOC(m_loc.index), objectRegister, ctx->staticStrings().Object), context, this);
            codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), objectRegister, funcRegister, ctx->staticStrings().defineProperty), context, this);
            codeBlock->pushCode(LoadLiteral(ByteCodeLOC(m ? m->m_loc.index : m_loc.index), keyRegister, keyString), context, this);
            codeBlock->pushCode(CreateObject(ByteCodeLOC(m_loc.index), descriptReigster), context, this);
            codeBlock->pushCode(ObjectDefineOwnPropertyWithNameOperation(ByteCodeLOC(m_loc.index), descriptReigster, ctx->staticStrings().key, keyRegister), context, this);
            codeBlock->pushCode(ObjectDefineOwnPropertyWithNameOperation(ByteCodeLOC(m_loc.index), descriptReigster, ctx->staticStrings().value, valueRegister), context, this);
            codeBlock->pushCode(CallFunctionWithReceiver(ByteCodeLOC(m_loc.index), objectRegister, funcRegister, defineRegister, 3, descriptReigster), context, this);

            context->giveUpRegister();
            context->giveUpRegister();
            context->giveUpRegister();
            context->giveUpRegister();
            context->giveUpRegister();
        }
    }


    virtual void generateExpressionByteCode(ByteCodeBlock* codeBlock, ByteCodeGenerateContext* context, ByteCodeRegisterIndex dstRegister)
    {
        Context* ctx = codeBlock->m_codeBlock->context();
        bool hasProtoMember = false;
        bool hasConstructor = false;
        size_t regSize = 0;
        size_t protoRegister = 0;
        PropertiesNodeVector& body = m_body->body();

        for (size_t i = 0; i < body.size(); i++) {
            MethodDefinitionNode* m = static_cast<MethodDefinitionNode*>(body[i].get());
            if (m->kind() == PropertyNode::Kind::Constructor) {
                m->setName(id() ? id() : targetId());
                m->generateExpressionByteCode(codeBlock, context, dstRegister);
                hasConstructor = true;
            } else {
                size_t reg = context->getRegister();
                m->generateExpressionByteCode(codeBlock, context, reg);
                regSize++;
                hasProtoMember = true;
            }
        }

        Node* super = superClass();
        if (super && super->type() == ASTNodeType::SequenceExpression) {
            const ExpressionNodeVector& nodeVector = static_cast<SequenceExpressionNode*>(super)->expressions();
            for (size_t i = 0; i < nodeVector.size() - 1; i++) {
                size_t tmpRegister = context->getRegister();
                nodeVector[i]->generateExpressionByteCode(codeBlock, context, tmpRegister);
                context->giveUpRegister();
            }
            super = nodeVector.back().get();
        }

        if (!hasConstructor) {
            generateEmptyConstructor(codeBlock, context, dstRegister);
        }


        if (super) {
            size_t superProtoRegister = context->getRegister();
            size_t superRegister = context->getRegister();
            size_t objectRegister = context->getRegister();
            size_t funcRegister = context->getRegister();
            size_t resultRegister = context->getRegister();

            // SubClass.prototype = Object.create(SuperClass.prototype);
            super->generateExpressionByteCode(codeBlock, context, superRegister);
            codeBlock->pushCode(GetGlobalObject(ByteCodeLOC(m_loc.index), objectRegister, ctx->staticStrings().Object), context, this);
            codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), objectRegister, funcRegister, ctx->staticStrings().create), context, this);

            if (super->type() != ASTNodeType::Literal || !super->asLiteral()->value().isNull()) {
                codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), superRegister, superProtoRegister, ctx->staticStrings().prototype), context, this);
            } else {
                superProtoRegister = superRegister;
            }

            codeBlock->pushCode(CallFunctionWithReceiver(ByteCodeLOC(m_loc.index), objectRegister, funcRegister, superProtoRegister, 1, resultRegister), context, this);

            SetObjectInlineCache* inlineCache = new SetObjectInlineCache();
            codeBlock->m_literalData.pushBack(inlineCache);
            codeBlock->pushCode(SetObjectPreComputedCase(ByteCodeLOC(m_loc.index), dstRegister, ctx->staticStrings().prototype, resultRegister, inlineCache), context, this);

            // SubClass.__proto__ = SuperClass
            if (!super->isLiteral() || !super->asLiteral()->value().isNull()) {
                SetObjectInlineCache* inlineCache = new SetObjectInlineCache();
                codeBlock->m_literalData.pushBack(inlineCache);
                codeBlock->pushCode(SetObjectPreComputedCase(ByteCodeLOC(m_loc.index), dstRegister, ctx->staticStrings().__proto__, superRegister, inlineCache), context, this);
            }
            context->giveUpRegister();
            context->giveUpRegister();
            context->giveUpRegister();
            context->giveUpRegister();
            context->giveUpRegister();
        }

        protoRegister = context->getRegister();
        codeBlock->pushCode(GetObjectPreComputedCase(ByteCodeLOC(m_loc.index), dstRegister, protoRegister, codeBlock->m_codeBlock->context()->staticStrings().prototype), context, this);
        regSize++;

        if (!hasConstructor) {
            definePropertyMethod(codeBlock, context, nullptr, ctx->staticStrings().constructor.string(), protoRegister, dstRegister);
        }

        size_t regIdx = regSize;
        for (size_t i = 0; i < body.size(); i++) {
            MethodDefinitionNode* m = static_cast<MethodDefinitionNode*>(body[i].get());
            if (m->kind() == PropertyNode::Kind::Constructor) {
                definePropertyMethod(codeBlock, context, m, ctx->staticStrings().constructor.string(), protoRegister, dstRegister);
                continue;
            }

            size_t targetRegister;
            if (m->isStatic()) {
                targetRegister = dstRegister;
            } else {
                targetRegister = protoRegister;
            }

            size_t valueRegister = context->getLastRegisterIndex(--regIdx);

            String* keyString;
            if (m->key()->isIdentifier()) {
                keyString = m->key()->asIdentifier()->name().string();
            } else if (m->key()->isLiteral()) {
                const Value& val = m->key()->asLiteral()->value();
                if (val.isInt32()) {
                    int num = val.asInt32();
                    if (num >= 0 && num < ESCARGOT_STRINGS_NUMBERS_MAX)
                        keyString = ctx->staticStrings().numbers[num].string();

                    keyString = ctx->staticStrings().dtoa(num);

                } else {
                    // TODO check possible types and support other types of name
                    RELEASE_ASSERT_NOT_REACHED();
                }
            } else {
                ASSERT_NOT_REACHED();
            }

            definePropertyMethod(codeBlock, context, m, keyString, targetRegister, valueRegister);
        }

        for (size_t i = 0; i < regSize; i++) {
            context->giveUpRegister();
        }
    }

    inline IdentifierNode* id()
    {
        return m_id.get();
    }

    inline Node* superClass()
    {
        return m_superClass.get();
    }

    inline ClassBodyNode* body()
    {
        return m_body.get();
    }

    inline void setTargetId(IdentifierNode* id)
    {
        m_targetId = id;
    }

    inline IdentifierNode* targetId()
    {
        return m_targetId.get();
    }

protected:
    RefPtr<IdentifierNode> m_id;
    RefPtr<IdentifierNode> m_targetId;
    RefPtr<Node> m_superClass;
    RefPtr<ClassBodyNode> m_body;
};
}

#endif
