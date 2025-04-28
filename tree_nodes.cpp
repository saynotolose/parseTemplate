// tree_nodes.cpp
#include "parse.h"
#include "node.h"
#include <sstream>
#include <iostream>

// 创建变量节点
std::unique_ptr<VariableNode> Tree::newVariable(Pos pos, const std::string& name) {
    return std::make_unique<VariableNode>(this, pos, name);
}

// 创建列表节点
std::unique_ptr<ListNode> Tree::newList(Pos pos) {
    return std::make_unique<ListNode>(this, pos);
}

// 创建文本节点
std::unique_ptr<TextNode> Tree::newText(Pos pos, const std::string& text) {
    return std::make_unique<TextNode>(this, pos, text);
}

// 创建注释节点
std::unique_ptr<CommentNode> Tree::newComment(Pos pos, const std::string& text) {
    return std::make_unique<CommentNode>(this, pos, text);
}

// 创建动作节点
std::unique_ptr<ActionNode> Tree::newAction(Pos pos, int line, std::unique_ptr<PipeNode> pipe) {
    return std::make_unique<ActionNode>(this, pos, line, std::move(pipe));
}

// 创建中断节点
std::unique_ptr<BreakNode> Tree::newBreak(Pos pos, int line) {
    return std::make_unique<BreakNode>(this, pos, line);
}

// 创建继续节点
std::unique_ptr<ContinueNode> Tree::newContinue(Pos pos, int line) {
    return std::make_unique<ContinueNode>(this, pos, line);
}

// 创建if节点
std::unique_ptr<IfNode> Tree::newIf(Pos pos, int line, 
                                   std::unique_ptr<PipeNode> pipe, 
                                   std::unique_ptr<ListNode> list, 
                                   std::unique_ptr<ListNode> elseList) {
    return std::make_unique<IfNode>(this, pos, line, std::move(pipe), std::move(list), std::move(elseList));
}

// 创建range节点
std::unique_ptr<RangeNode> Tree::newRange(Pos pos, int line, 
                                        std::unique_ptr<PipeNode> pipe, 
                                        std::unique_ptr<ListNode> list, 
                                        std::unique_ptr<ListNode> elseList) {
    return std::make_unique<RangeNode>(this, pos, line, std::move(pipe), std::move(list), std::move(elseList));
}

// 创建with节点
std::unique_ptr<WithNode> Tree::newWith(Pos pos, int line, 
                                       std::unique_ptr<PipeNode> pipe, 
                                       std::unique_ptr<ListNode> list, 
                                       std::unique_ptr<ListNode> elseList) {
    return std::make_unique<WithNode>(this, pos, line, std::move(pipe), std::move(list), std::move(elseList));
}

// 创建end节点
std::unique_ptr<EndNode> Tree::newEnd(Pos pos) {
    return std::make_unique<EndNode>(this, pos);
}

// 创建else节点
std::unique_ptr<ElseNode> Tree::newElse(Pos pos, int line) {
    return std::make_unique<ElseNode>(this, pos, line);
}

// 创建模板节点
std::unique_ptr<TemplateNode> Tree::newTemplate(Pos pos, int line, 
                                              const std::string& name, 
                                              std::unique_ptr<PipeNode> pipe) {
    return std::make_unique<TemplateNode>(this, pos, line, name, std::move(pipe));
}

// // 创建管道节点
// std::unique_ptr<PipeNode> Tree::newPipeline(Pos pos, int line, const std::vector<std::unique_ptr<VariableNode>>& vars) {
//     auto pipe = std::make_unique<PipeNode>(this, pos, line);
//     for (const auto& var : vars) {
//         pipe->AddDecl(std::unique_ptr<VariableNode>(static_cast<VariableNode*>(var->Copy().release())));
//     }
//     return pipe;
// }

// 如果没有带变量参数版本的newPipeline，添加这个
std::unique_ptr<PipeNode, std::default_delete<PipeNode>> Tree::newPipeline(
    Pos pos, 
    int line, 
    const std::vector<std::unique_ptr<VariableNode, std::default_delete<VariableNode>>, 
                     std::allocator<std::unique_ptr<VariableNode, std::default_delete<VariableNode>>>>& vars) {
    return std::make_unique<PipeNode>(this, pos, line);
}

// 创建命令节点
std::unique_ptr<CommandNode> Tree::newCommand(Pos pos) {
    return std::make_unique<CommandNode>(this, pos);
}

// 创建链节点
std::unique_ptr<ChainNode> Tree::newChain(Pos pos, std::unique_ptr<Node> node) {
    return std::make_unique<ChainNode>(this, pos, std::move(node));
}

// 创建字段节点
std::unique_ptr<FieldNode> Tree::newField(Pos pos, const std::string& field) {
    return std::make_unique<FieldNode>(this, pos, field);
}

// 创建字符串节点
std::unique_ptr<StringNode> Tree::newString(Pos pos, const std::string& quoted, const std::string& text) {
    return std::make_unique<StringNode>(this, pos, quoted, text);
}

// 实现term方法 - 根据你的语法分析逻辑来确定具体实现
// std::unique_ptr<Node> Tree::term() {
//     Item token = nextNonSpace();
    
//     switch (token.type) {
//         case ItemType::ItemIdentifier:
//             return std::make_unique<IdentifierNode>(this, token.pos, token.val);
//         case ItemType::ItemDot:
//             return std::make_unique<DotNode>(this, token.pos);
//         case ItemType::ItemNil:
//             return std::make_unique<NilNode>(this, token.pos);
//         case ItemType::ItemVariable:
//             return newVariable(token.pos, token.val);
//         case ItemType::ItemField:
//             return newField(token.pos, token.val);
//         case ItemType::ItemBool:
//             return std::make_unique<BoolNode>(this, token.pos, token.val == "true");
//         case ItemType::ItemNumber:
//             return std::make_unique<NumberNode>(this, token.pos, token.val);
//         case ItemType::ItemString:
//         case ItemType::ItemRawString:
//             return std::make_unique<StringNode>(this, token.pos, token.val, token.val);
//         case ItemType::ItemLeftParen: {
//             auto pipe = pipeline("parenthesized pipeline", ItemType::ItemRightParen);
//             return std::move(pipe);
//         }
//         default:
//             backup();
//             return nullptr;
//     }
// }
std::unique_ptr<Node> Tree::term() {
    Item token = nextNonSpace();
    std::cout << "  解析term，Token: " << itemTypeToString(token.type) << " 值: " << token.val << std::endl;
    
    try {
        switch (token.type) {
            case ItemType::ItemIdentifier:
                return std::make_unique<IdentifierNode>(this, token.pos, token.val);
            case ItemType::ItemDot:
                std::cout << "  发现点节点" << std::endl;
                return std::make_unique<DotNode>(this, token.pos);
            case ItemType::ItemNil:
                return std::make_unique<NilNode>(this, token.pos);
            case ItemType::ItemVariable:
                std::cout << "  发现变量: " << token.val << std::endl;
                return newVariable(token.pos, token.val);
            case ItemType::ItemField: {
                std::cout << "  发现字段: " << token.val << std::endl;
                std::string fieldPath = token.val;
                
                // 检查后续的字段部分
                std::string fullPath = fieldPath;
                Item peekToken;
                
                // 不使用循环和链式方式，而是直接将路径组合起来
                if (peek().type == ItemType::ItemField) {
                    std::cout << "  发现字段路径后缀: " << peek().val << std::endl;
                    // 只检查一次，避免复杂的链式处理
                    fullPath += peek().val;
                    next(); // 消费这个token
                }
                
                std::cout << "  最终字段路径: " << fullPath << std::endl;
                return newField(token.pos, fullPath);
            }
            case ItemType::ItemBool:
                return std::make_unique<BoolNode>(this, token.pos, token.val == "true");
            case ItemType::ItemNumber:
                return std::make_unique<NumberNode>(this, token.pos, token.val);

            
            case ItemType::ItemString: {
                std::cout << "  解析字符串常量: " << token.val << std::endl;
                
                // 处理引号
                std::string text = token.val;
                if (text.size() >= 2 && (text[0] == '"' || text[0] == '`') && 
                    text[0] == text[text.size()-1]) {
                    text = text.substr(1, text.size() - 2);
                }
                
                return newString(token.pos, token.val, text);
            }


            case ItemType::ItemRawString: {
                std::cout << "  发现字符串: " << token.val << std::endl;
                // 处理引号
                std::string text = token.val;
                if (text.size() >= 2 && (text[0] == '"' || text[0] == '`') && 
                    text[0] == text[text.size()-1]) {
                    text = text.substr(1, text.size() - 2);
                }
                return newString(token.pos, token.val, text);
            }

            case ItemType::ItemLeftParen: {
                auto pipe = pipeline("parenthesized pipeline", ItemType::ItemRightParen);
            //             return std::move(pipe);
            }

            default:
                std::cout << "  在term中回退Token: " << itemTypeToString(token.type) << std::endl;
                backup();
                return nullptr;
        }

    } catch (const std::exception& e) {
        std::cout << "  Term解析异常: " << e.what() << std::endl;
        throw;
    }
}
