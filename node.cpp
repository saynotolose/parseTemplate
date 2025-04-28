#include "node.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cmath>

// 字符串辅助函数
std::vector<std::string> SplitString(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// TextNode实现
TextNode::TextNode(Tree* tr, Pos pos, const std::string& text)
    : tree_(tr), pos_(pos), text_(text) {}

std::string TextNode::String() const {
    return text_;
}

std::unique_ptr<Node> TextNode::Copy() const {
    return std::make_unique<TextNode>(tree_, pos_, text_);
}

void TextNode::WriteTo(std::stringstream& ss) const {
    ss << String();
}

// CommentNode实现
CommentNode::CommentNode(Tree* tr, Pos pos, const std::string& text)
    : tree_(tr), pos_(pos), text_(text) {}

std::string CommentNode::String() const {
    std::stringstream ss;
    WriteTo(ss);
    return ss.str();
}

std::unique_ptr<Node> CommentNode::Copy() const {
    return std::make_unique<CommentNode>(tree_, pos_, text_);
}

void CommentNode::WriteTo(std::stringstream& ss) const {
    ss << "{{" << text_ << "}}";
}

// ListNode实现
ListNode::ListNode(Tree* tr, Pos pos)
    : tree_(tr), pos_(pos) {}

std::string ListNode::String() const {
    std::stringstream ss;
    WriteTo(ss);
    return ss.str();
}

std::unique_ptr<Node> ListNode::Copy() const {
    auto list = std::make_unique<ListNode>(tree_, pos_);
    for (const auto& node : nodes_) {
        list->Append(node->Copy());
    }
    return list;
}

void ListNode::WriteTo(std::stringstream& ss) const {
    for (const auto& node : nodes_) {
        node->WriteTo(ss);
    }
}

void ListNode::Append(std::unique_ptr<Node> node) {
    nodes_.push_back(std::move(node));
}

// VariableNode实现
VariableNode::VariableNode(Tree* tr, Pos pos, const std::string& ident)
    : tree_(tr), pos_(pos) {
    ident_ = SplitString(ident, '.');
}

std::string VariableNode::String() const {
    std::stringstream ss;
    WriteTo(ss);
    return ss.str();
}

std::unique_ptr<Node> VariableNode::Copy() const {
    return std::make_unique<VariableNode>(tree_, pos_, String());
}

void VariableNode::WriteTo(std::stringstream& ss) const {
    for (size_t i = 0; i < ident_.size(); ++i) {
        if (i > 0) {
            ss << '.';
        }
        ss << ident_[i];
    }
}

// DotNode实现
DotNode::DotNode(Tree* tr, Pos pos)
    : tree_(tr), pos_(pos) {}

std::unique_ptr<Node> DotNode::Copy() const {
    return std::make_unique<DotNode>(tree_, pos_);
}

void DotNode::WriteTo(std::stringstream& ss) const {
    ss << ".";
}

// NilNode实现
NilNode::NilNode(Tree* tr, Pos pos)
    : tree_(tr), pos_(pos) {}

std::unique_ptr<Node> NilNode::Copy() const {
    return std::make_unique<NilNode>(tree_, pos_);
}

void NilNode::WriteTo(std::stringstream& ss) const {
    ss << "nil";
}

// FieldNode实现
// FieldNode::FieldNode(Tree* tr, Pos pos, const std::string& ident)
//     : tree_(tr), pos_(pos) {
//     // 去掉前导的'.'，然后分割
//     std::string field = ident.substr(1);
//     ident_ = SplitString(field, '.');
// }
// FieldNode::FieldNode(Tree* tr, Pos pos, const std::string& ident)
//     : tree_(tr), pos_(pos) {
//     std::cout << "创建字段节点，输入: " << ident << std::endl;
//     // 确保字段以点开头
//     if (ident.empty() || ident[0] != '.') {
//         throw std::runtime_error("字段标识符必须以'.'开头: " + ident);
//     }
    
//     // 去掉前导的'.'，然后分割
//     std::string field = ident.substr(1);
//     std::cout << "  处理的字段路径: " << field << std::endl;
//     ident_ = SplitString(field, '.');
    
//     // 打印分割结果
//     std::cout << "  分割后的字段部分:";
//     for (const auto& part : ident_) {
//         std::cout << " [" << part << "]";
//     }
//     std::cout << std::endl;
// }
FieldNode::FieldNode(Tree* tr, Pos pos, const std::string& ident)
    : tree_(tr), pos_(pos) {
    std::cout << "创建字段节点，输入: " << ident << std::endl;
    
    // 确保字段以点开头
    if (ident.empty() || ident[0] != '.') {
        throw std::runtime_error("字段标识符必须以'.'开头: " + ident);
    }
    
    // 去掉前导的'.'
    std::string fieldPath = ident.substr(1);  // 去掉第一个点，得到"Name.first"
    std::cout << "  处理的字段路径: " << fieldPath << std::endl;
    
    // 如果路径中包含点，我们需要正确处理
    size_t dotPos = fieldPath.find('.');
    if (dotPos != std::string::npos) {
        // 分割路径
        ident_.push_back(fieldPath.substr(0, dotPos));  // "Name"
        ident_.push_back(fieldPath.substr(dotPos + 1)); // "first"
    } else {
        // 如果没有点，就直接存储整个路径
        ident_.push_back(fieldPath);
    }
    
    std::cout << "  解析后的字段部分:";
    for (const auto& part : ident_) {
        std::cout << " [" << part << "]";
    }
    std::cout << std::endl;
}



std::string FieldNode::String() const {
    // 如果有多个部分，按照点连接起来
    if (ident_.size() > 1) {
        std::string result = ident_[0];
        for (size_t i = 1; i < ident_.size(); ++i) {
            result += "." + ident_[i];
        }
        return result;
    }
    // 否则直接返回第一个部分
    return ident_.empty() ? "" : ident_[0];
}

std::unique_ptr<Node> FieldNode::Copy() const {
    std::stringstream ss;
    for (size_t i = 0; i < ident_.size(); ++i) {
        ss << '.' << ident_[i];
    }
    return std::make_unique<FieldNode>(tree_, pos_, ss.str());
}

void FieldNode::WriteTo(std::stringstream& ss) const {
    for (const auto& id : ident_) {
        ss << '.' << id;
    }
}

// ChainNode实现
ChainNode::ChainNode(Tree* tr, Pos pos, std::unique_ptr<Node> node)
    : tree_(tr), pos_(pos), node_(std::move(node)) {}

std::string ChainNode::String() const {
    std::stringstream ss;
    WriteTo(ss);
    return ss.str();
}

std::unique_ptr<Node> ChainNode::Copy() const {
    auto chain = std::make_unique<ChainNode>(tree_, pos_, node_->Copy());
    chain->field_ = field_;
    return chain;
}

void ChainNode::WriteTo(std::stringstream& ss) const {
    if (node_->Type() == NodeType::NodePipe) {
        ss << "(";
        node_->WriteTo(ss);
        ss << ")";
    } else {
        node_->WriteTo(ss);
    }
    
    for (const auto& field : field_) {
        ss << '.' << field;
    }
}

void ChainNode::Add(const std::string& field) {
    if (field.empty() || field[0] != '.') {
        throw std::runtime_error("no dot in field");
    }
    std::string fieldWithoutDot = field.substr(1);
    if (fieldWithoutDot.empty()) {
        throw std::runtime_error("empty field");
    }
    field_.push_back(fieldWithoutDot);
}

// BoolNode实现
BoolNode::BoolNode(Tree* tr, Pos pos, bool value)
    : tree_(tr), pos_(pos), value_(value) {}

std::string BoolNode::String() const {
    return value_ ? "true" : "false";
}

std::unique_ptr<Node> BoolNode::Copy() const {
    return std::make_unique<BoolNode>(tree_, pos_, value_);
}

void BoolNode::WriteTo(std::stringstream& ss) const {
    ss << String();
}

// NumberNode实现
NumberNode::NumberNode(Tree* tr, Pos pos, const std::string& text)
    : tree_(tr), pos_(pos), is_int_(false), is_uint_(false), 
      is_float_(false), is_complex_(false), 
      int64_(0), uint64_(0), float64_(0), complex128_(0, 0), text_(text) {
    
    // 尝试解析为整数
    try {
        int64_ = std::stoll(text);
        is_int_ = true;
        
        // 如果是非负数，也可以表示为无符号整数
        if (int64_ >= 0) {
            uint64_ = static_cast<uint64_t>(int64_);
            is_uint_ = true;
        }
    } catch (...) {
        // 不是有效整数
    }
    
    // 尝试解析为浮点数
    try {
        float64_ = std::stod(text);
        is_float_ = true;
        
        // 检查是否可以表示为整数
        if (!is_int_ && float64_ == std::floor(float64_) && 
            float64_ <= static_cast<double>(std::numeric_limits<int64_t>::max()) &&
            float64_ >= static_cast<double>(std::numeric_limits<int64_t>::min())) {
            int64_ = static_cast<int64_t>(float64_);
            is_int_ = true;
        }
        
        // 检查是否可以表示为无符号整数
        if (!is_uint_ && float64_ == std::floor(float64_) && 
            float64_ >= 0 && 
            float64_ <= static_cast<double>(std::numeric_limits<uint64_t>::max())) {
            uint64_ = static_cast<uint64_t>(float64_);
            is_uint_ = true;
        }
    } catch (...) {
        // 不是有效浮点数
    }
    
    // 如果是虚数形式，尝试解析为复数
    if (text.back() == 'i') {
        std::string imagPart = text.substr(0, text.length() - 1);
        try {
            double imag = std::stod(imagPart);
            complex128_ = std::complex<double>(0, imag);
            is_complex_ = true;
            simplifyComplex();
        } catch (...) {
            // 不是有效虚数
        }
    }
    
    // 如果没有任何类型成功解析，那么这是一个无效的数字
    if (!is_int_ && !is_uint_ && !is_float_ && !is_complex_) {
        throw std::runtime_error("illegal number syntax: " + text);
    }
}

std::unique_ptr<Node> NumberNode::Copy() const {
    return std::make_unique<NumberNode>(tree_, pos_, text_);
}

void NumberNode::WriteTo(std::stringstream& ss) const {
    ss << text_;
}

// 将复数简化到其他可能的类型
void NumberNode::simplifyComplex() {
    // 如果虚部为零，则可以表示为浮点数
    if (std::imag(complex128_) == 0) {
        is_float_ = true;
        float64_ = std::real(complex128_);
        
        // 检查是否可以表示为整数
        if (float64_ == std::floor(float64_) && 
            float64_ <= static_cast<double>(std::numeric_limits<int64_t>::max()) &&
            float64_ >= static_cast<double>(std::numeric_limits<int64_t>::min())) {
            is_int_ = true;
            int64_ = static_cast<int64_t>(float64_);
        }
        
        // 检查是否可以表示为无符号整数
        if (float64_ == std::floor(float64_) && 
            float64_ >= 0 && 
            float64_ <= static_cast<double>(std::numeric_limits<uint64_t>::max())) {
            is_uint_ = true;
            uint64_ = static_cast<uint64_t>(float64_);
        }
    }
}

// StringNode实现
StringNode::StringNode(Tree* tr, Pos pos, const std::string& quoted, const std::string& text)
    : tree_(tr), pos_(pos), quoted_(quoted), text_(text) {}

std::unique_ptr<Node> StringNode::Copy() const {
    return std::make_unique<StringNode>(tree_, pos_, quoted_, text_);
}

void StringNode::WriteTo(std::stringstream& ss) const {
    ss << quoted_;
}

// EndNode实现
EndNode::EndNode(Tree* tr, Pos pos)
    : tree_(tr), pos_(pos) {}

std::unique_ptr<Node> EndNode::Copy() const {
    return std::make_unique<EndNode>(tree_, pos_);
}

void EndNode::WriteTo(std::stringstream& ss) const {
    ss << "{{end}}";
}

// ElseNode实现
ElseNode::ElseNode(Tree* tr, Pos pos, int line)
    : tree_(tr), pos_(pos), line_(line) {}

std::unique_ptr<Node> ElseNode::Copy() const {
    return std::make_unique<ElseNode>(tree_, pos_, line_);
}

void ElseNode::WriteTo(std::stringstream& ss) const {
    ss << "{{else}}";
}

// IdentifierNode实现
IdentifierNode::IdentifierNode(const std::string& ident)
    : tree_(nullptr), pos_(0), ident_(ident) {}

IdentifierNode::IdentifierNode(Tree* tr, Pos pos, const std::string& ident)
    : tree_(tr), pos_(pos), ident_(ident) {}

std::unique_ptr<Node> IdentifierNode::Copy() const {
    auto node = std::make_unique<IdentifierNode>(ident_);
    node->SetTree(tree_);
    node->SetPos(pos_);
    return node;
}

void IdentifierNode::WriteTo(std::stringstream& ss) const {
    ss << ident_;
}

IdentifierNode* IdentifierNode::SetPos(Pos pos) {
    pos_ = pos;
    return this;
}

IdentifierNode* IdentifierNode::SetTree(Tree* t) {
    tree_ = t;
    return this;
}

// CommandNode实现
CommandNode::CommandNode(Tree* tr, Pos pos)
    : tree_(tr), pos_(pos) {}

std::string CommandNode::String() const {
    std::stringstream ss;
    WriteTo(ss);
    return ss.str();
}

std::unique_ptr<Node> CommandNode::Copy() const {
    auto cmd = std::make_unique<CommandNode>(tree_, pos_);
    for (const auto& arg : args_) {
        cmd->Append(arg->Copy());
    }
    return cmd;
}

void CommandNode::WriteTo(std::stringstream& ss) const {
    for (size_t i = 0; i < args_.size(); ++i) {
        if (i > 0) {
            ss << ' ';
        }
        if (args_[i]->Type() == NodeType::NodePipe) {
            ss << '(';
            args_[i]->WriteTo(ss);
            ss << ')';
            continue;
        }
        args_[i]->WriteTo(ss);
    }
}

void CommandNode::Append(std::unique_ptr<Node> arg) {
    args_.push_back(std::move(arg));
}

// PipeNode实现
PipeNode::PipeNode(Tree* tr, Pos pos, int line)
    : tree_(tr), pos_(pos), line_(line) {}

std::string PipeNode::String() const {
    std::stringstream ss;
    WriteTo(ss);
    return ss.str();
}

std::unique_ptr<Node> PipeNode::Copy() const {
    auto pipe = std::make_unique<PipeNode>(tree_, pos_, line_);
    pipe->is_assign_ = is_assign_;
    
    for (const auto& var : decl_) {
        pipe->AddDecl(std::unique_ptr<VariableNode>(
            static_cast<VariableNode*>(var->Copy().release())));
    }
    
    for (const auto& cmd : cmds_) {
        pipe->Append(std::unique_ptr<CommandNode>(
            static_cast<CommandNode*>(cmd->Copy().release())));
    }
    
    return pipe;
}

void PipeNode::WriteTo(std::stringstream& ss) const {
    if (!decl_.empty()) {
        for (size_t i = 0; i < decl_.size(); ++i) {
            if (i > 0) {
                ss << ", ";
            }
            decl_[i]->WriteTo(ss);
        }
        ss << (is_assign_ ? " = " : " := ");
    }
    
    for (size_t i = 0; i < cmds_.size(); ++i) {
        if (i > 0) {
            ss << " | ";
        }
        cmds_[i]->WriteTo(ss);
    }
}

void PipeNode::Append(std::unique_ptr<CommandNode> cmd) {
    cmds_.push_back(std::move(cmd));
}

void PipeNode::AddDecl(std::unique_ptr<VariableNode> v) {
    decl_.push_back(std::move(v));
}

// ActionNode实现
ActionNode::ActionNode(Tree* tr, Pos pos, int line, std::unique_ptr<PipeNode> pipe)
    : tree_(tr), pos_(pos), line_(line), pipe_(std::move(pipe)) {}

std::string ActionNode::String() const {
    std::stringstream ss;
    WriteTo(ss);
    return ss.str();
}

std::unique_ptr<Node> ActionNode::Copy() const {
    return std::make_unique<ActionNode>(
        tree_, pos_, line_,
        std::unique_ptr<PipeNode>(static_cast<PipeNode*>(pipe_->Copy().release())));
}

void ActionNode::WriteTo(std::stringstream& ss) const {
    ss << "{{";
    pipe_->WriteTo(ss);
    ss << "}}";
}

// BranchNode实现
BranchNode::BranchNode(Tree* tr, NodeType type, Pos pos, int line, 
                       std::unique_ptr<PipeNode> pipe, 
                       std::unique_ptr<ListNode> list, 
                       std::unique_ptr<ListNode> else_list)
    : tree_(tr), node_type_(type), pos_(pos), line_(line), 
      pipe_(std::move(pipe)), list_(std::move(list)), else_list_(std::move(else_list)) {}

std::string BranchNode::String() const {
    std::stringstream ss;
    WriteTo(ss);
    return ss.str();
}

void BranchNode::WriteTo(std::stringstream& ss) const {
    std::string name;
    switch (node_type_) {
        case NodeType::NodeIf:
            name = "if";
            break;
        case NodeType::NodeRange:
            name = "range";
            break;
        case NodeType::NodeWith:
            name = "with";
            break;
        default:
            throw std::runtime_error("unknown branch type");
    }
    
    ss << "{{" << name << " ";
    pipe_->WriteTo(ss);
    ss << "}}";
    
    list_->WriteTo(ss);
    
    if (else_list_) {
        ss << "{{else}}";
        else_list_->WriteTo(ss);
    }
    
    ss << "{{end}}";
}

// IfNode实现
IfNode::IfNode(Tree* tr, Pos pos, int line, 
               std::unique_ptr<PipeNode> pipe, 
               std::unique_ptr<ListNode> list, 
               std::unique_ptr<ListNode> else_list)
    : BranchNode(tr, NodeType::NodeIf, pos, line, std::move(pipe), std::move(list), std::move(else_list)) {}

std::unique_ptr<Node> IfNode::Copy() const {
    return std::make_unique<IfNode>(
        tree_, pos_, line_,
        std::unique_ptr<PipeNode>(static_cast<PipeNode*>(pipe_->Copy().release())),
        std::unique_ptr<ListNode>(static_cast<ListNode*>(list_->Copy().release())),
        else_list_ ? std::unique_ptr<ListNode>(static_cast<ListNode*>(else_list_->Copy().release())) : nullptr);
}

// RangeNode实现
RangeNode::RangeNode(Tree* tr, Pos pos, int line, 
                     std::unique_ptr<PipeNode> pipe, 
                     std::unique_ptr<ListNode> list, 
                     std::unique_ptr<ListNode> else_list)
    : BranchNode(tr, NodeType::NodeRange, pos, line, std::move(pipe), std::move(list), std::move(else_list)) {}

std::unique_ptr<Node> RangeNode::Copy() const {
    return std::make_unique<RangeNode>(
        tree_, pos_, line_,
        std::unique_ptr<PipeNode>(static_cast<PipeNode*>(pipe_->Copy().release())),
        std::unique_ptr<ListNode>(static_cast<ListNode*>(list_->Copy().release())),
        else_list_ ? std::unique_ptr<ListNode>(static_cast<ListNode*>(else_list_->Copy().release())) : nullptr);
}

// WithNode实现
WithNode::WithNode(Tree* tr, Pos pos, int line, 
                   std::unique_ptr<PipeNode> pipe, 
                   std::unique_ptr<ListNode> list, 
                   std::unique_ptr<ListNode> else_list)
    : BranchNode(tr, NodeType::NodeWith, pos, line, std::move(pipe), std::move(list), std::move(else_list)) {}

std::unique_ptr<Node> WithNode::Copy() const {
    return std::make_unique<WithNode>(
        tree_, pos_, line_,
        std::unique_ptr<PipeNode>(static_cast<PipeNode*>(pipe_->Copy().release())),
        std::unique_ptr<ListNode>(static_cast<ListNode*>(list_->Copy().release())),
        else_list_ ? std::unique_ptr<ListNode>(static_cast<ListNode*>(else_list_->Copy().release())) : nullptr);
}

// BreakNode实现
BreakNode::BreakNode(Tree* tr, Pos pos, int line)
    : tree_(tr), pos_(pos), line_(line) {}

std::unique_ptr<Node> BreakNode::Copy() const {
    return std::make_unique<BreakNode>(tree_, pos_, line_);
}

void BreakNode::WriteTo(std::stringstream& ss) const {
    ss << "{{break}}";
}

// ContinueNode实现
ContinueNode::ContinueNode(Tree* tr, Pos pos, int line)
    : tree_(tr), pos_(pos), line_(line) {}

std::unique_ptr<Node> ContinueNode::Copy() const {
    return std::make_unique<ContinueNode>(tree_, pos_, line_);
}

void ContinueNode::WriteTo(std::stringstream& ss) const {
    ss << "{{continue}}";
}

// TemplateNode实现
TemplateNode::TemplateNode(Tree* tr, Pos pos, int line, 
                           const std::string& name, 
                           std::unique_ptr<PipeNode> pipe)
    : tree_(tr), pos_(pos), line_(line), name_(name), pipe_(std::move(pipe)) {}

std::string TemplateNode::String() const {
    std::stringstream ss;
    WriteTo(ss);
    return ss.str();
}

std::unique_ptr<Node> TemplateNode::Copy() const {
    return std::make_unique<TemplateNode>(
        tree_, pos_, line_, name_,
        pipe_ ? std::unique_ptr<PipeNode>(static_cast<PipeNode*>(pipe_->Copy().release())) : nullptr);
}

void TemplateNode::WriteTo(std::stringstream& ss) const {
    ss << "{{template " << std::quoted(name_);
    if (pipe_) {
        ss << " ";
        pipe_->WriteTo(ss);
    }
    ss << "}}";
}

// 检查树是否为空
// bool IsEmptyTree(const Node* n) {
//     if (!n) {
//         return true;
//     }
    
//     switch (n->Type()) {
//         case NodeType::NodeComment:
//             return true;
//         case NodeType::NodeList: {
//             const auto* list = dynamic_cast<const ListNode*>(n);
//             if (!list) return false;
//             for (const auto& node : list->Nodes()) {
//                 if (!IsEmptyTree(node.get())) {
//                     return false;
//                 }
//             }
//             return true;
//         }
//         case NodeType::NodeText: {
//             const auto* text = dynamic_cast<const TextNode*>(n);
//             if (!text) return false;
//             std::string s = text->Text();
//             std::string trimmed;
//             // 去除空白字符
//             std::copy_if(s.begin(), s.end(), std::back_inserter(trimmed),
//                          [](char c) { return !std::isspace(static_cast<unsigned char>(c)); });
//             return trimmed.empty();
//         }
//         default:
//             return false;
//     }
// }
bool IsEmptyTree(const Node* n) {
    if (!n) {
        std::cout << "IsEmptyTree: 节点为空" << std::endl;
        return true;
    }
    
    std::cout << "IsEmptyTree: 检查节点类型: " << static_cast<int>(n->Type()) << std::endl;
    
    switch (n->Type()) {
        case NodeType::NodeComment:
            std::cout << "IsEmptyTree: 注释节点，视为空" << std::endl;
            return true;
        case NodeType::NodeList: {
            const auto* list = dynamic_cast<const ListNode*>(n);
            if (!list) {
                std::cout << "IsEmptyTree: 列表节点转换失败" << std::endl;
                return false;
            }
            
            std::cout << "IsEmptyTree: 列表节点，子节点数: " << list->Nodes().size() << std::endl;
            
            for (const auto& node : list->Nodes()) {
                if (!IsEmptyTree(node.get())) {
                    std::cout << "IsEmptyTree: 发现非空子节点" << std::endl;
                    return false;
                }
            }
            std::cout << "IsEmptyTree: 所有子节点都为空" << std::endl;
            return true;
        }
        case NodeType::NodeText: {
            const auto* text = dynamic_cast<const TextNode*>(n);
            if (!text) {
                std::cout << "IsEmptyTree: 文本节点转换失败" << std::endl;
                return false;
            }
            
            std::string s = text->Text();
            std::string trimmed;
            // 去除空白字符
            std::copy_if(s.begin(), s.end(), std::back_inserter(trimmed),
                         [](char c) { return !std::isspace(static_cast<unsigned char>(c)); });
            
            std::cout << "IsEmptyTree: 文本节点，原始长度: " << s.length() 
                      << ", 去空格后长度: " << trimmed.length() << std::endl;
            
            return trimmed.empty();
        }
        default:
            std::cout << "IsEmptyTree: 其他节点类型，非空" << std::endl;
            return false;
    }
}