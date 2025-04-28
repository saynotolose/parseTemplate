#ifndef TEMPLATE_NODE_H
#define TEMPLATE_NODE_H

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <complex>

// 前向声明
class Tree;

// 节点类型
enum NodeType {
    NodeText,         // 普通文本
    NodeAction,       // 非控制动作（如字段求值）
    NodeBool,         // 布尔常量
    NodeChain,        // 字段访问序列
    NodeCommand,      // 管道的一个元素
    NodeDot,          // 光标，点
    NodeElse,         // else动作（不添加到树中）
    NodeEnd,          // end动作（不添加到树中）
    NodeField,        // 字段或方法名
    NodeIdentifier,   // 标识符，总是函数名
    NodeIf,           // if动作
    NodeList,         // 节点列表
    NodeNil,          // 无类型nil常量
    NodeNumber,       // 数值常量
    NodePipe,         // 命令管道
    NodeRange,        // range动作
    NodeString,       // 字符串常量
    NodeTemplate,     // 模板调用动作
    NodeVariable,     // $变量
    NodeWith,         // with动作
    NodeComment,      // 注释
    NodeBreak,        // break动作
    NodeContinue      // continue动作
};

// 位置类型
typedef size_t Pos;

// 节点接口
class Node {
public:
    Node(Pos pos) : pos_(pos) {}
    virtual ~Node() {}
    
    // 返回节点类型
    virtual NodeType Type() const = 0;
    
    // 返回字符串表示
    virtual std::string String() const = 0;
    
    // 深度复制节点及其所有组件
    virtual Node* Copy() const = 0;
    
    // 返回节点在完整原始输入字符串中的起始字节位置
    virtual Pos Position() const { return pos_; }
    
    // 返回包含的Tree
    virtual Tree* GetTree() const = 0;
    
    // 将字符串输出写入stringstream
    virtual void WriteTo(std::stringstream& ss) const = 0;
    
    Pos GetPos() const { return pos_; }
    
protected:
    Pos pos_;
};

// 文本节点
class TextNode : public Node {
public:
    TextNode(Tree* tr, Pos pos, const std::string& text);
    
    NodeType Type() const { return NodeText; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    const std::string& Text() const { return text_; }
    
private:
    Tree* tree_;
    std::string text_;
};

// 注释节点
class CommentNode : public Node {
public:
    CommentNode(Tree* tr, Pos pos, const std::string& text);
    
    NodeType Type() const { return NodeComment; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    const std::string& Text() const { return text_; }
    
private:
    Tree* tree_;
    std::string text_;
};

// ListNode保存节点序列
class ListNode : public Node {
public:
    ListNode(Tree* tr, Pos pos);
    
    NodeType Type() const { return NodeList; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    void Append(Node* node);
    
    const std::vector<Node*>& Nodes() const { return nodes_; }
    
private:
    Tree* tree_;
    std::vector<Node*> nodes_;
};

// 变量节点
class VariableNode : public Node {
public:
    VariableNode(Tree* tr, Pos pos, const std::string& ident);
    
    NodeType Type() const { return NodeVariable; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    const std::string& Ident() const { return ident_; }
    
private:
    Tree* tree_;
    std::string ident_;
};

// Dot节点
class DotNode : public Node {
public:
    DotNode(Tree* tr, Pos pos);
    
    NodeType Type() const { return NodeDot; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
private:
    Tree* tree_;
};

// Nil节点
class NilNode : public Node {
public:
    NilNode(Tree* tr, Pos pos);
    
    NodeType Type() const { return NodeNil; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
private:
    Tree* tree_;
};

// 字段节点
class FieldNode : public Node {
public:
    FieldNode(Tree* tr, Pos pos, const std::string& ident);
    
    NodeType Type() const { return NodeField; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    const std::string& Ident() const { return ident_; }
    
private:
    Tree* tree_;
    std::string ident_;
};

// 链式节点
class ChainNode : public Node {
public:
    ChainNode(Tree* tr, Pos pos, Node* node);
    
    NodeType Type() const { return NodeChain; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    Node* GetNode() const { return node_; }
    
    // 添加字段方法和成员
    void AddField(const std::string& field);
    const std::vector<std::string>& Fields() const { return fields_; }
    
private:
    Tree* tree_;
    Node* node_;
    std::vector<std::string> fields_; // 添加字段列表
};

// 布尔节点
class BoolNode : public Node {
public:
    BoolNode(Tree* tr, Pos pos, bool value);
    
    NodeType Type() const { return NodeBool; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    bool Value() const { return value_; }
    
private:
    Tree* tree_;
    bool value_;
};

// 数字节点
class NumberNode : public Node {
public:
    NumberNode(Tree* tr, Pos pos, const std::string& text);
    
    NodeType Type() const { return NodeNumber; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    bool IsInt() const { return is_int_; }
    bool IsUint() const { return is_uint_; }
    bool IsFloat() const { return is_float_; }
    bool IsComplex() const { return is_complex_; }
    
    long Int64() const { return int64_; }
    unsigned long Uint64() const { return uint64_; }
    double Float64() const { return float64_; }
    std::complex<double> Complex() const { return complex_; }
    
    const std::string& Text() const { return text_; }
    
private:
    Tree* tree_;
    std::string text_;
    bool is_int_;
    bool is_uint_;
    bool is_float_;
    bool is_complex_;
    
    long int64_;
    unsigned long uint64_;
    double float64_;
    std::complex<double> complex_;
    
    void simplifyComplex();
};

// 字符串节点
class StringNode : public Node {
public:
    StringNode(Tree* tr, Pos pos, const std::string& quoted, const std::string& text);
    
    NodeType Type() const { return NodeString; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    std::string Quoted() const { return quoted_; }
    std::string Text() const { return text_; }
    
private:
    Tree* tree_;
    std::string quoted_; // 带引号的原始文本
    std::string text_;   // 经过引号处理的字符串
};

// EndNode表示{{end}}动作
class EndNode : public Node {
public:
    EndNode(Tree* tr, Pos pos);
    
    NodeType Type() const { return NodeEnd; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
private:
    Tree* tree_;
};

// ElseNode表示{{else}}动作
class ElseNode : public Node {
public:
    ElseNode(Tree* tr, Pos pos, int line);
    
    NodeType Type() const { return NodeElse; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    int Line() const { return line_; }
    
private:
    Tree* tree_;
    int line_;
};

// 标识符节点
class IdentifierNode : public Node {
public:
    IdentifierNode(Tree* tr, Pos pos, const std::string& ident);
    
    NodeType Type() const { return NodeIdentifier; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    const std::string& Ident() const { return ident_; }
    
private:
    Tree* tree_;
    std::string ident_;
};

// 命令节点
class CommandNode : public Node {
public:
    CommandNode(Tree* tr, Pos pos);
    
    NodeType Type() const { return NodeCommand; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    void Append(Node* arg);
    
    const std::vector<Node*>& Args() const { return args_; }
    
private:
    Tree* tree_;
    std::vector<Node*> args_;
};

// 管道节点
class PipeNode : public Node {
public:
    PipeNode(Tree* tr, Pos pos, int line);
    
    NodeType Type() const { return NodePipe; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    void Append(CommandNode* cmd);
    
    void AddDecl(VariableNode* v);
    
    int Line() const { return line_; }
    bool IsAssign() const { return is_assign_; }
    void SetIsAssign(bool is_assign) { is_assign_ = is_assign; }
    const std::vector<VariableNode*>& Decl() const { return decl_; }
    const std::vector<CommandNode*>& Cmds() const { return cmds_; }
    
private:
    Tree* tree_;
    int line_;
    bool is_assign_;
    std::vector<VariableNode*> decl_;
    std::vector<CommandNode*> cmds_;
};

// 动作节点
class ActionNode : public Node {
public:
    ActionNode(Tree* tr, Pos pos, int line, PipeNode* pipe);
    
    NodeType Type() const { return NodeAction; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    int Line() const { return line_; }
    const PipeNode* Pipe() const { return pipe_; }
    
private:
    Tree* tree_;
    int line_;
    PipeNode* pipe_;
};

// 分支节点基类
class BranchNode : public Node {
public:
    BranchNode(Tree* tr, NodeType type, Pos pos, int line, 
               PipeNode* pipe, 
               ListNode* list, 
               ListNode* else_list);
    
    std::string String() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    int Line() const { return line_; }
    const PipeNode* GetPipe() const { return pipe_; }
    const ListNode* List() const { return list_; }
    const ListNode* ElseList() const { return else_list_; }
    
protected:
    Tree* tree_;
    NodeType node_type_;
    int line_;
    PipeNode* pipe_;
    ListNode* list_;
    ListNode* else_list_;
};

// If节点
class IfNode : public BranchNode {
public:
    IfNode(Tree* tr, Pos pos, int line, 
           PipeNode* pipe, 
           ListNode* list, 
           ListNode* else_list);
    
    NodeType Type() const { return NodeIf; }
    Node* Copy() const;
};

// Range节点
class RangeNode : public BranchNode {
public:
    RangeNode(Tree* tr, Pos pos, int line, 
              PipeNode* pipe, 
              ListNode* list, 
              ListNode* else_list);
    
    NodeType Type() const { return NodeRange; }
    Node* Copy() const;
};

// With节点
class WithNode : public BranchNode {
public:
    WithNode(Tree* tr, Pos pos, int line, 
             PipeNode* pipe, 
             ListNode* list, 
             ListNode* else_list);
    
    NodeType Type() const { return NodeWith; }
    Node* Copy() const;
};

// Break节点
class BreakNode : public Node {
public:
    BreakNode(Tree* tr, Pos pos, int line);
    
    NodeType Type() const { return NodeBreak; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    int Line() const { return line_; }
    
private:
    Tree* tree_;
    int line_;
};

// Continue节点
class ContinueNode : public Node {
public:
    ContinueNode(Tree* tr, Pos pos, int line);
    
    NodeType Type() const { return NodeContinue; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    int Line() const { return line_; }
    
private:
    Tree* tree_;
    int line_;
};

// Template节点
class TemplateNode : public Node {
public:
    TemplateNode(Tree* tr, Pos pos, int line, 
                 const std::string& name, 
                 PipeNode* pipe);
    
    NodeType Type() const { return NodeTemplate; }
    std::string String() const;
    Node* Copy() const;
    Tree* GetTree() const { return tree_; }
    void WriteTo(std::stringstream& ss) const;
    
    int Line() const { return line_; }
    std::string Name() const { return name_; }
    const PipeNode* Pipe() const { return pipe_; }
    
private:
    Tree* tree_;
    int line_;
    std::string name_;
    PipeNode* pipe_;
};

// 工具函数：检查树是否为空
bool IsEmptyTree(const Node* n);

// 字符串辅助函数
std::vector<std::string> SplitString(const std::string& s, char delimiter);

#endif // TEMPLATE_NODE_H