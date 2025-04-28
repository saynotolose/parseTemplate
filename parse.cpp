#include "parse.h"
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <memory>
#include <cstdarg>
#include <regex>
#include <algorithm>
#include "lexer.h"  // 直接包含我们的词法分析器
#include <tuple>  // 确保包含了这个头文件

// 辅助函数：从字符串构建错误消息
static std::string buildErrorMsg(const char* format, va_list args) {
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    return std::string(buffer);
}

// Tree构造函数
// Tree::Tree(const std::string& name)
//     : name_(name), parseName_(name), mode_(Mode::ParseNone), 
//       peekCount_(0), actionLine_(0), rangeDepth_(0) {
//     vars_.push_back("$"); // 初始变量
// }

// Tree::Tree(const std::string& name, const std::vector<std::unordered_map<std::string, std::any>>& funcs)
//     : name_(name), parseName_(name), mode_(Mode::ParseNone), 
//       funcs_(funcs), peekCount_(0), actionLine_(0), rangeDepth_(0) {
//     vars_.push_back("$"); // 初始变量
// }
Tree::Tree(const std::string& name)
    : name_(name), parseName_(name), mode_(Mode::ParseNone), 
      treeSet_(*new std::unordered_map<std::string, std::shared_ptr<Tree>>), // 临时解决方案，稍后会替换
      peekCount_(0), actionLine_(0), rangeDepth_(0) {
    vars_.push_back("$"); // 初始变量
}

Tree::Tree(const std::string& name, const std::vector<std::unordered_map<std::string, std::any>>& funcs)
    : name_(name), parseName_(name), mode_(Mode::ParseNone), 
      treeSet_(*new std::unordered_map<std::string, std::shared_ptr<Tree>>), // 临时解决方案
      funcs_(funcs), peekCount_(0), actionLine_(0), rangeDepth_(0) {
    vars_.push_back("$"); // 初始变量
}


// 创建一个新的解析树
std::shared_ptr<Tree> Tree::Copy() const {
    if (!root_) {
        return nullptr;
    }
    
    auto copy = std::make_shared<Tree>(name_);
    copy->parseName_ = parseName_;
    copy->root_ = std::unique_ptr<ListNode>(static_cast<ListNode*>(root_->Copy().release()));
    copy->text_ = text_;
    
    return copy;
}

// 静态Parse方法，返回模板名称到解析树的映射
// std::unordered_map<std::string, std::shared_ptr<Tree>> Tree::Parse(
//     const std::string& name, 
//     const std::string& text, 
//     const std::string& leftDelim, 
//     const std::string& rightDelim,
//     const std::vector<std::unordered_map<std::string, std::any>>& funcs) {
    
//     std::unordered_map<std::string, std::shared_ptr<Tree>> treeSet;
//     auto t = std::make_shared<Tree>(name, funcs);
//     t->text_ = text;
    
//     try {
//         t->Parse(text, leftDelim, rightDelim, treeSet, funcs);
//         return treeSet;
//     } catch (const ParseError& e) {
//         // 解析错误，返回空映射
//         return {};
//     }
// }
// std::unordered_map<std::string, std::shared_ptr<Tree>> Tree::Parse(
//     const std::string& name, 
//     const std::string& text, 
//     const std::string& leftDelim, 
//     const std::string& rightDelim,
//     const std::vector<std::unordered_map<std::string, std::any>>& funcs) {
    
//     std::cout << "开始解析模板: " << name << std::endl;
//     std::unordered_map<std::string, std::shared_ptr<Tree>> treeSet;
    
//     try {
//         auto t = std::make_shared<Tree>(name, funcs);
//         t->text_ = text;
//         std::cout << "创建Tree对象成功" << std::endl;
        
//         try {
//             std::cout << "调用Tree::Parse实例方法" << std::endl;
//             t->Parse(text, leftDelim, rightDelim, treeSet, funcs);
//             std::cout << "Tree::Parse实例方法完成，treeSet大小: " << treeSet.size() << std::endl;
            
//             // 检查treeSet中的内容
//             for (const auto& [treeName, tree] : treeSet) {
//                 std::cout << "树集合中的树: " << treeName << std::endl;
//                 if (tree && tree->GetRoot()) {
//                     std::cout << "  根节点类型: " << static_cast<int>(tree->GetRoot()->Type()) << std::endl;
//                     std::cout << "  子节点数量: " << (tree->GetRoot()->Type() == NodeType::NodeList ? 
//                              static_cast<const ListNode*>(tree->GetRoot())->Nodes().size() : 0) << std::endl;
//                 } else {
//                     std::cout << "  树为空或没有根节点" << std::endl;
//                 }
//             }
            
//             return treeSet;
//         } catch (const ParseError& e) {
//             std::cerr << "解析错误: " << e.what() << std::endl;
//             throw;
//         } catch (const std::exception& e) {
//             std::cerr << "其他异常: " << e.what() << std::endl;
//             throw;
//         } catch (...) {
//             std::cerr << "未知异常!" << std::endl;
//             throw;
//         }
//     } catch (const std::exception& e) {
//         std::cerr << "创建或设置Tree对象时异常: " << e.what() << std::endl;
//         return {};
//     } catch (...) {
//         std::cerr << "创建或设置Tree对象时未知异常!" << std::endl;
//         return {};
//     }
// }

std::unordered_map<std::string, std::shared_ptr<Tree>> Tree::Parse(
    const std::string& name, 
    const std::string& text, 
    const std::string& leftDelim, 
    const std::string& rightDelim,
    const std::vector<std::unordered_map<std::string, std::any>>& funcs) {
    
    std::cout << "开始解析模板: " << name << std::endl;
    // 创建结果容器并确保其生命周期能持续到方法结束
    auto treeSet = std::make_shared<std::unordered_map<std::string, std::shared_ptr<Tree>>>();
    
    try {
        // 使用 make_shared 创建树对象
        auto t = std::make_shared<Tree>(name, funcs);
        t->text_ = text;
        std::cout << "创建Tree对象成功" << std::endl;
        
        try {
            std::cout << "调用Tree::Parse实例方法" << std::endl;
            // 传入 treeSet 的引用
            t->Parse(text, leftDelim, rightDelim, *treeSet, funcs);
            std::cout << "Tree::Parse实例方法完成，treeSet大小: " << treeSet->size() << std::endl;
            
            // 检查每个树的内容
            for (const auto& [treeName, tree] : *treeSet) {
                std::cout << "树集合中的树: " << treeName << std::endl;
                if (tree && tree->GetRoot()) {
                    std::cout << "  根节点类型: " << static_cast<int>(tree->GetRoot()->Type()) << std::endl;
                    std::cout << "  子节点数量: " << (tree->GetRoot()->Type() == NodeType::NodeList ? 
                             static_cast<const ListNode*>(tree->GetRoot())->Nodes().size() : 0) << std::endl;
                } else {
                    std::cout << "  树为空或没有根节点" << std::endl;
                }
            }
            
            // 如果 treeSet 为空，则手动添加主树
            if (treeSet->empty() && t->GetRoot()) {
                (*treeSet)[name] = t;
                std::cout << "手动添加主树，现在treeSet大小: " << treeSet->size() << std::endl;
            }
            
            // 返回 treeSet 的内容
            return *treeSet;
        } catch (const ParseError& e) {
            std::cerr << "解析错误: " << e.what() << std::endl;
            throw;
        } catch (const std::exception& e) {
            std::cerr << "其他异常: " << e.what() << std::endl;
            throw;
        } catch (...) {
            std::cerr << "未知异常!" << std::endl;
            throw;
        }
    } catch (const std::exception& e) {
        std::cerr << "创建或设置Tree对象时异常: " << e.what() << std::endl;
        return {};
    } catch (...) {
        std::cerr << "创建或设置Tree对象时未知异常!" << std::endl;
        return {};
    }
}


// 实例Parse方法
std::shared_ptr<Tree> Tree::Parse(
    const std::string& text, 
    const std::string& leftDelim, 
    const std::string& rightDelim,
    std::unordered_map<std::string, std::shared_ptr<Tree>>& treeSet,
    const std::vector<std::unordered_map<std::string, std::any>>& funcs) {
    
    parseName_ = name_;
    auto lexer = createLexer(name_, text, leftDelim, rightDelim);
    
    try {
        startParse(funcs, lexer, treeSet);
        text_ = text;
        parse();
        add();
        stopParse();
        return this->shared_from_this();
    } catch (const ParseError& e) {
        stopParse();
        throw;
    }
}

// 初始化解析器
// void Tree::startParse(
//     const std::vector<std::unordered_map<std::string, std::any>>& funcs, 
//     std::shared_ptr<Lexer> lex,
//     std::unordered_map<std::string, std::shared_ptr<Tree>>& treeSet) {
    
//     root_ = nullptr;
//     lex_ = lex;
//     vars_ = {"$"};
//     funcs_ = funcs;
//     treeSet_ = treeSet;
    
//     // 设置词法分析器选项
//     LexOptions options;
//     options.emitComment = (mode_ & Mode::ParseComments) != Mode::ParseNone;
//     options.breakOK = !hasFunction("break");
//     options.continueOK = !hasFunction("continue");
    
//     // 假设Lexer类有一个setOptions方法
//     lex_->setOptions(options);
// }
void Tree::startParse(
    const std::vector<std::unordered_map<std::string, std::any>>& funcs, 
    std::shared_ptr<Lexer> lex,
    std::unordered_map<std::string, std::shared_ptr<Tree>>& treeSet) {
    
    root_ = nullptr;
    lex_ = lex;
    vars_ = {"$"};
    funcs_ = funcs;
    // 替换之前创建的临时 treeSet_
    treeSet_ = treeSet;
    
    // 设置词法分析器选项
    LexOptions options;
    options.emitComment = (mode_ & Mode::ParseComments) != Mode::ParseNone;
    options.breakOK = !hasFunction("break");
    options.continueOK = !hasFunction("continue");
    
    lex_->setOptions(options);
}



// 停止解析
void Tree::stopParse() {
    lex_ = nullptr;
    vars_.clear();
    funcs_.clear();
    treeSet_.clear();
}

// 添加树到treeSet_
// void Tree::add() {
//     auto it = treeSet_.find(name_);
//     if (it == treeSet_.end() || IsEmptyTree(it->second->root_.get())) {
//         treeSet_[name_] = this->shared_from_this();
//         return;
//     }
    
//     if (!IsEmptyTree(root_.get())) {
//         errorf("template: multiple definition of template %q", name_.c_str());
//     }
// }
// void Tree::add() {
//     std::cout << "Tree::add 方法开始，名称: " << name_ << std::endl;
    
//     if (!root_) {
//         std::cout << "警告: 根节点为空!" << std::endl;
//     }
    
//     auto it = treeSet_.find(name_);
//     if (it == treeSet_.end() || IsEmptyTree(it->second->root_.get())) {
//         std::cout << "将树添加到treeSet中，树名: " << name_ << std::endl;
//         treeSet_[name_] = this->shared_from_this();
//         std::cout << "添加完成，检查treeSet大小: " << treeSet_.size() << std::endl;
//         return;
//     }
    
//     if (!IsEmptyTree(root_.get())) {
//         std::cout << "错误: 模板重复定义: " << name_ << std::endl;
//         errorf("template: multiple definition of template %q", name_.c_str());
//     }
    
//     std::cout << "Tree::add 方法完成" << std::endl;
// }

void Tree::add() {
    std::cout << "Tree::add 方法开始，名称: " << name_ << std::endl;
    
    if (!root_) {
        std::cout << "警告: 根节点为空!" << std::endl;
        return; // 如果根节点为空就不添加
    }
    
    auto it = treeSet_.find(name_);
    if (it == treeSet_.end() || IsEmptyTree(it->second->root_.get())) {
        std::cout << "将树添加到treeSet中，树名: " << name_ << std::endl;
        // 使用 shared_from_this() 获取当前对象的 shared_ptr
        treeSet_[name_] = shared_from_this();
        std::cout << "添加完成，检查treeSet大小: " << treeSet_.size() << std::endl;
    } else if (!IsEmptyTree(root_.get())) {
        std::cout << "错误: 模板重复定义: " << name_ << std::endl;
        errorf("template: multiple definition of template %q", name_.c_str());
    }
}




// 获取下一个token
Item Tree::next() {
    if (peekCount_ > 0) {
        peekCount_--;
    } else {
        token_[0] = lex_->nextItem();
    }
    return token_[peekCount_];
}

// 备份输入流
void Tree::backup() {
    peekCount_++;
}

// 备份两个token
void Tree::backup2(const Item& t1) {
    token_[1] = t1;
    peekCount_ = 2;
}

// 备份三个token
void Tree::backup3(const Item& t2, const Item& t1) {
    token_[1] = t1;
    token_[2] = t2;
    peekCount_ = 3;
}

// 查看但不消费下一个token
Item Tree::peek() {
    if (peekCount_ > 0) {
        return token_[peekCount_ - 1];
    }
    peekCount_ = 1;
    token_[0] = lex_->nextItem();
    return token_[0];
}

// 获取下一个非空白token
Item Tree::nextNonSpace() {
    Item token;
    for (;;) {
        token = next();
        if (token.type != ItemType::ItemSpace) {
            break;
        }
    }
    return token;
}

// 查看但不消费下一个非空白token
Item Tree::peekNonSpace() {
    Item token = nextNonSpace();
    backup();
    return token;
}

// 错误格式化
void Tree::errorf(const std::string& format, ...) {
    va_list args;
    va_start(args, format);
    std::string msg = "template: " + parseName_ + ":" + 
                      std::to_string(token_[0].line) + ": " + 
                      buildErrorMsg(format.c_str(), args);
    va_end(args);
    
    root_ = nullptr;
    throw ParseError(msg);
}

// 简单错误
void Tree::error(const std::string& msg) {
    errorf("%s", msg.c_str());
}

// 期望下一个token具有指定类型
Item Tree::expect(ItemType expected, const std::string& context) {
    Item token = nextNonSpace();
    if (token.type != expected) {
        unexpected(token, context);
    }
    return token;
}

// 期望下一个token具有两种可能类型之一
Item Tree::expectOneOf(ItemType expected1, ItemType expected2, const std::string& context) {
    Item token = nextNonSpace();
    if (token.type != expected1 && token.type != expected2) {
        unexpected(token, context);
    }
    return token;
}

// 处理意外的token
void Tree::unexpected(const Item& token, const std::string& context) {
    if (token.type == ItemType::ItemError) {
        std::string extra = "";
        if (actionLine_ != 0 && actionLine_ != token.line) {
            extra = " in action started at " + parseName_ + ":" + std::to_string(actionLine_);
        }
        errorf("%s%s", token.val.c_str(), extra.c_str());
    }
    errorf("unexpected %s in %s", token.toString().c_str(), context.c_str());
}

// 清除actionLine
void Tree::clearActionLine() {
    actionLine_ = 0;
}

// 检查函数是否存在
bool Tree::hasFunction(const std::string& name) const {
    for (const auto& funcMap : funcs_) {
        if (funcMap.find(name) != funcMap.end()) {
            return true;
        }
    }
    return false;
}

// 弹出变量列表到指定长度
void Tree::popVars(size_t n) {
    vars_.resize(n);
}

// 使用变量，如果未定义则报错
std::unique_ptr<VariableNode> Tree::useVar(Pos pos, const std::string& name) {
    auto v = newVariable(pos, name);
    for (const auto& varName : vars_) {
        if (varName == v->Ident()[0]) {
            return v;
        }
    }
    errorf("undefined variable %q", v->Ident()[0].c_str());
    return nullptr; // 不会到达这里
}

// 获取错误上下文
std::pair<std::string, std::string> Tree::ErrorContext(const Node* n) const {
    Pos pos = n->Position();
    Tree* nodeTree = n->GetTree();
    if (!nodeTree) {
        nodeTree = const_cast<Tree*>(this);
    }
    
    std::string text = nodeTree->text_.substr(0, pos);
    size_t byteNum = text.find_last_of('\n');
    if (byteNum == std::string::npos) {
        byteNum = pos; // 在第一行
    } else {
        byteNum++; // 在换行符之后
        byteNum = pos - byteNum;
    }
    
    int lineNum = 1 + std::count(text.begin(), text.end(), '\n');
    std::string context = n->String();
    
    std::stringstream ss;
    ss << nodeTree->parseName_ << ":" << lineNum << ":" << byteNum;
    
    return {ss.str(), context};
}

// 主解析方法
void Tree::parse() {
    std::cout << "开始解析模板: " << name_ << std::endl;

    root_ = newList(peek().pos);
    
    while (peek().type != ItemType::ItemEOF) {

        std::cout << "处理Token: " << itemTypeToString(peek().type) << " 值: " << peek().val << std::endl;

        if (peek().type == ItemType::ItemLeftDelim) {
            Item delim = next();
            if (nextNonSpace().type == ItemType::ItemDefine) {
                auto newT = std::make_shared<Tree>("definition"); // 名称将在知道它后更新
                newT->text_ = text_;
                newT->mode_ = mode_;
                newT->parseName_ = parseName_;
                newT->startParse(funcs_, lex_, treeSet_);
                newT->parseDefinition();
                continue;
            }
            backup2(delim);
        }
        
        auto n = textOrAction();
        if (n->Type() == NodeType::NodeEnd || n->Type() == NodeType::NodeElse) {
            errorf("unexpected %s", n->String().c_str());
        } else {
            root_->Append(std::move(n));
        }
    }
}

// 解析定义
void Tree::parseDefinition() {
    const std::string context = "define clause";
    Item name = expectOneOf(ItemType::ItemString, ItemType::ItemRawString, context);
    
    // 去掉字符串的引号
    std::string nameStr = name.val;
    // 简单的去引号实现
    if (!nameStr.empty() && (nameStr.front() == '"' || nameStr.front() == '`') && 
        nameStr.back() == nameStr.front()) {
        nameStr = nameStr.substr(1, nameStr.length() - 2);
    }
    
    name_ = nameStr;
    expect(ItemType::ItemRightDelim, context);
    
    std::unique_ptr<Node> end;
    std::tie(root_, end) = itemList();
    
    if (end->Type() != NodeType::NodeEnd) {
        errorf("unexpected %s in %s", end->String().c_str(), context.c_str());
    }
    
    add();
    stopParse();
}

// 解析项目列表
std::pair<std::unique_ptr<ListNode>, std::unique_ptr<Node>> Tree::itemList() {
    auto list = newList(peekNonSpace().pos);
    
    while (peekNonSpace().type != ItemType::ItemEOF) {
        auto n = textOrAction();
        if (n->Type() == NodeType::NodeEnd || n->Type() == NodeType::NodeElse) {
            return {std::move(list), std::move(n)};
        }
        list->Append(std::move(n));
    }
    
    errorf("unexpected EOF");
    return {nullptr, nullptr}; // 不会到达这里
}

// 解析文本或动作
std::unique_ptr<Node> Tree::textOrAction() {
    Item token = nextNonSpace();
    std::unique_ptr<Node> result; // 声明但不初始化
    
    switch (token.type) {
        case ItemType::ItemText:
            result = newText(token.pos, token.val);
            break;
        case ItemType::ItemLeftDelim:
            actionLine_ = token.line;
            result = action();
            clearActionLine();
            break;
        case ItemType::ItemComment:
            result = newComment(token.pos, token.val);
            break;
        default:
            unexpected(token, "input");
            // 不会到达这里
            break;
    }
    
    return result;
}

// 解析动作
// std::unique_ptr<Node> Tree::action() {
//     Item token = nextNonSpace();
    
//     switch (token.type) {
//         case ItemType::ItemBlock:
//             return blockControl();
//         case ItemType::ItemBreak:
//             return breakControl(token.pos, token.line);
//         case ItemType::ItemContinue:
//             return continueControl(token.pos, token.line);
//         case ItemType::ItemElse:
//             return elseControl();
//         case ItemType::ItemEnd:
//             return endControl();
//         case ItemType::ItemIf:
//             return ifControl();
//         case ItemType::ItemRange:
//             return rangeControl();
//         case ItemType::ItemTemplate:
//             return templateControl();
//         case ItemType::ItemWith:
//             return withControl();
//         default:
//             backup();
//             return newAction(peek().pos, peek().line, pipeline("command", ItemType::ItemRightDelim));
//     }
// }

std::unique_ptr<Node> Tree::action() {
    Item token = nextNonSpace();
    std::cout << "  Action token: " << itemTypeToString(token.type) << " 值: " << token.val << std::endl;
    
    try {
        switch (token.type) {
            case ItemType::ItemBlock:
                return blockControl();
            case ItemType::ItemBreak:
                return breakControl(token.pos, token.line);
            case ItemType::ItemContinue:
                return continueControl(token.pos, token.line);
            case ItemType::ItemElse:
                return elseControl();
            case ItemType::ItemEnd:
                return endControl();
            case ItemType::ItemIf:
                return ifControl();
            case ItemType::ItemRange:
                return rangeControl();
            case ItemType::ItemTemplate:
                return templateControl();
            case ItemType::ItemWith:
                return withControl();
            default:
                std::cout << "  处理管道表达式" << std::endl;
                backup();
                auto pipe = pipeline("command", ItemType::ItemRightDelim);
                std::cout << "  管道解析完成: " << pipe->String() << std::endl;
                return newAction(peek().pos, peek().line, std::move(pipe));
        }
    } catch (const std::exception& e) {
        std::cout << "  Action处理异常: " << e.what() << std::endl;
        throw;
    }
}

// 解析break控制
std::unique_ptr<Node> Tree::breakControl(Pos pos, int line) {
    Item token = nextNonSpace();
    if (token.type != ItemType::ItemRightDelim) {
        unexpected(token, "{{break}}");
    }
    
    if (rangeDepth_ == 0) {
        errorf("{{break}} outside {{range}}");
    }
    
    return newBreak(pos, line);
}

// 解析continue控制
std::unique_ptr<Node> Tree::continueControl(Pos pos, int line) {
    Item token = nextNonSpace();
    if (token.type != ItemType::ItemRightDelim) {
        unexpected(token, "{{continue}}");
    }
    
    if (rangeDepth_ == 0) {
        errorf("{{continue}} outside {{range}}");
    }
    
    return newContinue(pos, line);
}

// 解析控制结构
std::tuple<Pos, int, std::unique_ptr<PipeNode>, std::unique_ptr<ListNode>, std::unique_ptr<ListNode>> 
Tree::parseControl(bool allowElseIf, const std::string& context) {
    size_t varsSize = vars_.size();
    auto pipe = pipeline(context, ItemType::ItemRightDelim);
    
    if (context == "range") {
        rangeDepth_++;
    }
    
    std::unique_ptr<Node> next;
    auto list = std::unique_ptr<ListNode>(nullptr);
    std::tie(list, next) = itemList();
    
    if (context == "range") {
        rangeDepth_--;
    }
    
    auto elseList = std::unique_ptr<ListNode>(nullptr);
    
    if (next->Type() == NodeType::NodeEnd) {
        // 结束
    } else if (next->Type() == NodeType::NodeElse) {
        if (allowElseIf) {
            // 特殊情况处理"else if"
            if (peek().type == ItemType::ItemIf) {
                this -> next();  // 消费"if"token
                elseList = newList(next->Position());
                elseList->Append(ifControl());
                // 不消费下一个item，只需要一个{{end}}
                popVars(varsSize);
                return {pipe->Position(), pipe->Line(), std::move(pipe), std::move(list), std::move(elseList)};
            }
        }
        
        std::tie(elseList, next) = itemList();
        if (next->Type() != NodeType::NodeEnd) {
            errorf("expected end; found %s", next->String().c_str());
        }
    }
    
    popVars(varsSize);
    // return {pipe->Position(), pipe->Line(), std::move(pipe), std::move(list), std::move(elseList)};
    return std::make_tuple(pipe->Position(), pipe->Line(), std::move(pipe), std::move(list), std::move(elseList));

}

// 解析if控制
std::unique_ptr<Node> Tree::ifControl() {
    auto [pos, line, pipe, list, elseList] = parseControl(true, "if");
    return newIf(pos, line, std::move(pipe), std::move(list), std::move(elseList));
}

// 解析range控制
std::unique_ptr<Node> Tree::rangeControl() {
    auto [pos, line, pipe, list, elseList] = parseControl(false, "range");
    return newRange(pos, line, std::move(pipe), std::move(list), std::move(elseList));
}

// 解析with控制
std::unique_ptr<Node> Tree::withControl() {
    auto [pos, line, pipe, list, elseList] = parseControl(false, "with");
    return newWith(pos, line, std::move(pipe), std::move(list), std::move(elseList));
}

// 解析end控制
std::unique_ptr<Node> Tree::endControl() {
    return newEnd(expect(ItemType::ItemRightDelim, "end").pos);
}

// 解析else控制
std::unique_ptr<Node> Tree::elseControl() {
    // 特殊情况处理"else if"
    Item peek = peekNonSpace();
    if (peek.type == ItemType::ItemIf) {
        // 我们看到"{{else if ... "但实际上将其重写为"{{else}}{{if ... "
        return newElse(peek.pos, peek.line);
    }
    
    Item token = expect(ItemType::ItemRightDelim, "else");
    return newElse(token.pos, token.line);
}

// 解析block控制
std::unique_ptr<Node> Tree::blockControl() {
    const std::string context = "block clause";
    
    Item token = nextNonSpace();
    std::string name = parseTemplateName(token, context);
    auto pipe = pipeline(context, ItemType::ItemRightDelim);
    
    auto block = std::make_shared<Tree>(name);
    block->text_ = text_;
    block->mode_ = mode_;
    block->parseName_ = parseName_;
    block->startParse(funcs_, lex_, treeSet_);
    
    std::unique_ptr<Node> end;
    std::tie(block->root_, end) = block->itemList();
    
    if (end->Type() != NodeType::NodeEnd) {
        errorf("unexpected %s in %s", end->String().c_str(), context.c_str());
    }
    
    block->add();
    block->stopParse();
    
    return newTemplate(token.pos, token.line, name, std::move(pipe));
}

// 解析template控制
std::unique_ptr<Node> Tree::templateControl() {
    const std::string context = "template clause";
    Item token = nextNonSpace();
    std::string name = parseTemplateName(token, context);
    
    std::unique_ptr<PipeNode> pipe = nullptr;
    if (nextNonSpace().type != ItemType::ItemRightDelim) {
        backup();
        pipe = pipeline(context, ItemType::ItemRightDelim);
    }
    
    return newTemplate(token.pos, token.line, name, std::move(pipe));
}

// 解析模板名称
std::string Tree::parseTemplateName(const Item& token, const std::string& context) {
    std::string name;
    
    if (token.type == ItemType::ItemString || token.type == ItemType::ItemRawString) {
        // 去掉字符串的引号
        name = token.val;
        // 简单的去引号实现
        if (!name.empty() && (name.front() == '"' || name.front() == '`') && 
            name.back() == name.front()) {
            name = name.substr(1, name.length() - 2);
        }
    } else {
        unexpected(token, context);
    }
    
    return name;
}

// 解析管道
// std::unique_ptr<PipeNode> Tree::pipeline(const std::string& context, ItemType end) {
//     Item token = peekNonSpace();
//     auto pipe = newPipeline(token.pos, token.line);
    
//     // 是否有声明或赋值？
//     if (peekNonSpace().type == ItemType::ItemVariable) {
//         Item v = next();
//         Item tokenAfterVariable = peek();
//         Item nextToken = peekNonSpace();
        
//         if (nextToken.type == ItemType::ItemAssign || nextToken.type == ItemType::ItemDeclare) {
//             pipe->SetIsAssign(nextToken.type == ItemType::ItemAssign);
//             nextNonSpace(); // 消费赋值token
//             pipe->AddDecl(newVariable(v.pos, v.val));
//             vars_.push_back(v.val);
//         } else if (nextToken.type == ItemType::ItemChar && nextToken.val == ",") {
//             nextNonSpace(); // 消费逗号
//             pipe->AddDecl(newVariable(v.pos, v.val));
//             vars_.push_back(v.val);
            
//             if (context == "range" && pipe->Decl().size() < 2) {
//                 ItemType peekType = peekNonSpace().type;
//                 if (peekType == ItemType::ItemVariable || 
//                     peekType == ItemType::ItemRightDelim || 
//                     peekType == ItemType::ItemRightParen) {
//                     // range管道中的第二个初始化变量
//                     goto decls;
//                 }
//                 errorf("range can only initialize variables");
//             }
//             errorf("too many declarations in %s", context.c_str());
//         } else if (tokenAfterVariable.type == ItemType::ItemSpace) {
//             backup3(v, tokenAfterVariable);
//         } else {
//             backup2(v);
//         }
//     }


//      // 检查是否是函数调用
//      if (peek().type == ItemType::ItemIdentifier) {
//         std::string funcName = next().val;
//         std::cout << "  发现函数调用: " << funcName << std::endl;
        
//         // 创建函数命令
//         auto cmd = newCommand(token.pos);
//         auto identNode = std::make_unique<IdentifierNode>(this, token.pos, funcName);
//         cmd->Append(std::move(identNode));
        
//         // 解析函数参数
//         while (peekNonSpace().type != end && peekNonSpace().type != ItemType::ItemPipe) {
//             auto arg = operand();
//             if (arg) {
//                 cmd->Append(std::move(arg));
//             }
//             nextNonSpace(); // 消费空格
//         }
        
//         pipe->Append(std::move(cmd));
//     }
    
//     for (;;) {
//         token = nextNonSpace();
//         if (token.type == end) {
//             // 管道结束
//             checkPipeline(pipe.get(), context);
//             return pipe;
//         }
        
//         // 如果是函数调用
//         if (token.type == ItemType::ItemIdentifier) {
//             std::cout << "  发现函数: " << token.val << std::endl;
//             auto cmd = newCommand(token.pos);
//             auto identNode = std::make_unique<IdentifierNode>(this, token.pos, token.val);
//             cmd->Append(std::move(identNode));
            
//             // 收集函数参数，直到遇到管道符号或右定界符
//             while (true) {
//                 token = nextNonSpace();
//                 if (token.type == end || token.type == ItemType::ItemPipe) {
//                     backup();
//                     break;
//                 }
                
//                 backup();
//                 auto arg = operand();
//                 if (arg) {
//                     std::cout << "  添加函数参数: " << arg->String() << std::endl;
//                     cmd->Append(std::move(arg));
//                 } else {
//                     break;
//                 }
//             }
            
//             pipe->Append(std::move(cmd));
//             continue;
//         }
        
//         switch (token.type) {
//             case ItemType::ItemBool:
//             case ItemType::ItemCharConstant:
//             case ItemType::ItemComplex:
//             case ItemType::ItemDot:
//             case ItemType::ItemField:
//             case ItemType::ItemIdentifier:
//             case ItemType::ItemNumber:
//             case ItemType::ItemNil:
//             case ItemType::ItemRawString:
//             case ItemType::ItemString:
//             case ItemType::ItemVariable:
//             case ItemType::ItemLeftParen:
//                 backup();
//                 pipe->Append(command());
//                 break;
//             default:
//                 unexpected(token, context);
//         }
//     }
    
//     return pipe; // 不会到达这里
    
// decls:
//     // 跳转到这里继续解析range中的声明
//     return pipeline(context, end);
// }

std::unique_ptr<PipeNode> Tree::pipeline(const std::string& context, ItemType end) {
    Item token = peekNonSpace();
    auto pipe = newPipeline(token.pos, token.line);
    
    // 是否有声明或赋值？
    if (peekNonSpace().type == ItemType::ItemVariable) {
        Item v = next();
        Item tokenAfterVariable = peek();
        Item nextToken = peekNonSpace();
        
        if (nextToken.type == ItemType::ItemAssign || nextToken.type == ItemType::ItemDeclare) {
            pipe->SetIsAssign(nextToken.type == ItemType::ItemAssign);
            nextNonSpace(); // 消费赋值token
            pipe->AddDecl(newVariable(v.pos, v.val));
            vars_.push_back(v.val);
        } else if (nextToken.type == ItemType::ItemChar && nextToken.val == ",") {
            nextNonSpace(); // 消费逗号
            pipe->AddDecl(newVariable(v.pos, v.val));
            vars_.push_back(v.val);
            
            if (context == "range" && pipe->Decl().size() < 2) {
                ItemType peekType = peekNonSpace().type;
                if (peekType == ItemType::ItemVariable || 
                    peekType == ItemType::ItemRightDelim || 
                    peekType == ItemType::ItemRightParen) {
                    // range管道中的第二个初始化变量
                    goto decls;
                }
                errorf("range can only initialize variables");
            }
            errorf("too many declarations in %s", context.c_str());
        } else if (tokenAfterVariable.type == ItemType::ItemSpace) {
            backup3(v, tokenAfterVariable);
        } else {
            backup2(v);
        }
    }
    
    // 解析命令和参数
    for (;;) {
        token = nextNonSpace();
        if (token.type == end) {
            // 此时，管道已完成
            checkPipeline(pipe.get(), context);
            return pipe;
        }
        
        switch (token.type) {
            case ItemType::ItemBool:
            case ItemType::ItemCharConstant:
            case ItemType::ItemComplex:
            case ItemType::ItemDot:
            case ItemType::ItemField:
            case ItemType::ItemIdentifier:
            case ItemType::ItemNumber:
            case ItemType::ItemNil:
            case ItemType::ItemRawString:
            case ItemType::ItemString:
            case ItemType::ItemVariable:
            case ItemType::ItemLeftParen:
                backup();
                pipe->Append(command());
                break;
            case ItemType::ItemPipe:
                // 继续解析下一个命令
                break;
            default:
                unexpected(token, context);
        }
    }
    
    return pipe; // 不会到达这里
    
decls:
    // 跳转到这里继续解析range中的声明
    return pipeline(context, end);
}

// 检查管道有效性
void Tree::checkPipeline(const PipeNode* pipe, const std::string& context) {
    // 拒绝空管道
    if (pipe->Cmds().empty()) {
        errorf("missing value for %s", context.c_str());
    }
    
    // 只有管道的第一个命令可以以非可执行操作数开始
    for (size_t i = 1; i < pipe->Cmds().size(); ++i) {
        const auto& c = pipe->Cmds()[i];
        if (!c->Args().empty()) {
            NodeType argType = c->Args()[0]->Type();
            if (argType == NodeType::NodeBool || 
                argType == NodeType::NodeDot || 
                argType == NodeType::NodeNil || 
                argType == NodeType::NodeNumber || 
                argType == NodeType::NodeString) {
                // 对于A|B|C，管道阶段2是B
                errorf("non executable command in pipeline stage %d", i + 2);
            }
        }
    }
}

// 解析命令
// 修改 command 方法来正确解析所有参数
std::unique_ptr<CommandNode> Tree::command() {
    Pos pos = peekNonSpace().pos;
    auto cmd = newCommand(pos);
    
    // 解析命令及其参数
    while (true) {
        // 跳过空格
        if (peek().type == ItemType::ItemSpace) {
            next();
            continue;
        }
        
        // 检查终止符
        if (peek().type == ItemType::ItemRightDelim || 
            peek().type == ItemType::ItemRightParen ||
            peek().type == ItemType::ItemPipe) {
            break; // 命令结束
        }
        
        // 解析参数
        auto arg = operand();
        if (!arg) {
            break; // 没有更多参数
        }
        
        cmd->Append(std::move(arg));
        std::cout << "命令添加参数，现在有 " << cmd->Args().size() << " 个参数" << std::endl;
    }
    
    if (cmd->Args().empty()) {
        errorf("empty command");
    }
    
    return cmd;
}

// 解析操作数
// 修改 operand 方法确保能解析所有类型的参数
std::unique_ptr<Node> Tree::operand() {
    if (peek().type == ItemType::ItemError) {
        next();
        return nullptr;
    }
    
    // 解析各种类型的操作数
    switch (peek().type) {
        case ItemType::ItemIdentifier:
        case ItemType::ItemDot:
        case ItemType::ItemNil:
        case ItemType::ItemVariable:
        case ItemType::ItemField:
        case ItemType::ItemBool:
        case ItemType::ItemNumber:
        case ItemType::ItemString:
        case ItemType::ItemRawString:
        case ItemType::ItemLeftParen:
            return term();
            
        default:
            return nullptr; // 不是有效的操作数
    }
}


