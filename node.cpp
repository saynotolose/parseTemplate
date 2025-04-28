#include "node.h"
#include "tree.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cmath>

// 字符串辅助函数
std::vector<std::string> SplitString(const std::string& s, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;
    
    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    
    return result;
}

// 添加IdentifierNode构造函数实现
IdentifierNode::IdentifierNode(Tree* tr, Pos pos, const std::string& ident)
    : Node(pos), tree_(tr), ident_(ident) {}

std::string IdentifierNode::String() const {
    return ident_;
}

Node* IdentifierNode::Copy() const {
    return new IdentifierNode(tree_, pos_, ident_);
}

void IdentifierNode::WriteTo(std::stringstream& ss) const {
    ss << ident_;
}

// TextNode 类实现
TextNode::TextNode(Tree* tr, Pos pos, const std::string& text) : Node(pos), tree_(tr), text_(text) {}

std::string TextNode::String() const {
    return text_;
}

Node* TextNode::Copy() const {
    return new TextNode(tree_, pos_, text_);
}

void TextNode::WriteTo(std::stringstream& ss) const {
    ss << text_;
}

// CommentNode 类实现
CommentNode::CommentNode(Tree* tr, Pos pos, const std::string& text) : Node(pos), tree_(tr), text_(text) {}

std::string CommentNode::String() const {
    return text_;
}

Node* CommentNode::Copy() const {
    return new CommentNode(tree_, pos_, text_);
}

void CommentNode::WriteTo(std::stringstream& ss) const {
    ss << text_;
}

// ListNode 类实现
ListNode::ListNode(Tree* tr, Pos pos) : Node(pos), tree_(tr) {}

std::string ListNode::String() const {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < nodes_.size(); ++i) {
        if (i > 0) {
            ss << ", ";
        }
        ss << nodes_[i]->String();
    }
    ss << "]";
    return ss.str();
}

Node* ListNode::Copy() const {
    ListNode* result = new ListNode(tree_, pos_);
    for (size_t i = 0; i < nodes_.size(); ++i) {
        result->Append(nodes_[i]->Copy());
    }
    return result;
}

void ListNode::Append(Node* node) {
    nodes_.push_back(node);
}

void ListNode::WriteTo(std::stringstream& ss) const {
    for (size_t i = 0; i < nodes_.size(); ++i) {
        nodes_[i]->WriteTo(ss);
    }
}

// ActionNode 类实现
ActionNode::ActionNode(Tree* tr, Pos pos, int line, PipeNode* pipe) 
    : Node(pos), tree_(tr), line_(line), pipe_(pipe) {}

std::string ActionNode::String() const {
    return "{{" + pipe_->String() + "}}";
}

Node* ActionNode::Copy() const {
    return new ActionNode(tree_, pos_, line_, static_cast<PipeNode*>(pipe_->Copy()));
}

void ActionNode::WriteTo(std::stringstream& ss) const {
    ss << "{{";
    pipe_->WriteTo(ss);
    ss << "}}";
}

// CommandNode 类实现
CommandNode::CommandNode(Tree* tr, Pos pos) : Node(pos), tree_(tr) {}

std::string CommandNode::String() const {
    std::stringstream ss;
    for (size_t i = 0; i < args_.size(); ++i) {
        if (i > 0) {
            ss << " ";
        }
        ss << args_[i]->String();
    }
    return ss.str();
}

Node* CommandNode::Copy() const {
    CommandNode* result = new CommandNode(tree_, pos_);
    for (size_t i = 0; i < args_.size(); ++i) {
        result->Append(args_[i]->Copy());
    }
    return result;
}

void CommandNode::Append(Node* arg) {
    args_.push_back(arg);
}

void CommandNode::WriteTo(std::stringstream& ss) const {
    for (size_t i = 0; i < args_.size(); ++i) {
        if (i > 0) {
            ss << " ";
        }
        args_[i]->WriteTo(ss);
    }
}

// PipeNode 类实现
PipeNode::PipeNode(Tree* tr, Pos pos, int line) : Node(pos), tree_(tr), line_(line), is_assign_(false) {}

std::string PipeNode::String() const {
    std::stringstream ss;
    for (size_t i = 0; i < decl_.size(); ++i) {
        if (i > 0) {
            ss << ", ";
        }
        ss << decl_[i]->String();
    }
    if (!decl_.empty()) {
        ss << " := ";
    }
    for (size_t i = 0; i < cmds_.size(); ++i) {
        if (i > 0) {
            ss << " | ";
        }
        ss << cmds_[i]->String();
    }
    return ss.str();
}

Node* PipeNode::Copy() const {
    PipeNode* result = new PipeNode(tree_, pos_, line_);
    for (size_t i = 0; i < cmds_.size(); ++i) {
        result->Append(static_cast<CommandNode*>(cmds_[i]->Copy()));
    }
    for (size_t i = 0; i < decl_.size(); ++i) {
        result->AddDecl(static_cast<VariableNode*>(decl_[i]->Copy()));
    }
    return result;
}

void PipeNode::Append(CommandNode* cmd) {
    cmds_.push_back(cmd);
}

void PipeNode::AddDecl(VariableNode* v) {
    decl_.push_back(v);
}

void PipeNode::WriteTo(std::stringstream& ss) const {
    if (!decl_.empty()) {
        for (size_t i = 0; i < decl_.size(); ++i) {
            if (i > 0) {
                ss << ", ";
            }
            decl_[i]->WriteTo(ss);
        }
        if (is_assign_) {
            ss << " = ";
        } else {
            ss << " := ";
        }
    }
    
    for (size_t i = 0; i < cmds_.size(); ++i) {
        if (i > 0) {
            ss << " | ";
        }
        cmds_[i]->WriteTo(ss);
    }
}

// VariableNode 类实现
VariableNode::VariableNode(Tree* tr, Pos pos, const std::string& ident) : Node(pos), tree_(tr), ident_(ident) {}

std::string VariableNode::String() const {
    return "$" + ident_;
}

Node* VariableNode::Copy() const {
    return new VariableNode(tree_, pos_, ident_);
}

void VariableNode::WriteTo(std::stringstream& ss) const {
    ss << "$" << ident_;
}

// DotNode 类实现
DotNode::DotNode(Tree* tr, Pos pos) : Node(pos), tree_(tr) {}

std::string DotNode::String() const {
    return ".";
}

Node* DotNode::Copy() const {
    return new DotNode(tree_, pos_);
}

void DotNode::WriteTo(std::stringstream& ss) const {
    ss << ".";
}

// NilNode 类实现
NilNode::NilNode(Tree* tr, Pos pos) : Node(pos), tree_(tr) {}

std::string NilNode::String() const {
    return "nil";
}

Node* NilNode::Copy() const {
    return new NilNode(tree_, pos_);
}

void NilNode::WriteTo(std::stringstream& ss) const {
    ss << "nil";
}

// FieldNode 类实现
FieldNode::FieldNode(Tree* tr, Pos pos, const std::string& ident) : Node(pos), tree_(tr), ident_(ident) {}

std::string FieldNode::String() const {
    return "." + ident_;
}

Node* FieldNode::Copy() const {
    return new FieldNode(tree_, pos_, ident_);
}

void FieldNode::WriteTo(std::stringstream& ss) const {
    ss << "." << ident_;
}

// ChainNode 类实现
ChainNode::ChainNode(Tree* tr, Pos pos, Node* node) : Node(pos), tree_(tr), node_(node) {}

std::string ChainNode::String() const {
    return node_->String();
}

Node* ChainNode::Copy() const {
    return new ChainNode(tree_, pos_, node_->Copy());
}

void ChainNode::WriteTo(std::stringstream& ss) const {
    node_->WriteTo(ss);
}

// 在node.cpp中添加
void ChainNode::AddField(const std::string& field) {
    // 去掉前导点(如果有)
    if (field.length() > 0 && field[0] == '.') {
        fields_.push_back(field.substr(1));
    } else {
        fields_.push_back(field);
    }
}

// BoolNode 类实现
BoolNode::BoolNode(Tree* tr, Pos pos, bool value) : Node(pos), tree_(tr), value_(value) {}

std::string BoolNode::String() const {
    return value_ ? "true" : "false";
}

Node* BoolNode::Copy() const {
    return new BoolNode(tree_, pos_, value_);
}

void BoolNode::WriteTo(std::stringstream& ss) const {
    ss << (value_ ? "true" : "false");
}

// NumberNode 类实现
NumberNode::NumberNode(Tree* tr, Pos pos, const std::string& text) : Node(pos), tree_(tr), text_(text), 
      is_int_(false), is_uint_(false), is_float_(false), is_complex_(false),
      int64_(0), uint64_(0), float64_(0.0) {}

std::string NumberNode::String() const {
    return text_;
}

Node* NumberNode::Copy() const {
    return new NumberNode(tree_, pos_, text_);
}

void NumberNode::WriteTo(std::stringstream& ss) const {
    ss << text_;
}

// StringNode 类实现
StringNode::StringNode(Tree* tr, Pos pos, const std::string& quoted, const std::string& text) 
    : Node(pos), tree_(tr), quoted_(quoted), text_(text) {}

std::string StringNode::String() const {
    return quoted_;
}

Node* StringNode::Copy() const {
    return new StringNode(tree_, pos_, quoted_, text_);
}

void StringNode::WriteTo(std::stringstream& ss) const {
    ss << quoted_;
}

// EndNode 类实现
EndNode::EndNode(Tree* tr, Pos pos) : Node(pos), tree_(tr) {}

std::string EndNode::String() const {
    return "{{end}}";
}

Node* EndNode::Copy() const {
    return new EndNode(tree_, pos_);
}

void EndNode::WriteTo(std::stringstream& ss) const {
    ss << "{{end}}";
}

// ElseNode 类实现
ElseNode::ElseNode(Tree* tr, Pos pos, int line) : Node(pos), tree_(tr), line_(line) {}

std::string ElseNode::String() const {
    return "{{else}}";
}

Node* ElseNode::Copy() const {
    return new ElseNode(tree_, pos_, line_);
}

void ElseNode::WriteTo(std::stringstream& ss) const {
    ss << "{{else}}";
}

// BranchNode 类实现
BranchNode::BranchNode(Tree* tr, NodeType type, Pos pos, int line, 
                       PipeNode* pipe, ListNode* list, ListNode* else_list)
    : Node(pos), tree_(tr), node_type_(type), line_(line), 
      pipe_(pipe), list_(list), else_list_(else_list) {}

std::string BranchNode::String() const {
    std::string name;
    switch (node_type_) {
        case NodeIf:
            name = "if";
            break;
        case NodeRange:
            name = "range";
            break;
        case NodeWith:
            name = "with";
            break;
        default:
            name = "unknown";
    }
    
    std::stringstream ss;
    ss << "{{" << name << " " << pipe_->String() << "}}";
    if (list_ != NULL) {
        ss << list_->String();
    }
    if (else_list_ != NULL) {
        ss << "{{else}}" << else_list_->String();
    }
    ss << "{{end}}";
    return ss.str();
}

void BranchNode::WriteTo(std::stringstream& ss) const {
    // 确定分支类型
    std::string name;
    switch (node_type_) {
        case NodeIf:
            name = "if";
            break;
        case NodeRange:
            name = "range";
            break;
        case NodeWith:
            name = "with";
            break;
        default:
            name = "unknown";
    }
    
    ss << "{{" << name << " ";
    pipe_->WriteTo(ss);
    ss << "}}";
    
    if (list_) {
        list_->WriteTo(ss);
    }
    
    if (else_list_) {
        ss << "{{else}}";
        else_list_->WriteTo(ss);
    }
    
    ss << "{{end}}";
}

// IfNode 类实现
IfNode::IfNode(Tree* tr, Pos pos, int line, PipeNode* pipe, 
               ListNode* list, ListNode* elseList)
    : BranchNode(tr, NodeIf, pos, line, pipe, list, elseList) {}

Node* IfNode::Copy() const {
    return new IfNode(tree_, pos_, line_,
        static_cast<PipeNode*>(pipe_->Copy()),
        static_cast<ListNode*>(list_->Copy()),
        else_list_ ? static_cast<ListNode*>(else_list_->Copy()) : NULL);
}

// RangeNode 类实现
RangeNode::RangeNode(Tree* tr, Pos pos, int line, PipeNode* pipe, 
                     ListNode* list, ListNode* elseList)
    : BranchNode(tr, NodeRange, pos, line, pipe, list, elseList) {}

Node* RangeNode::Copy() const {
    return new RangeNode(tree_, pos_, line_,
        static_cast<PipeNode*>(pipe_->Copy()),
        static_cast<ListNode*>(list_->Copy()),
        else_list_ ? static_cast<ListNode*>(else_list_->Copy()) : NULL);
}

// WithNode 类实现
WithNode::WithNode(Tree* tr, Pos pos, int line, PipeNode* pipe, 
                   ListNode* list, ListNode* elseList)
    : BranchNode(tr, NodeWith, pos, line, pipe, list, elseList) {}

Node* WithNode::Copy() const {
    return new WithNode(tree_, pos_, line_,
        static_cast<PipeNode*>(pipe_->Copy()),
        static_cast<ListNode*>(list_->Copy()),
        else_list_ ? static_cast<ListNode*>(else_list_->Copy()) : NULL);
}

// TemplateNode 类实现
TemplateNode::TemplateNode(Tree* tr, Pos pos, int line, const std::string& name, PipeNode* pipe)
    : Node(pos), tree_(tr), line_(line), name_(name), pipe_(pipe) {}

std::string TemplateNode::String() const {
    return "{{template \"" + name_ + "\" " + pipe_->String() + "}}";
}

Node* TemplateNode::Copy() const {
    return new TemplateNode(tree_, pos_, line_, name_, static_cast<PipeNode*>(pipe_->Copy()));
}

void TemplateNode::WriteTo(std::stringstream& ss) const {
    ss << "{{template \"" << name_ << "\" ";
    if (pipe_) {
        pipe_->WriteTo(ss);
    }
    ss << "}}";
}

// BreakNode 类实现
BreakNode::BreakNode(Tree* tr, Pos pos, int line) : Node(pos), tree_(tr), line_(line) {}

std::string BreakNode::String() const {
    return "{{break}}";
}

Node* BreakNode::Copy() const {
    return new BreakNode(tree_, pos_, line_);
}

void BreakNode::WriteTo(std::stringstream& ss) const {
    ss << "{{break}}";
}

// ContinueNode 类实现
ContinueNode::ContinueNode(Tree* tr, Pos pos, int line) : Node(pos), tree_(tr), line_(line) {}

std::string ContinueNode::String() const {
    return "{{continue}}";
}

Node* ContinueNode::Copy() const {
    return new ContinueNode(tree_, pos_, line_);
}

void ContinueNode::WriteTo(std::stringstream& ss) const {
    ss << "{{continue}}";
}

// 检查树是否为空
bool IsEmptyTree(const Node* n) {
    if (n == NULL) {
        return true;
    }

    switch (n->Type()) {
        case NodeList:
            {
                const ListNode* l = static_cast<const ListNode*>(n);
                const std::vector<Node*>& nodes = l->Nodes();
                for (size_t i = 0; i < nodes.size(); ++i) {
                    if (!IsEmptyTree(nodes[i])) {
                        return false;
                    }
                }
                return true;
            }
        case NodeAction:
            return IsEmptyTree(static_cast<const ActionNode*>(n)->Pipe());
        case NodeCommand:
            {
                const CommandNode* c = static_cast<const CommandNode*>(n);
                const std::vector<Node*>& args = c->Args();
                for (size_t i = 0; i < args.size(); ++i) {
                    if (!IsEmptyTree(args[i])) {
                        return false;
                    }
                }
                return true;
            }
        case NodePipe:
            {
                const PipeNode* p = static_cast<const PipeNode*>(n);
                const std::vector<CommandNode*>& cmds = p->Cmds();
                for (size_t i = 0; i < cmds.size(); ++i) {
                    if (!IsEmptyTree(cmds[i])) {
                        return false;
                    }
                }
                return true;
            }
        case NodeText:
            return static_cast<const TextNode*>(n)->Text().empty();
        default:
            return false;
    }
}