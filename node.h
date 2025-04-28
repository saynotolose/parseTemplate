#ifndef TEMPLATE_NODE_H
#define TEMPLATE_NODE_H

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <complex>
#include <unordered_map>

// 前向声明
class Tree;

// 节点类型
enum class NodeType {
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
using Pos = size_t;

// 节点接口
class Node {
public:
    virtual ~Node() = default;
    
    // 返回节点类型
    virtual NodeType Type() const = 0;
    
    // 返回字符串表示
    virtual std::string String() const = 0;
    
    // 深度复制节点及其所有组件
    virtual std::unique_ptr<Node> Copy() const = 0;
    
    // 返回节点在完整原始输入字符串中的起始字节位置
    virtual Pos Position() const = 0;
    
    // 返回包含的Tree
    virtual Tree* GetTree() const = 0;
    
    // 将字符串输出写入stringstream
    virtual void WriteTo(std::stringstream& ss) const = 0;
};

// 文本节点
class TextNode : public Node {
public:
    TextNode(Tree* tr, Pos pos, const std::string& text);
    
    NodeType Type() const override { return NodeType::NodeText; }
    std::string String() const override;
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    std::string Text() const { return text_; }
    
public:
    Tree* tree_;
    Pos pos_;
    std::string text_;
};

// 注释节点
class CommentNode : public Node {
public:
    CommentNode(Tree* tr, Pos pos, const std::string& text);
    
    NodeType Type() const override { return NodeType::NodeComment; }
    std::string String() const override;
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    std::string Text() const { return text_; }
    
public:
    Tree* tree_;
    Pos pos_;
    std::string text_;
};

// ListNode保存节点序列
class ListNode : public Node {
public:
    ListNode(Tree* tr, Pos pos);
    
    NodeType Type() const override { return NodeType::NodeList; }
    std::string String() const override;
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    void Append(std::unique_ptr<Node> node);
    const std::vector<std::unique_ptr<Node>>& Nodes() const { return nodes_; }
    
public:
    Tree* tree_;
    Pos pos_;
    std::vector<std::unique_ptr<Node>> nodes_;
};

// 变量节点
class VariableNode : public Node {
public:
    VariableNode(Tree* tr, Pos pos, const std::string& ident);
    
    NodeType Type() const override { return NodeType::NodeVariable; }
    std::string String() const override;
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    const std::vector<std::string>& Ident() const { return ident_; }
    
public:
    Tree* tree_;
    Pos pos_;
    std::vector<std::string> ident_; // 按词法顺序排列的变量名和字段
};

// Dot节点
class DotNode : public Node {
public:
    DotNode(Tree* tr, Pos pos);
    
    NodeType Type() const override { return NodeType::NodeDot; }
    std::string String() const override { return "."; }
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
public:
    Tree* tree_;
    Pos pos_;
};

// Nil节点
class NilNode : public Node {
public:
    NilNode(Tree* tr, Pos pos);
    
    NodeType Type() const override { return NodeType::NodeNil; }
    std::string String() const override { return "nil"; }
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
public:
    Tree* tree_;
    Pos pos_;
};

// 字段节点
class FieldNode : public Node {
public:
    FieldNode(Tree* tr, Pos pos, const std::string& ident);
    
    NodeType Type() const override { return NodeType::NodeField; }
    std::string String() const override;
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    const std::vector<std::string>& Ident() const { return ident_; }
    
public:
    Tree* tree_;
    Pos pos_;
    std::vector<std::string> ident_; // 按词法顺序排列的标识符
};

// 链式节点
class ChainNode : public Node {
public:
    ChainNode(Tree* tr, Pos pos, std::unique_ptr<Node> node);
    
    NodeType Type() const override { return NodeType::NodeChain; }
    std::string String() const override;
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    void Add(const std::string& field);
    const Node* GetNode() const { return node_.get(); }
    const std::vector<std::string>& Field() const { return field_; }
    
public:
    Tree* tree_;
    Pos pos_;
    std::unique_ptr<Node> node_;
    std::vector<std::string> field_;
};

// 布尔节点
class BoolNode : public Node {
public:
    BoolNode(Tree* tr, Pos pos, bool value);
    
    NodeType Type() const override { return NodeType::NodeBool; }
    std::string String() const override;
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    bool Value() const { return value_; }
    
public:
    Tree* tree_;
    Pos pos_;
    bool value_;
};

// 数字节点
class NumberNode : public Node {
public:
    NumberNode(Tree* tr, Pos pos, const std::string& text);
    
    NodeType Type() const override { return NodeType::NodeNumber; }
    std::string String() const override { return text_; }
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    bool IsInt() const { return is_int_; }
    bool IsUint() const { return is_uint_; }
    bool IsFloat() const { return is_float_; }
    bool IsComplex() const { return is_complex_; }
    int64_t Int64() const { return int64_; }
    uint64_t Uint64() const { return uint64_; }
    double Float64() const { return float64_; }
    std::complex<double> Complex128() const { return complex128_; }
    
public:
    Tree* tree_;
    Pos pos_;
    bool is_int_;
    bool is_uint_;
    bool is_float_;
    bool is_complex_;
    int64_t int64_;
    uint64_t uint64_;
    double float64_;
    std::complex<double> complex128_;
    std::string text_;
    
    void simplifyComplex();
};

// 字符串节点
class StringNode : public Node {
public:
    StringNode(Tree* tr, Pos pos, const std::string& quoted, const std::string& text);
    
    NodeType Type() const override { return NodeType::NodeString; }
    std::string String() const override { return quoted_; }
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    std::string Quoted() const { return quoted_; }
    std::string Text() const { return text_; }
    
public:
    Tree* tree_;
    Pos pos_;
    std::string quoted_; // 带引号的原始文本
    std::string text_;   // 经过引号处理的字符串
};

// EndNode表示{{end}}动作
class EndNode : public Node {
public:
    EndNode(Tree* tr, Pos pos);
    
    NodeType Type() const override { return NodeType::NodeEnd; }
    std::string String() const override { return "{{end}}"; }
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
public:
    Tree* tree_;
    Pos pos_;
};

// ElseNode表示{{else}}动作
class ElseNode : public Node {
public:
    ElseNode(Tree* tr, Pos pos, int line);
    
    NodeType Type() const override { return NodeType::NodeElse; }
    std::string String() const override { return "{{else}}"; }
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    int Line() const { return line_; }
    
public:
    Tree* tree_;
    Pos pos_;
    int line_;
};

// 标识符节点
class IdentifierNode : public Node {
public:
    IdentifierNode(const std::string& ident);
    IdentifierNode(Tree* tr, Pos pos, const std::string& ident);
    
    NodeType Type() const override { return NodeType::NodeIdentifier; }
    std::string String() const override { return ident_; }
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    IdentifierNode* SetPos(Pos pos);
    IdentifierNode* SetTree(Tree* t);
    std::string Ident() const { return ident_; }
    
public:
    Tree* tree_;
    Pos pos_;
    std::string ident_;
};

// 命令节点
class CommandNode : public Node {
public:
    CommandNode(Tree* tr, Pos pos);
    
    NodeType Type() const override { return NodeType::NodeCommand; }
    std::string String() const override;
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    void Append(std::unique_ptr<Node> arg);
    const std::vector<std::unique_ptr<Node>>& Args() const { return args_; }
    
public:
    Tree* tree_;
    Pos pos_;
    std::vector<std::unique_ptr<Node>> args_;
};

// 管道节点
class PipeNode : public Node {
public:
    PipeNode(Tree* tr, Pos pos, int line);
    
    NodeType Type() const override { return NodeType::NodePipe; }
    std::string String() const override;
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    void Append(std::unique_ptr<CommandNode> cmd);
    void AddDecl(std::unique_ptr<VariableNode> v);
    
    int Line() const { return line_; }
    bool IsAssign() const { return is_assign_; }
    void SetIsAssign(bool is_assign) { is_assign_ = is_assign; }
    const std::vector<std::unique_ptr<VariableNode>>& Decl() const { return decl_; }
    const std::vector<std::unique_ptr<CommandNode>>& Cmds() const { return cmds_; }
    
public:
    Tree* tree_;
    Pos pos_;
    int line_;
    bool is_assign_ = false;
    std::vector<std::unique_ptr<VariableNode>> decl_;
    std::vector<std::unique_ptr<CommandNode>> cmds_;
};

// 动作节点
class ActionNode : public Node {
public:
    ActionNode(Tree* tr, Pos pos, int line, std::unique_ptr<PipeNode> pipe);
    
    NodeType Type() const override { return NodeType::NodeAction; }
    std::string String() const override;
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    int Line() const { return line_; }
    const PipeNode* Pipe() const { return pipe_.get(); }
    
public:
    Tree* tree_;
    Pos pos_;
    int line_;
    std::unique_ptr<PipeNode> pipe_;
};

// 分支节点基类
class BranchNode : public Node {
public:
    BranchNode(Tree* tr, NodeType type, Pos pos, int line, 
               std::unique_ptr<PipeNode> pipe, 
               std::unique_ptr<ListNode> list, 
               std::unique_ptr<ListNode> else_list);
    
    std::string String() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    int Line() const { return line_; }
    const PipeNode* GetPipe() const { return pipe_.get(); }
    const ListNode* List() const { return list_.get(); }
    const ListNode* ElseList() const { return else_list_.get(); }
    
public:
    Tree* tree_;
    NodeType node_type_;
    Pos pos_;
    int line_;
    std::unique_ptr<PipeNode> pipe_;
    std::unique_ptr<ListNode> list_;
    std::unique_ptr<ListNode> else_list_;
};

// If节点
class IfNode : public BranchNode {
public:
    IfNode(Tree* tr, Pos pos, int line, 
           std::unique_ptr<PipeNode> pipe, 
           std::unique_ptr<ListNode> list, 
           std::unique_ptr<ListNode> else_list);
    
    NodeType Type() const override { return NodeType::NodeIf; }
    std::unique_ptr<Node> Copy() const override;
};

// Range节点
class RangeNode : public BranchNode {
public:
    RangeNode(Tree* tr, Pos pos, int line, 
              std::unique_ptr<PipeNode> pipe, 
              std::unique_ptr<ListNode> list, 
              std::unique_ptr<ListNode> else_list);
    
    NodeType Type() const override { return NodeType::NodeRange; }
    std::unique_ptr<Node> Copy() const override;
};

// With节点
class WithNode : public BranchNode {
public:
    WithNode(Tree* tr, Pos pos, int line, 
             std::unique_ptr<PipeNode> pipe, 
             std::unique_ptr<ListNode> list, 
             std::unique_ptr<ListNode> else_list);
    
    NodeType Type() const override { return NodeType::NodeWith; }
    std::unique_ptr<Node> Copy() const override;
};

// Break节点
class BreakNode : public Node {
public:
    BreakNode(Tree* tr, Pos pos, int line);
    
    NodeType Type() const override { return NodeType::NodeBreak; }
    std::string String() const override { return "{{break}}"; }
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    int Line() const { return line_; }
    
public:
    Tree* tree_;
    Pos pos_;
    int line_;
};

// Continue节点
class ContinueNode : public Node {
public:
    ContinueNode(Tree* tr, Pos pos, int line);
    
    NodeType Type() const override { return NodeType::NodeContinue; }
    std::string String() const override { return "{{continue}}"; }
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    int Line() const { return line_; }
    
public:
    Tree* tree_;
    Pos pos_;
    int line_;
};

// Template节点
class TemplateNode : public Node {
public:
    TemplateNode(Tree* tr, Pos pos, int line, 
                 const std::string& name, 
                 std::unique_ptr<PipeNode> pipe);
    
    NodeType Type() const override { return NodeType::NodeTemplate; }
    std::string String() const override;
    std::unique_ptr<Node> Copy() const override;
    Pos Position() const override { return pos_; }
    Tree* GetTree() const override { return tree_; }
    void WriteTo(std::stringstream& ss) const override;
    
    int Line() const { return line_; }
    std::string Name() const { return name_; }
    const PipeNode* Pipe() const { return pipe_.get(); }
    
public:
    Tree* tree_;
    Pos pos_;
    int line_;
    std::string name_;
    std::unique_ptr<PipeNode> pipe_;
};

// 工具函数：检查树是否为空
bool IsEmptyTree(const Node* n);

// 字符串辅助函数
std::vector<std::string> SplitString(const std::string& s, char delimiter);

#endif // TEMPLATE_NODE_H