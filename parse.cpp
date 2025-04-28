#include "parse.h"
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <memory>
#include <cstdarg>
#include <algorithm>
#include "lexer.h"  // 直接包含我们的词法分析器

// 辅助函数：从字符串构建错误消息
static std::string buildErrorMsg(const char* format, va_list args) {
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    return std::string(buffer);
}

// Tree构造函数
Tree::Tree(const std::string& name)
    : name_(name), parseName_(name), mode_(ParseNone), root_(NULL),
      peekCount_(0), actionLine_(0), rangeDepth_(0) {
    vars_.push_back("$"); // 初始变量
}

Tree::Tree(const std::string& name, const std::vector<std::map<std::string, std::string> >& funcs)
    : name_(name), parseName_(name), mode_(ParseNone), root_(NULL),
      funcs_(funcs), peekCount_(0), actionLine_(0), rangeDepth_(0) {
    vars_.push_back("$"); // 初始变量
}

// 析构函数 - 释放自己拥有的资源
Tree::~Tree() {
    delete root_;
}

// 创建一个新的解析树
Tree* Tree::Copy() const {
    if (!root_) {
        return NULL;
    }
    
    Tree* copy = new Tree(name_);
    copy->parseName_ = parseName_;
    copy->root_ = static_cast<ListNode*>(root_->Copy());
    copy->text_ = text_;
    
    return copy;
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
        if (token.type != ItemSpace) {
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

// 静态Parse方法，返回模板名称到解析树的映射
std::map<std::string, Tree*> Tree::Parse(
    const std::string& name, 
    const std::string& text, 
    const std::string& leftDelim, 
    const std::string& rightDelim,
    const std::vector<std::map<std::string, std::string> >& funcs) {
    
    std::cout << "开始解析模板: " << name << std::endl;
    // 创建结果容器
    std::map<std::string, Tree*> treeSet;
    
    try {
        // 创建树对象
        Tree* t = new Tree(name, funcs);
        t->text_ = text;
        std::cout << "创建Tree对象成功" << std::endl;
        
        try {
            std::cout << "调用Tree::Parse实例方法" << std::endl;
            // 传入 treeSet 的引用
            Tree* result = t->Parse(text, leftDelim, rightDelim, treeSet, funcs);
            std::cout << "Tree::Parse实例方法完成，treeSet大小: " << treeSet.size() << std::endl;
            
            // 检查每个树的内容
            std::map<std::string, Tree*>::iterator it;
            for (it = treeSet.begin(); it != treeSet.end(); ++it) {
                std::string treeName = it->first;
                Tree* tree = it->second;
                
                std::cout << "树集合中的树: " << treeName << std::endl;
                if (tree && tree->GetRoot()) {
                    std::cout << "  根节点类型: " << static_cast<int>(tree->GetRoot()->Type()) << std::endl;
                    std::cout << "  子节点数量: " << (tree->GetRoot()->Type() == NodeList ? 
                             static_cast<const ListNode*>(tree->GetRoot())->Nodes().size() : 0) << std::endl;
                } else {
                    std::cout << "  树为空或没有根节点" << std::endl;
                }
            }
            
            // 如果 treeSet 为空，则手动添加主树
            if (treeSet.empty() && t->GetRoot()) {
                treeSet[name] = t;
                std::cout << "手动添加主树，现在treeSet大小: " << treeSet.size() << std::endl;
            } else {
                // 如果已经添加到treeSet，就不需要原始树了
                delete t;
            }
            
            // 返回 treeSet
            return treeSet;
        } catch (const ParseError& e) {
            std::cerr << "解析错误: " << e.what() << std::endl;
            delete t; // 清理资源
            throw;
        }
    } catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        // 清理已生成的树
        std::map<std::string, Tree*>::iterator it;
        for (it = treeSet.begin(); it != treeSet.end(); ++it) {
            delete it->second;
        }
        throw;
    }
}

// 实例Parse方法，解析模板
Tree* Tree::Parse(
    const std::string& text, 
    const std::string& leftDelim, 
    const std::string& rightDelim,
    std::map<std::string, Tree*>& treeSet,
    const std::vector<std::map<std::string, std::string> >& funcs) {
    
    // 创建词法分析器
    Lexer* lex = createLexer(name_, text, leftDelim, rightDelim);
    
    // 开始解析
    startParse(funcs, lex, treeSet);
    
    // 执行解析过程
        parse();
    
    // 返回这个树
    return this;
}

// 开始解析，设置初始环境
void Tree::startParse(
    const std::vector<std::map<std::string, std::string> >& funcs,
    Lexer* lex,
    std::map<std::string, Tree*>& treeSet) {
    
    funcs_ = funcs;
    lex_ = lex;
    treeSet_ = treeSet;
}

// 停止解析，清理资源
void Tree::stopParse() {
    lex_ = NULL; // 不负责Lexer的释放
    treeSet_.clear(); // 清除引用
}

// 将当前树添加到treeSet
void Tree::add() {
    std::string name = name_;  // 获取正在解析的模板名称
    treeSet_[name] = Copy();
}

// 抛出错误
void Tree::errorf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::ostringstream lineStr;
    lineStr << token_[0].line;
    std::string msg = "template: " + parseName_ + ":" + lineStr.str() + ": " + 
                      buildErrorMsg(format, args);
    va_end(args);
    
    delete root_;
    root_ = NULL;
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
    if (token.type == ItemError) {
        std::string extra = "";
        if (actionLine_ != 0 && actionLine_ != token.line) {
            std::ostringstream oss;
            oss << actionLine_;
            extra = " in action started at " + parseName_ + ":" + oss.str();
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
    for (size_t i = 0; i < funcs_.size(); ++i) {
        std::map<std::string, std::string>::const_iterator it = funcs_[i].find(name);
        if (it != funcs_[i].end()) {
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
VariableNode* Tree::useVar(Pos pos, const std::string& name) {
    VariableNode* v = newVariable(pos, name);
    for (size_t i = 0; i < vars_.size(); ++i) {
        if (vars_[i] == v->Ident()) {
            return v;
        }
    }
    std::string ident = v->Ident();
    delete v;
    errorf("undefined variable %s", ident.c_str());
    return NULL; // 不会到达这里
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
    
    int lineNum = 1;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\n') {
            lineNum++;
        }
    }
    
    std::string context = n->String();
    
    std::ostringstream ss;
    ss << nodeTree->parseName_ << ":" << lineNum << ":" << byteNum;
    
    return std::make_pair(ss.str(), context);
}

// 主解析方法
void Tree::parse() {
    std::cout << "开始解析模板: " << name_ << std::endl;
    
    // 创建根节点
    root_ = newList(peek().pos);
    
    // 循环解析模板内容
    while (peek().type != ItemEOF) {
        // 输出当前标记信息
        Item token = peek();
        std::cout << "当前标记: 类型=" << itemTypeToString(token.type) 
                 << " 值=\"" << token.val << "\" 行=" << token.line 
                 << " 位置=" << token.pos << std::endl;

        // 处理模板定义
        if (token.type == ItemLeftDelim) {
            // 先获取左分隔符
            Item delim = next();
            
            // 查看后续是否是定义
            Item next = nextNonSpace();
            std::cout << "  检查是否是定义: " << itemTypeToString(next.type) << std::endl;
            
            if (next.type == ItemDefine) {
                // 处理模板定义
                std::cout << "  发现模板定义，创建子模板" << std::endl;
                Tree* newT = new Tree("definition");
                newT->text_ = text_;
                newT->mode_ = mode_;
                newT->parseName_ = parseName_;
                newT->startParse(funcs_, lex_, treeSet_);
                newT->parseDefinition();
                continue;
            }
            
            // 不是定义，回退所有token
            std::cout << "  不是定义，回退token" << std::endl;
            backup2(delim);
        }
        
        // 解析文本或动作
        std::cout << "解析文本或动作..." << std::endl;
        Node* n = textOrAction();
        
        // 处理end和else特殊情况
        if (n->Type() == NodeEnd || n->Type() == NodeElse) {
            std::string errorStr = n->String();
            delete n;
            errorf("unexpected %s", errorStr.c_str());
        } else {
            // 打印节点信息
            std::cout << "添加节点: 类型=" << n->Type() 
                     << " 位置=" << n->Position() << std::endl;
            std::cout << std::endl; // 打印空行
            root_->Append(n);
        }
    }
    
    std::cout << "模板解析完成: " << name_ << std::endl;
}

// 解析定义
void Tree::parseDefinition() {
    const std::string context = "define clause";
    Item name = expectOneOf(ItemString, ItemRawString, context);
    
    // 去掉字符串的引号
    std::string nameStr = name.val;
    // 简单的去引号实现
    if (!nameStr.empty() && (nameStr[0] == '"' || nameStr[0] == '`') && 
        nameStr[nameStr.length() - 1] == nameStr[0]) {
        nameStr = nameStr.substr(1, nameStr.length() - 2);
    }
    
    name_ = nameStr;
    expect(ItemRightDelim, context);
    
    ListNode* list = NULL;
    Node* end = NULL;
    std::pair<ListNode*, Node*> result = itemList();
    list = result.first;
    end = result.second;
    
    if (end->Type() != NodeEnd) {
        std::string errorStr = end->String();
        delete list;
        delete end;
        errorf("unexpected %s in %s", errorStr.c_str(), context.c_str());
    }
    
    delete root_;
    delete end;
    root_ = list;
    add();
    stopParse();
}

// 解析项目列表
std::pair<ListNode*, Node*> Tree::itemList() {
    ListNode* list = newList(peekNonSpace().pos);
    
    while (peekNonSpace().type != ItemEOF) {
        Node* n = textOrAction();
        if (n->Type() == NodeEnd || n->Type() == NodeElse) {
            return std::make_pair(list, n);
        }
        list->Append(n);
    }
    
    errorf("unexpected EOF");
    return std::make_pair(static_cast<ListNode*>(NULL), static_cast<Node*>(NULL)); // 不会到达这里
}

// 解析文本或动作
Node* Tree::textOrAction() {
    Item token = nextNonSpace();
    std::cout << "处理token: 类型=" << itemTypeToString(token.type) 
             << " 值=\"" << token.val << "\"" << std::endl;
    
    Node* result = NULL;
    
    switch (token.type) {
        case ItemText:
            std::cout << "  创建文本节点" << std::endl;
            result = newText(token.pos, token.val);
            break;
            
        case ItemLeftDelim:
            std::cout << "  开始解析动作" << std::endl;
            actionLine_ = token.line;
            result = action();
            clearActionLine();
            break;
            
        case ItemComment:
            std::cout << "  创建注释节点" << std::endl;
            result = newComment(token.pos, token.val);
            break;
            
        default:
            std::cout << "  遇到意外token: " << itemTypeToString(token.type) << std::endl;
            unexpected(token, "input");
            break; // 不会到达这里
    }
    
    if (result) {
        std::cout << "  创建节点: 类型=" << result->Type() << std::endl;
    }
    
    return result;
}

// 检查下一个非空格token是否为指定类型
bool Tree::lookAhead(ItemType itemType) {
    Item token = nextNonSpace();
    bool result = token.type == itemType;
                backup();
    return result;
}

// 解析管道
PipeNode* Tree::pipeline(const std::string& context, ItemType end) {
    Item token = peekNonSpace();
    PipeNode* pipe = newPipeline(token.pos, token.line);
    
    std::cout << "解析pipeline，期望的结束标记: " << itemTypeToString(end) << std::endl;
    
    // 创建一个命令节点
    CommandNode* cmd = newCommand(token.pos);
    while (peekNonSpace().type != end && peekNonSpace().type != ItemEOF) {
        // 跳过空格
        while (peekNonSpace().type == ItemSpace) nextNonSpace();

        // 字段参数：每次只吃一组连续的字段
        if (peekNonSpace().type == ItemField) {
            std::vector<Item> fieldBuffer;
            while (peek().type == ItemField) {
                fieldBuffer.push_back(next());
            }
            // 合并为ChainNode或FieldNode
            if (fieldBuffer.size() == 1) {
                FieldNode* baseField = newField(fieldBuffer[0].pos, fieldBuffer[0].val);
                cmd->Append(baseField);
                std::cout << "    添加单个字段节点: " << fieldBuffer[0].val << std::endl;
            } else {
                FieldNode* baseField = newField(fieldBuffer[0].pos, fieldBuffer[0].val);
                ChainNode* chainNode = newChain(fieldBuffer[0].pos, baseField);
                for (size_t i = 1; i < fieldBuffer.size(); ++i) {
                    std::string name = fieldBuffer[i].val;
                    if (i > 0 && !name.empty() && name[0] == '.') name = name.substr(1);
                    if (i > 0) std::cout << ".";
                    std::cout << name;
                    chainNode->AddField(name);
                }
                std::cout << std::endl;
                cmd->Append(chainNode);
            }
            continue;
        }

        // 其它类型参数
        Item token = peekNonSpace();
        if (token.type == end || token.type == ItemEOF) break;
        token = nextNonSpace();
        switch (token.type) {
            case ItemDot:
                std::cout << "    添加点节点" << std::endl;
                cmd->Append(newDot(token.pos));
                break;
            case ItemIdentifier:
                std::cout << "    添加标识符节点: " << token.val << std::endl;
                cmd->Append(newIdentifier(token.pos, token.val));
                break;
            case ItemString: {
                std::string s = token.val;
                if (s.length() >= 2 && s[0] == '"' && s[s.length()-1] == '"') {
                    s = s.substr(1, s.length()-2);
                }
                std::cout << "    添加字符串节点: " << s << std::endl;
                cmd->Append(newString(token.pos, token.val, s));
                break;
            }
            case ItemBool:
                std::cout << "    添加布尔节点: " << token.val << std::endl;
                cmd->Append(newBool(token.pos, token.val == "true"));
                break;
            case ItemNumber:
                std::cout << "    添加数字节点: " << token.val << std::endl;
                cmd->Append(newNumber(token.pos, token.val));
                break;
            case ItemPipe:
                std::cout << "    结束当前命令，开始新命令" << std::endl;
                pipe->Append(cmd);
                cmd = newCommand(token.pos);
                break;
            case ItemVariable:
                std::cout << "    添加变量节点: " << token.val << std::endl;
                cmd->Append(useVar(token.pos, token.val));
                break;
            case ItemLeftParen: {
                std::cout << "    解析括号表达式参数..." << std::endl;
                PipeNode* subPipe = pipeline("parenthesized pipeline", ItemRightParen);
                cmd->Append(subPipe);
                break;
            }
            default:
                std::cout << "    未知token类型: " << itemTypeToString(token.type) << std::endl;
                delete pipe;
                delete cmd;
                unexpected(token, context);
                return NULL;
        }
    }
    
    // 添加最后一个命令
    if (cmd) {
        pipe->Append(cmd);
    }
    
    // 确保找到结束标记
    Item endToken = nextNonSpace();
    if (endToken.type != end) {
        std::cout << "  未找到期望的结束标记，得到: " << itemTypeToString(endToken.type) << std::endl;
        delete pipe;
        unexpected(endToken, context);
    } else {
        std::cout << "  找到结束标记: " << itemTypeToString(endToken.type) << std::endl;
    }
    
    return pipe;
}

// 解析控制结构
Tree::ControlResult Tree::parseControl(bool allowElseIf, const std::string& context) {
    Item token = peek(); // 查看条件管道的第一个 token
    Pos begin = token.pos;
    int line = token.line;
    ListNode* list = NULL;
    ListNode* elseList = NULL;
    
    PipeNode* pipe = pipeline(context, ItemRightDelim); // 现在 pipeline 从正确的 token 开始解析
    
    // 解析列表
    ListNode* tempList = NULL;
    Node* tempNext = NULL;
    std::pair<ListNode*, Node*> result = itemList();
    tempList = result.first;
    tempNext = result.second;
    
    // 解析else部分
    if (tempNext->Type() == NodeElse) {
        if (allowElseIf && lookAhead(ItemIf)) {
            // else if
            ListNode* tempElseList = newList(peek().pos);
            tempElseList->Append(ifControl());
            elseList = tempElseList;
            
            // 继续解析，等待end
            delete tempNext;
            result = itemList();
            list = tempList;
            tempNext = result.second;
            
            if (tempNext->Type() != NodeEnd) {
                std::string errorStr = tempNext->String();
                delete tempList;
                delete tempNext;
                delete pipe;
                delete elseList;
                errorf("expected end; found %s", errorStr.c_str());
            }
        } else {
            // 普通else
            delete tempNext;
            result = itemList();
            elseList = result.first;
            tempNext = result.second;
            list = tempList;
            
            if (tempNext->Type() != NodeEnd) {
                std::string errorStr = tempNext->String();
                delete tempList;
                delete tempNext;
                delete pipe;
                delete elseList;
                errorf("expected end; found %s", errorStr.c_str());
            }
        }
    } else if (tempNext->Type() != NodeEnd) {
        std::string errorStr = tempNext->String();
        delete tempList;
        delete tempNext;
        delete pipe;
        errorf("expected end; found %s", errorStr.c_str());
    } else {
        list = tempList;
    }
    
    delete tempNext; // 删除end节点
    
    // 构造返回结果
    Tree::ControlResult cr;
    cr.pos = begin;
    cr.line = line;
    cr.pipe = pipe;
    cr.list = list;
    cr.elseList = elseList;
    
    return cr;
}

// ControlResult析构函数
Tree::ControlResult::~ControlResult() {
    // 不删除任何指针，因为它们会被树拥有
    // 这里只是传递引用
}

// 解析动作
Node* Tree::action() {
    Item token = nextNonSpace();
    std::cout << "动作token: 类型=" << itemTypeToString(token.type) 
             << " 值=\"" << token.val << "\"" << std::endl;
    
    switch (token.type) {
        case ItemBlock:
            std::cout << "  解析block控制" << std::endl;
            return blockControl();
            
        case ItemBreak:
            std::cout << "  解析break控制" << std::endl;
            return breakControl(token.pos, token.line);
            
        case ItemContinue:
            std::cout << "  解析continue控制" << std::endl;
            return continueControl(token.pos, token.line);
            
        case ItemElse:
            std::cout << "  解析else控制" << std::endl;
            return elseControl();
            
        case ItemEnd:
            std::cout << "  解析end控制" << std::endl;
            return endControl();
            
        case ItemIf:
            std::cout << "  解析if控制" << std::endl;
            return ifControl();
            
        case ItemRange:
            std::cout << "  解析range控制" << std::endl;
            return rangeControl();
            
        case ItemTemplate:
            std::cout << "  解析template控制" << std::endl;
            return templateControl();
            
        case ItemWith:
            std::cout << "  解析with控制" << std::endl;
            return withControl();
            
        default:
            std::cout << "  解析普通action" << std::endl;
            backup();
            return newAction(peek().pos, peek().line, pipeline("command", ItemRightDelim));
    }
}

// 解析block控制
Node* Tree::blockControl() {
    // 简单实现
    return newEnd(peek().pos);
}

// 解析break控制
Node* Tree::breakControl(Pos pos, int line) {
    if (rangeDepth_ <= 0) {
        errorf("{{break}} outside of range");
    }
    expect(ItemRightDelim, "break");
    return newBreak(pos, line);
}

// 解析continue控制
Node* Tree::continueControl(Pos pos, int line) {
    if (rangeDepth_ <= 0) {
        errorf("{{continue}} outside of range");
    }
    expect(ItemRightDelim, "continue");
    return newContinue(pos, line);
}

// 解析else控制
Node* Tree::elseControl() {
    // 特殊情况处理"else if"
    Item peek = peekNonSpace();
    if (peek.type == ItemIf) {
        // 我们看到"{{else if ... "但实际上将其重写为"{{else}}{{if ... "
        return newElse(peek.pos, peek.line);
    }
    
    Item token = expect(ItemRightDelim, "else");
    return newElse(token.pos, token.line);
}

// 解析if控制
Node* Tree::ifControl() {
    // std::cout << "  解析if控制" << std::endl;
    std::cout << "  解析if控制 (通用)" << std::endl; // 区分日志
    
    // --- 移除 eq 函数的特殊硬编码处理 --- 
    /*
    Item peekItem = peekNonSpace();
    if (peekItem.type == ItemIdentifier && peekItem.val == "eq") {
        // ... (大量被删除的 eq 特殊处理代码) ...
        return newIf(peekItem.pos, peekItem.line, pipe, body, elseBody);
    }
    */
    
// normal_if: // 标签不再需要
    // 常规if解析，交给 parseControl 处理
    ControlResult cr = parseControl(true, "if");
    return newIf(cr.pos, cr.line, cr.pipe, cr.list, cr.elseList);
}

// 解析range控制
Node* Tree::rangeControl() {
    rangeDepth_++;
    ControlResult cr = parseControl(false, "range");
    rangeDepth_--;
    return newRange(cr.pos, cr.line, cr.pipe, cr.list, cr.elseList);
}

// 解析with控制
Node* Tree::withControl() {
    ControlResult cr = parseControl(false, "with");
    return newWith(cr.pos, cr.line, cr.pipe, cr.list, cr.elseList);
}

// 解析end控制
Node* Tree::endControl() {
    return newEnd(expect(ItemRightDelim, "end").pos);
}

// 解析模板控制
Node* Tree::templateControl() {
    Item token = expect(ItemRightDelim, "template");
    // TODO: 当前实现是空的，如果需要实际处理 {{template}}，需要补充逻辑
    Node* n = NULL;
    std::cout << "  (STUB) 解析template控制" << std::endl; // 添加日志
    return n; // 返回 NULL，暂不创建 TemplateNode
}

// 解析操作数
Node* Tree::operand() {
    if (peek().type == ItemError) {
        next();
        return NULL;
    }
    // std::cout << "  解析 operand, next token type: " << itemTypeToString(peek().type) << std::endl; // 日志
    
    // 解析各种类型的操作数
    switch (peek().type) {
        case ItemIdentifier:
        case ItemDot:
        case ItemNil:
        case ItemVariable:
        case ItemField:
        case ItemBool:
        case ItemNumber:
        case ItemString:
        case ItemRawString:
        case ItemLeftParen:
            return term();
            
        default:
            // std::cout << "  operand: not a term type." << std::endl; // 日志
            return NULL; // 不是有效的操作数
    }
}

// 创建变量节点
// VariableNode* Tree::newVariable(Pos pos, const std::string& ident) {
//     return new VariableNode(this, pos, ident);
// }

// 创建标识符节点
// IdentifierNode* Tree::newIdentifier(Pos pos, const std::string& ident) {
//     return new IdentifierNode(this, pos, ident);
// }

// 创建点节点
// DotNode* Tree::newDot(Pos pos) {
//     return new DotNode(this, pos);
// }


