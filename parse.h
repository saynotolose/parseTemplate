#ifndef TEMPLATE_PARSE_H
#define TEMPLATE_PARSE_H

#include "node.h"
#include <string>

#include <vector>
#include <map>
#include <stdexcept>

#include "lexer.h"  // 包含完整的 Item 和 ItemType 定义

// 前向声明Lexer
class Lexer;

// 解析模式
enum Mode {
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

// 解析错误
class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& what_arg) : std::runtime_error(what_arg) {}
    explicit ParseError(const char* what_arg) : std::runtime_error(what_arg) {}
};

// 表示单个解析的模板
class Tree {
public:
    Tree(const std::string& name);
    Tree(const std::string& name, const std::vector<std::map<std::string, std::string> >& funcs);
    ~Tree(); // 析构函数
    
    // 创建Tree的副本
    Tree* Copy() const;
    
    // 解析模板定义字符串以构造模板的表示
    Tree* Parse(
        const std::string& text, 
        const std::string& leftDelim, 
        const std::string& rightDelim,
        std::map<std::string, Tree*>& treeSet,
        const std::vector<std::map<std::string, std::string> >& funcs = std::vector<std::map<std::string, std::string> >());
    
    // 创建新节点的工厂方法 - 全部使用裸指针
    ListNode* newList(Pos pos);
    TextNode* newText(Pos pos, const std::string& text);
    CommentNode* newComment(Pos pos, const std::string& text);
    
    ActionNode* newAction(Pos pos, int line, PipeNode* pipe);
    CommandNode* newCommand(Pos pos);
    VariableNode* newVariable(Pos pos, const std::string& ident);
    IdentifierNode* newIdentifier(Pos pos, const std::string& ident);
    DotNode* newDot(Pos pos);
    NilNode* newNil(Pos pos);
    FieldNode* newField(Pos pos, const std::string& ident);
    ChainNode* newChain(Pos pos, Node* node);
    BoolNode* newBool(Pos pos, bool b);
    NumberNode* newNumber(Pos pos, const std::string& text);
    StringNode* newString(Pos pos, const std::string& orig, const std::string& text);
    EndNode* newEnd(Pos pos);
    ElseNode* newElse(Pos pos, int line);
    IfNode* newIf(Pos pos, int line, PipeNode* pipe, 
                           ListNode* list, ListNode* elseList);
    RangeNode* newRange(Pos pos, int line, PipeNode* pipe, 
                                ListNode* list, ListNode* elseList);
    WithNode* newWith(Pos pos, int line, PipeNode* pipe, 
                               ListNode* list, ListNode* elseList);
    TemplateNode* newTemplate(Pos pos, int line, const std::string& name, PipeNode* pipe);
    BreakNode* newBreak(Pos pos, int line);
    ContinueNode* newContinue(Pos pos, int line);
    PipeNode* newPipeline(Pos pos, int line, const std::vector<VariableNode*>& vars = std::vector<VariableNode*>());

    // 获取错误上下文
    std::pair<std::string, std::string> ErrorContext(const Node* n) const;
    
    // 检查函数是否存在
    bool hasFunction(const std::string& name) const;

    // 解析静态方法
    static std::map<std::string, Tree*> Parse(
        const std::string& name, 
        const std::string& text, 
        const std::string& leftDelim, 
        const std::string& rightDelim,
        const std::vector<std::map<std::string, std::string> >& funcs = std::vector<std::map<std::string, std::string> >());

    std::string GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }
    std::string GetParseName() const { return parseName_; }
    void SetParseName(const std::string& name) { parseName_ = name; }
    Mode GetMode() const { return mode_; }
    void SetMode(Mode mode) { mode_ = mode; }
    const ListNode* GetRoot() const { return root_; }
    
private:
    std::string name_;        // 树表示的模板的名称
    std::string parseName_;   // 解析期间顶级模板的名称，用于错误消息
    ListNode* root_; // 树的顶级根节点
    Mode mode_; // 解析模式
    std::string text_;        // 用于创建模板的文本（或其父模板）

    // 仅用于解析；解析后清除
    std::vector<std::map<std::string, std::string> > funcs_;
    Lexer* lex_;

    Item token_[3]; // 用于解析器的三令牌前瞻
    
    int peekCount_;
    std::vector<std::string> vars_; // 当前定义的变量
    std::map<std::string, Tree*> treeSet_;
    int actionLine_; // 开始动作的左定界符行
    int rangeDepth_;

    // 内部解析方法
    void startParse(const std::vector<std::map<std::string, std::string> >& funcs, 
                   Lexer* lex,
                   std::map<std::string, Tree*>& treeSet);
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
    bool lookAhead(ItemType itemType);
    
    // 解析方法 - 修改为返回裸指针
    std::pair<ListNode*, Node*> itemList();
    Node* textOrAction();
    Node* action();

    // 自定义控制结构
    struct ControlResult {
        Pos pos;
        int line;
        PipeNode* pipe;
        ListNode* list;
        ListNode* elseList;
        
        ControlResult() : pos(0), line(0), pipe(NULL), list(NULL), elseList(NULL) {}
        ~ControlResult(); // 析构函数，释放动态分配的资源
    };
    
    ControlResult parseControl(bool allowElseIf, const std::string& context);

    Node* blockControl();
    Node* breakControl(Pos pos, int line);
    Node* continueControl(Pos pos, int line);
    Node* elseControl();
    Node* endControl();
    Node* ifControl();
    Node* rangeControl();
    Node* templateControl();
    Node* withControl();
    PipeNode* pipeline(const std::string& context, ItemType end);
    Node* operand();
    Node* term();
    VariableNode* useVar(Pos pos, const std::string& name);
    void popVars(size_t n);
    
    // 错误处理
    void errorf(const char* format, ...);
    void error(const std::string& msg);
    void unexpected(const Item& item, const std::string& context);
    Item expect(ItemType expected, const std::string& context);
    Item expectOneOf(ItemType expected1, ItemType expected2, const std::string& context);
    void clearActionLine();
};

#endif // TEMPLATE_PARSE_H