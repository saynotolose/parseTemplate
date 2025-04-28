// tree_nodes.cpp
#include "parse.h"
#include "node.h"
#include <sstream>
#include <iostream>

// 创建变量节点
VariableNode* Tree::newVariable(Pos pos, const std::string& name) {
    return new VariableNode(this, pos, name);
}

NilNode* Tree::newNil(Pos pos) {
    return new NilNode(this, pos);
}

// 在tree_nodes.cpp中添加这个函数
DotNode* Tree::newDot(Pos pos) {
    return new DotNode(this, pos);
}

// 创建列表节点
ListNode* Tree::newList(Pos pos) {
    return new ListNode(this, pos);
}

// 创建文本节点
TextNode* Tree::newText(Pos pos, const std::string& text) {
    return new TextNode(this, pos, text);
}

// 创建注释节点
CommentNode* Tree::newComment(Pos pos, const std::string& text) {
    return new CommentNode(this, pos, text);
}

// 创建动作节点
ActionNode* Tree::newAction(Pos pos, int line, PipeNode* pipe) {
    return new ActionNode(this, pos, line, pipe);
}

// 创建中断节点
BreakNode* Tree::newBreak(Pos pos, int line) {
    return new BreakNode(this, pos, line);
}

// 创建继续节点
ContinueNode* Tree::newContinue(Pos pos, int line) {
    return new ContinueNode(this, pos, line);
}

// 创建if节点
IfNode* Tree::newIf(Pos pos, int line, 
                                PipeNode* pipe, 
                                ListNode* list, 
                                ListNode* elseList) {
    return new IfNode(this, pos, line, pipe, list, elseList);
}

// 创建range节点
RangeNode* Tree::newRange(Pos pos, int line, 
                                     PipeNode* pipe, 
                                     ListNode* list, 
                                     ListNode* elseList) {
    return new RangeNode(this, pos, line, pipe, list, elseList);
}

// 创建with节点
WithNode* Tree::newWith(Pos pos, int line, 
                                    PipeNode* pipe, 
                                    ListNode* list, 
                                    ListNode* elseList) {
    return new WithNode(this, pos, line, pipe, list, elseList);
}

// 创建end节点
EndNode* Tree::newEnd(Pos pos) {
    return new EndNode(this, pos);
}

// 创建else节点
ElseNode* Tree::newElse(Pos pos, int line) {
    return new ElseNode(this, pos, line);
}

// 创建模板节点
TemplateNode* Tree::newTemplate(Pos pos, int line, 
                                           const std::string& name, 
                                           PipeNode* pipe) {
    return new TemplateNode(this, pos, line, name, pipe);
}

// 创建管道节点
PipeNode* Tree::newPipeline(
    Pos pos, 
    int line, 
    const std::vector<VariableNode*>& vars) {
    PipeNode* pipe = new PipeNode(this, pos, line);
    for (size_t i = 0; i < vars.size(); ++i) {
        VariableNode* varCopy = static_cast<VariableNode*>(vars[i]->Copy());
        pipe->AddDecl(varCopy);
    }
    return pipe;
}

// 创建命令节点
CommandNode* Tree::newCommand(Pos pos) {
    return new CommandNode(this, pos);
}

// 创建链节点
ChainNode* Tree::newChain(Pos pos, Node* node) {
    return new ChainNode(this, pos, node);
}

// 创建字段节点
FieldNode* Tree::newField(Pos pos, const std::string& field) {
    return new FieldNode(this, pos, field);
}

// 创建布尔节点
BoolNode* Tree::newBool(Pos pos, bool b) {
    return new BoolNode(this, pos, b);
}

// 创建数字节点
NumberNode* Tree::newNumber(Pos pos, const std::string& text) {
    return new NumberNode(this, pos, text);
}

// 创建标识符节点
IdentifierNode* Tree::newIdentifier(Pos pos, const std::string& ident) {
    return new IdentifierNode(this, pos, ident);
}

// 创建字符串节点
StringNode* Tree::newString(Pos pos, const std::string& quoted, const std::string& text) {
    return new StringNode(this, pos, quoted, text);
}

// term方法实现
Node* Tree::term() {
    Item token = nextNonSpace();
    std::cout << "  解析term，Token: " << itemTypeToString(token.type) << " 值: " << token.val << std::endl;
    
    try {
        switch (token.type) {
            case ItemIdentifier:
                return new IdentifierNode(this, token.pos, token.val);
            case ItemDot:
                std::cout << "  发现点节点" << std::endl;
                return new DotNode(this, token.pos);
            case ItemNil:
                return new NilNode(this, token.pos);
            case ItemVariable:
                std::cout << "  发现变量: " << token.val << std::endl;
                return newVariable(token.pos, token.val);
            case ItemField: {
                std::cout << "  发现字段: " << token.val << std::endl;
                
                // 只处理当前的字段节点，不尝试处理整个路径
                FieldNode* fieldNode = newField(token.pos, token.val);
                
                // 检查是否存在下一个字段（链式访问）
                if (peek().type == ItemField) {
                    // 在解析阶段不处理链式访问，交给执行阶段处理
                    std::cout << "  检测到可能的链式字段访问" << std::endl;
                }
                
                return fieldNode;
            }
            
            

            case ItemBool:
                return new BoolNode(this, token.pos, token.val == "true");
            case ItemNumber:
                return new NumberNode(this, token.pos, token.val);
            
            case ItemString: {
                std::cout << "  解析字符串常量: " << token.val << std::endl;
                
                // 处理引号
                std::string text = token.val;
                if (text.size() >= 2 && (text[0] == '"' || text[0] == '`') && 
                    text[0] == text[text.size()-1]) {
                    text = text.substr(1, text.size() - 2);
                }
                
                return newString(token.pos, token.val, text);
            }

            case ItemRawString: {
                std::cout << "  发现字符串: " << token.val << std::endl;
                // 处理引号
                std::string text = token.val;
                if (text.size() >= 2 && (text[0] == '"' || text[0] == '`') && 
                    text[0] == text[text.size()-1]) {
                    text = text.substr(1, text.size() - 2);
                }
                return newString(token.pos, token.val, text);
            }

            case ItemLeftParen: {
                PipeNode* pipe = pipeline("parenthesized pipeline", ItemRightParen);
                return pipe;
            }

            default:
                std::cout << "  在term中回退Token: " << itemTypeToString(token.type) << std::endl;
                backup();
                return NULL;
        }

    } catch (const std::exception& e) {
        std::cout << "  Term解析异常: " << e.what() << std::endl;
        throw;
    }
}
