#ifndef TEMPLATE_PARSE_H
#define TEMPLATE_PARSE_H

#include "node.h"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <any>
#pragma once
#include "lexer.h"  // 包含完整的 Item 和 ItemType 定义
// 前向声明Lexer
class Lexer;

// struct Item;

// 解析模式
enum class Mode : unsigned int {
    ParseNone = 0,
    ParseComments = 1 << 0,  // 解析注释并将其添加到AST
    SkipFuncCheck = 1 << 1   // 不检查函数是否已定义
};

// 组合模式运算符重载
inline Mode operator|(Mode a, Mode b) {
    return static_cast<Mode>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

inline Mode operator&(Mode a, Mode b) {
    return static_cast<Mode>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
}

// 表示单个解析的模板
class Tree : public std::enable_shared_from_this<Tree>{
public:
    Tree(const std::string& name);
    Tree(const std::string& name, const std::vector<std::unordered_map<std::string, std::any>>& funcs);
    
    // 创建Tree的副本
    std::shared_ptr<Tree> Copy() const;
    
    // 解析模板定义字符串以构造模板的表示
    std::shared_ptr<Tree> Parse(
        const std::string& text, 
        const std::string& leftDelim, 
        const std::string& rightDelim,
        std::unordered_map<std::string, std::shared_ptr<Tree>>& treeSet,
        const std::vector<std::unordered_map<std::string, std::any>>& funcs = {});
    
    // 创建新节点的工厂方法
    std::unique_ptr<ListNode> newList(Pos pos);
    std::unique_ptr<TextNode> newText(Pos pos, const std::string& text);
    std::unique_ptr<CommentNode> newComment(Pos pos, const std::string& text);
    
    std::unique_ptr<ActionNode> newAction(Pos pos, int line, std::unique_ptr<PipeNode> pipe);
    std::unique_ptr<CommandNode> newCommand(Pos pos);
    std::unique_ptr<VariableNode> newVariable(Pos pos, const std::string& ident);
    std::unique_ptr<DotNode> newDot(Pos pos);
    std::unique_ptr<NilNode> newNil(Pos pos);
    std::unique_ptr<FieldNode> newField(Pos pos, const std::string& ident);
    std::unique_ptr<ChainNode> newChain(Pos pos, std::unique_ptr<Node> node);
    std::unique_ptr<BoolNode> newBool(Pos pos, bool b);
    std::unique_ptr<NumberNode> newNumber(Pos pos, const std::string& text);
    std::unique_ptr<StringNode> newString(Pos pos, const std::string& orig, const std::string& text);
    std::unique_ptr<EndNode> newEnd(Pos pos);
    std::unique_ptr<ElseNode> newElse(Pos pos, int line);
    std::unique_ptr<IfNode> newIf(Pos pos, int line, std::unique_ptr<PipeNode> pipe, 
                                  std::unique_ptr<ListNode> list, std::unique_ptr<ListNode> elseList);
    std::unique_ptr<RangeNode> newRange(Pos pos, int line, std::unique_ptr<PipeNode> pipe, 
                                       std::unique_ptr<ListNode> list, std::unique_ptr<ListNode> elseList);
    std::unique_ptr<WithNode> newWith(Pos pos, int line, std::unique_ptr<PipeNode> pipe, 
                                      std::unique_ptr<ListNode> list, std::unique_ptr<ListNode> elseList);
    std::unique_ptr<TemplateNode> newTemplate(Pos pos, int line, const std::string& name, std::unique_ptr<PipeNode> pipe);
    std::unique_ptr<BreakNode> newBreak(Pos pos, int line);
    std::unique_ptr<ContinueNode> newContinue(Pos pos, int line);
    // std::unique_ptr<PipeNode> newPipeline(Pos pos, int line, const std::vector<std::unique_ptr<VariableNode>>& vars = {});
    std::unique_ptr<PipeNode, std::default_delete<PipeNode>> newPipeline(
        Pos pos, 
        int line, 
        const std::vector<std::unique_ptr<VariableNode, std::default_delete<VariableNode>>, 
                         std::allocator<std::unique_ptr<VariableNode, std::default_delete<VariableNode>>>>& vars = {});


    // 获取错误上下文
    std::pair<std::string, std::string> ErrorContext(const Node* n) const;
    
    // 检查函数是否存在
    bool hasFunction(const std::string& name) const;

    // 解析静态方法
    static std::unordered_map<std::string, std::shared_ptr<Tree>> Parse(
        const std::string& name, 
        const std::string& text, 
        const std::string& leftDelim, 
        const std::string& rightDelim,
        const std::vector<std::unordered_map<std::string, std::any>>& funcs = {});

    std::string GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }
    std::string GetParseName() const { return parseName_; }
    void SetParseName(const std::string& name) { parseName_ = name; }
    Mode GetMode() const { return mode_; }
    void SetMode(Mode mode) { mode_ = mode; }
    const ListNode* GetRoot() const { return root_.get(); }
    
private:
    std::string name_;        // 树表示的模板的名称
    std::string parseName_;   // 解析期间顶级模板的名称，用于错误消息
    std::unique_ptr<ListNode> root_; // 树的顶级根节点
    Mode mode_ = Mode::ParseNone; // 解析模式
    std::string text_;        // 用于创建模板的文本（或其父模板）

    // 仅用于解析；解析后清除
    std::vector<std::unordered_map<std::string, std::any>> funcs_;
    std::shared_ptr<Lexer> lex_;

    Item token_[3]; // 用于解析器的三令牌前瞻
    
    
    int peekCount_ = 0;
    std::vector<std::string> vars_; // 当前定义的变量
    std::unordered_map<std::string, std::shared_ptr<Tree>> treeSet_;
    int actionLine_ = 0; // 开始动作的左定界符行
    int rangeDepth_ = 0;

    // 内部解析方法
    void startParse(const std::vector<std::unordered_map<std::string, std::any>>& funcs, 
                   std::shared_ptr<Lexer> lex,
                   std::unordered_map<std::string, std::shared_ptr<Tree>>& treeSet);
    void stopParse();
    void add();
    void parse();
    void parseDefinition();
    
    // 处理tokens的方法
    Item next();
    void backup();
    void backup2(const Item& t1);
    void backup3(const Item& t2, const Item& t1);
    Item peek();
    Item nextNonSpace();
    Item peekNonSpace();
    
    // 解析方法
    std::pair<std::unique_ptr<ListNode>, std::unique_ptr<Node>> itemList();
    std::unique_ptr<Node> textOrAction();
    std::tuple<Pos, int, std::unique_ptr<PipeNode>, std::unique_ptr<ListNode>, std::unique_ptr<ListNode>> 
    parseControl(bool allowElseIf, const std::string& context);
    
    // 各种控制结构的解析方法
    std::unique_ptr<Node> action();
    std::unique_ptr<Node> blockControl();
    std::unique_ptr<Node> breakControl(Pos pos, int line);
    std::unique_ptr<Node> continueControl(Pos pos, int line);
    std::unique_ptr<Node> ifControl();
    std::unique_ptr<Node> elseControl();
    std::unique_ptr<Node> rangeControl();
    std::unique_ptr<Node> withControl();
    std::unique_ptr<Node> templateControl();
    std::unique_ptr<Node> endControl();
    
    // 解析命令和管道
    std::unique_ptr<PipeNode> pipeline(const std::string& context, ItemType end);
    void checkPipeline(const PipeNode* pipe, const std::string& context);
    std::unique_ptr<CommandNode> command();
    std::unique_ptr<Node> operand();
    std::unique_ptr<Node> term();
    
    // 错误处理
    [[noreturn]] void errorf(const std::string& format, ...);
    [[noreturn]] void error(const std::string& msg);
    Item expect(ItemType expected, const std::string& context);
    Item expectOneOf(ItemType expected1, ItemType expected2, const std::string& context);
    [[noreturn]] void unexpected(const Item& token, const std::string& context);
    void clearActionLine();
    
    // 变量管理
    void popVars(size_t n);
    std::unique_ptr<VariableNode> useVar(Pos pos, const std::string& name);
    
    // 模板名称解析
    std::string parseTemplateName(const Item& token, const std::string& context);
};

// 自定义异常类
class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& what_arg) : std::runtime_error(what_arg) {}
};

#endif // TEMPLATE_PARSE_H