#include "lexer.h"
#include <iostream>
#include <sstream>
#include <cstdarg>
#include <algorithm>
#include <iostream>
#include <iomanip>

// 常量定义
const int EOF_RUNE = -1;
const std::string LEFT_COMMENT = "/*";
const std::string RIGHT_COMMENT = "*/";
const std::string SPACE_CHARS = " \t\r\n";
const char TRIM_MARKER = '-';
const Pos TRIM_MARKER_LEN = 2;  // 标记加前后空格

// 实现Item的toString方法
std::string Item::toString() const {
    switch (type) {
        case ItemType::ItemEOF:
            return "EOF";
        case ItemType::ItemError:
            return val;
        case ItemType::ItemKeyword:
        case ItemType::ItemBlock:
        case ItemType::ItemBreak:
        case ItemType::ItemContinue:
        case ItemType::ItemDot:
        case ItemType::ItemDefine:
        case ItemType::ItemElse:
        case ItemType::ItemEnd:
        case ItemType::ItemIf:
        case ItemType::ItemNil:
        case ItemType::ItemRange:
        case ItemType::ItemTemplate:
        case ItemType::ItemWith:
            return "<" + val + ">";
        default:
            if (val.length() > 10) {
                return "\"" + val.substr(0, 10) + "\"...";
            }
            return "\"" + val + "\"";
    }
}

// 构建关键字映射
std::unordered_map<std::string, ItemType> buildKeywordMap() {
    std::unordered_map<std::string, ItemType> keyMap;
    keyMap["."] = ItemType::ItemDot;
    keyMap["block"] = ItemType::ItemBlock;
    keyMap["break"] = ItemType::ItemBreak;
    keyMap["continue"] = ItemType::ItemContinue;
    keyMap["define"] = ItemType::ItemDefine;
    keyMap["else"] = ItemType::ItemElse;
    keyMap["end"] = ItemType::ItemEnd;
    keyMap["if"] = ItemType::ItemIf;
    keyMap["range"] = ItemType::ItemRange;
    keyMap["nil"] = ItemType::ItemNil;
    keyMap["template"] = ItemType::ItemTemplate;
    keyMap["with"] = ItemType::ItemWith;
    return keyMap;
}

// 工具函数实现

bool isSpace(int r) {
    // 处理空格
    return r == ' ' || r == '\t' || r == '\r' || r == '\n';
}

bool isAlphaNumeric(int r) {
    // 下划线、大小写字母、数字
    return r == '_' || std::isalpha(r) || std::isdigit(r);
}

bool hasLeftTrimMarker(const std::string& s) {
    return s.length() >= 2 && s[0] == TRIM_MARKER && isSpace(s[1]);
}

bool hasRightTrimMarker(const std::string& s) {
    return s.length() >= 2 && isSpace(s[0]) && s[1] == TRIM_MARKER;
}

// 计算左右两侧连续空白字符的长度
Pos rightTrimLength(const std::string& s) {
    size_t len = s.length();
    size_t i = len;
    while (i > 0 && SPACE_CHARS.find(s[i-1]) != std::string::npos) {
        i--;
    }
    return len - i;
}

Pos leftTrimLength(const std::string& s) {
    size_t i = 0;
    while (i < s.length() && SPACE_CHARS.find(s[i]) != std::string::npos) {
        i++;
    }
    return i;
}

// 词法分析器构造函数
Lexer::Lexer(const std::string& name, const std::string& input, 
             const std::string& left, const std::string& right)
    : name_(name), input_(input), 
      leftDelim_(left.empty() ? "{{" : left), 
      rightDelim_(right.empty() ? "}}" : right),
      pos_(0), start_(0), atEOF_(false), parenDepth_(0), line_(1), startLine_(1),
      insideAction_(false), lastPos_(0), lastType_(ItemType::ItemError) {
    
    keyMap_ = buildKeywordMap();
}

// 设置词法分析器选项
void Lexer::setOptions(const LexOptions& options) {
    options_ = options;
}

// 返回输入中的下一个字符
int Lexer::next() {
    if (pos_ >= input_.length()) {
        atEOF_ = true;
        return EOF_RUNE;
    }
    
    char ch = input_[pos_++];
    if (ch == '\n') {
        line_++;
    }
    return ch;
}

// 查看但不消费输入中的下一个字符
int Lexer::peek() const {
    if (pos_ >= input_.length()) {
        return EOF_RUNE;
    }
    return input_[pos_];
}

// 后退一个字符
void Lexer::backup() {
    if (!atEOF_ && pos_ > 0) {
        pos_--;
        if (pos_ < input_.length() && input_[pos_] == '\n') {
            line_--;
        }
    }
}

// 为错误创建一个Item
Item Lexer::errorItem(const std::string& format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format.c_str(), args);
    va_end(args);
    
    return Item(ItemType::ItemError, start_, buffer, startLine_);
}

// 跳过此点之前的待处理输入
void Lexer::ignore() {
    start_ = pos_;
    startLine_ = line_;
}

// 如果下一个字符来自有效集合，则消耗它
bool Lexer::accept(const std::string& valid) {
    int r = next();
    if (r != EOF_RUNE && valid.find(static_cast<char>(r)) != std::string::npos) {
        return true;
    }
    backup();
    return false;
}

// 消耗有效集合中的一组字符
void Lexer::acceptRun(const std::string& valid) {
    while (true) {
        int r = next();
        if (r == EOF_RUNE || valid.find(static_cast<char>(r)) == std::string::npos) {
            break;
        }
    }
    backup();
}

// 测试是否在右分隔符处
std::pair<bool, bool> Lexer::atRightDelim() {
    // 检查带修剪标记的右分隔符
    if (pos_ > 0 && hasRightTrimMarker(input_.substr(pos_-1)) && 
        pos_-1+TRIM_MARKER_LEN+rightDelim_.length() <= input_.length() && 
        input_.substr(pos_-1+TRIM_MARKER_LEN, rightDelim_.length()) == rightDelim_) {
        return {true, true};
    }
    // 检查不带修剪标记的右分隔符
    if (pos_+rightDelim_.length() <= input_.length() && 
        input_.substr(pos_, rightDelim_.length()) == rightDelim_) {
        return {true, false};
    }
    return {false, false};
}

// 测试是否在终止符处
bool Lexer::atTerminator() {
    int r = peek();
    if (isSpace(r)) {
        return true;
    }
    switch (r) {
        case EOF_RUNE:
        case '.':
        case ',':
        case '|':
        case ':':
        case ')':
        case '(':
            return true;
    }
    return pos_+rightDelim_.length() <= input_.length() && 
           input_.substr(pos_, rightDelim_.length()) == rightDelim_;
}

// 扫描数字
bool Lexer::scanNumber() {
    // 可选的前导符号
    accept("+-");
    // 是十六进制吗？
    std::string digits = "0123456789_";
    if (accept("0")) {
        // 注意：浮点数中的前导0不表示八进制
        if (accept("xX")) {
            digits = "0123456789abcdefABCDEF_";
        } else if (accept("oO")) {
            digits = "01234567_";
        } else if (accept("bB")) {
            digits = "01_";
        }
    }
    acceptRun(digits);
    if (accept(".")) {
        acceptRun(digits);
    }
    if (digits.length() == 10+1 && accept("eE")) {
        accept("+-");
        acceptRun("0123456789_");
    }
    if (digits.length() == 16+6+1 && accept("pP")) {
        accept("+-");
        acceptRun("0123456789_");
    }
    // 是虚数吗？
    accept("i");
    // 下一个字符不能是字母数字
    if (isAlphaNumeric(peek())) {
        next();
        return false;
    }
    return true;
}

// 获取下一个词法单元
Item Lexer::nextItem() {
    // 如果已经到达文件结束，返回EOF
    if (atEOF_) {
        return Item(ItemType::ItemEOF, pos_, "EOF", line_);
    }

    // 重置开始位置和行号
    start_ = pos_;
    startLine_ = line_;

    // 根据当前状态调用相应的处理函数
    if (insideAction_) {
        return lexInsideAction();
    } else {
        return lexText();
    }
}

// 获取所有项目
std::vector<Item> Lexer::getAllItems() {
    std::vector<Item> items;
    Item item;
    do {
        item = nextItem();
        items.push_back(item);
    } while (item.type != ItemType::ItemEOF && item.type != ItemType::ItemError);
    return items;
}

// 处理普通文本
Item Lexer::lexText() {
    size_t x = input_.find(leftDelim_, pos_);
    if (x != std::string::npos) {
        if (x > pos_) {
            // 返回文本片段
            pos_ = x;
            Item result(ItemType::ItemText, start_, input_.substr(start_, pos_ - start_), startLine_);
            return result;
        }
        // 处理左分隔符
        return lexLeftDelim();
    }
    
    // 到达输入结束
    pos_ = input_.length();
    if (pos_ > start_) {
        // 返回最后的文本片段
        Item result(ItemType::ItemText, start_, input_.substr(start_, pos_ - start_), startLine_);
        return result;
    }
    // 真正的EOF
    atEOF_ = true;
    return Item(ItemType::ItemEOF, pos_, "EOF", line_);
}

// 处理左分隔符
Item Lexer::lexLeftDelim() {
    pos_ += leftDelim_.length();
    bool trimSpace = false;
    if (pos_ < input_.length() && hasLeftTrimMarker(input_.substr(pos_))) {
        trimSpace = true;
    }
    Pos afterMarker = 0;
    if (trimSpace) {
        afterMarker = TRIM_MARKER_LEN;
    }
    
    // 检查是否是注释
    if (pos_ + afterMarker + LEFT_COMMENT.length() <= input_.length() &&
        input_.substr(pos_ + afterMarker, LEFT_COMMENT.length()) == LEFT_COMMENT) {
        pos_ += afterMarker;
        ignore();
        return lexComment();
    }
    
    // 不是注释，返回左分隔符项
    Item result(ItemType::ItemLeftDelim, start_, input_.substr(start_, pos_ - start_), startLine_);
    insideAction_ = true;
    pos_ += afterMarker;
    parenDepth_ = 0;
    ignore();
    return result;
}

// 处理注释
Item Lexer::lexComment() {
    pos_ += LEFT_COMMENT.length();
    size_t x = input_.find(RIGHT_COMMENT, pos_);
    if (x == std::string::npos) {
        return errorItem("unclosed comment");
    }
    pos_ = x + RIGHT_COMMENT.length();
    auto [delim, trimSpace] = atRightDelim();
    if (!delim) {
        return errorItem("comment ends before closing delimiter");
    }
    Item result(ItemType::ItemComment, start_, input_.substr(start_, pos_ - start_), startLine_);
    if (trimSpace) {
        pos_ += TRIM_MARKER_LEN;
    }
    pos_ += rightDelim_.length();
    if (trimSpace && pos_ < input_.length()) {
        pos_ += leftTrimLength(input_.substr(pos_));
    }
    ignore();
    insideAction_ = false;
    
    if (options_.emitComment) {
        return result;
    }
    
    // 如果不发出注释，就继续解析下一个项
    return nextItem();
}

// 处理右分隔符
Item Lexer::lexRightDelim() {
    auto [_, trimSpace] = atRightDelim();
    if (trimSpace) {
        pos_ += TRIM_MARKER_LEN;
        ignore();
    }
    pos_ += rightDelim_.length();
    Item result(ItemType::ItemRightDelim, start_, input_.substr(start_, pos_ - start_), startLine_);
    if (trimSpace && pos_ < input_.length()) {
        pos_ += leftTrimLength(input_.substr(pos_));
        ignore();
    }
    insideAction_ = false;
    return result;
}

// 处理操作内部的内容
Item Lexer::lexInsideAction() {
    // 检查是否在右分隔符处
    auto [delim, _] = atRightDelim();
    if (delim) {
        if (parenDepth_ == 0) {
            return lexRightDelim();
        }
        return errorItem("unclosed left paren");
    }
    
    int r = next();
    switch (r) {
        case EOF_RUNE:
            return errorItem("unclosed action");
        case ' ': case '\t': case '\r': case '\n':
            backup(); // 放回空格
            return lexSpace();
        case '=':
            return Item(ItemType::ItemAssign, start_, "=", startLine_);
        case ':':
            if (next() != '=') {
                return errorItem("expected :=");
            }
            return Item(ItemType::ItemDeclare, start_, ":=", startLine_);
        case '|':
            return Item(ItemType::ItemPipe, start_, "|", startLine_);
        case '"':
            return lexQuote();
        case '`':
            return lexRawQuote();
        case '$':
            return lexVariable();
        case '\'':
            return lexChar();
        case '.':
            if (pos_ < input_.length()) {
                char ch = input_[pos_];
                if (ch < '0' || ch > '9') {
                    return lexField();
                }
            }
            backup();
            return lexNumber();
        case '+': case '-': case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            backup();
            return lexNumber();
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
        case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
        case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
        case 'v': case 'w': case 'x': case 'y': case 'z': case 'A': case 'B':
        case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
        case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
        case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W':
        case 'X': case 'Y': case 'Z': case '_':
            backup();
            return lexIdentifier();
        case '(':
            parenDepth_++;
            return Item(ItemType::ItemLeftParen, start_, "(", startLine_);
        case ')':
            parenDepth_--;
            if (parenDepth_ < 0) {
                return errorItem("unexpected right paren");
            }
            return Item(ItemType::ItemRightParen, start_, ")", startLine_);
        default:
            if (r <= 127 && std::isprint(r)) {
                return Item(ItemType::ItemChar, start_, std::string(1, static_cast<char>(r)), startLine_);
            }
            return errorItem("unrecognized character in action: %c", r);
    }
}

// 处理空格
Item Lexer::lexSpace() {
    int r;
    int numSpaces = 0;
    while (true) {
        r = peek();
        if (!isSpace(r)) {
            break;
        }
        next();
        numSpaces++;
    }
    
    // 检查修剪标记的右分隔符
    if (pos_ > 0 && hasRightTrimMarker(input_.substr(pos_-1)) && 
        pos_-1+TRIM_MARKER_LEN+rightDelim_.length() <= input_.length() && 
        input_.substr(pos_-1+TRIM_MARKER_LEN, rightDelim_.length()) == rightDelim_) {
        backup(); // 在空格之前
        if (numSpaces == 1) {
            return lexRightDelim(); // 直接处理分隔符
        }
    }
    
    Item result(ItemType::ItemSpace, start_, input_.substr(start_, pos_ - start_), startLine_);
    return result;
}

// 处理标识符
Item Lexer::lexIdentifier() {
    while (true) {
        int r = next();
        if (!isAlphaNumeric(r)) {
            break;
        }
    }
    backup();
    
    std::string word = input_.substr(start_, pos_ - start_);
    if (!atTerminator()) {
        return errorItem("bad character %c", peek());
    }
    
    auto it = keyMap_.find(word);
    if (it != keyMap_.end() && it->second > ItemType::ItemKeyword) {
        ItemType item = it->second;
        if ((item == ItemType::ItemBreak && !options_.breakOK) || 
            (item == ItemType::ItemContinue && !options_.continueOK)) {
            return Item(ItemType::ItemIdentifier, start_, word, startLine_);
        }
        return Item(item, start_, word, startLine_);
    } else if (word == "true" || word == "false") {
        return Item(ItemType::ItemBool, start_, word, startLine_);
    } else {
        return Item(ItemType::ItemIdentifier, start_, word, startLine_);
    }
}

// 处理字段
Item Lexer::lexField() {
    if (pos_ >= input_.length() || !isAlphaNumeric(input_[pos_])) {
        return Item(ItemType::ItemDot, start_, ".", startLine_);
    }
    
    while (true) {
        int r = next();
        if (!isAlphaNumeric(r)) {
            backup();
            break;
        }
    }
    
    if (!atTerminator()) {
        return errorItem("bad character %c", peek());
    }
    
    return Item(ItemType::ItemField, start_, input_.substr(start_, pos_ - start_), startLine_);
}

// 处理变量
Item Lexer::lexVariable() {
    if (atTerminator()) { // 单独的"$"
        return Item(ItemType::ItemVariable, start_, "$", startLine_);
    }
    
    while (true) {
        int r = next();
        if (!isAlphaNumeric(r)) {
            backup();
            break;
        }
    }
    
    if (!atTerminator()) {
        return errorItem("bad character %c", peek());
    }
    
    return Item(ItemType::ItemVariable, start_, input_.substr(start_, pos_ - start_), startLine_);
}

// 处理字符常量
Item Lexer::lexChar() {
    while (true) {
        int r = next();
        if (r == '\\') {
            int r2 = next();
            if (r2 != EOF_RUNE && r2 != '\n') {
                continue;
            }
            return errorItem("unterminated character constant");
        }
        if (r == EOF_RUNE || r == '\n') {
            return errorItem("unterminated character constant");
        }
        if (r == '\'') {
            break;
        }
    }
    return Item(ItemType::ItemCharConstant, start_, input_.substr(start_, pos_ - start_), startLine_);
}

// 处理数字
Item Lexer::lexNumber() {
    if (!scanNumber()) {
        return errorItem("bad number syntax: %s", input_.substr(start_, pos_ - start_).c_str());
    }
    
    int sign = peek();
    if (sign == '+' || sign == '-') {
        // 复数: 1+2i
        next();
        if (!scanNumber() || input_[pos_-1] != 'i') {
            return errorItem("bad number syntax: %s", input_.substr(start_, pos_ - start_).c_str());
        }
        return Item(ItemType::ItemComplex, start_, input_.substr(start_, pos_ - start_), startLine_);
    }
    
    return Item(ItemType::ItemNumber, start_, input_.substr(start_, pos_ - start_), startLine_);
}

// 处理引用字符串
Item Lexer::lexQuote() {
    while (true) {
        int r = next();
        if (r == '\\') {
            int r2 = next();
            if (r2 != EOF_RUNE && r2 != '\n') {
                continue;
            }
            return errorItem("unterminated quoted string");
        }
        if (r == EOF_RUNE || r == '\n') {
            return errorItem("unterminated quoted string");
        }
        if (r == '"') {
            break;
        }
    }
    return Item(ItemType::ItemString, start_, input_.substr(start_, pos_ - start_), startLine_);
}

// 处理原始引用字符串
Item Lexer::lexRawQuote() {
    while (true) {
        int r = next();
        if (r == EOF_RUNE) {
            return errorItem("unterminated raw quoted string");
        }
        if (r == '`') {
            break;
        }
    }
    return Item(ItemType::ItemRawString, start_, input_.substr(start_, pos_ - start_), startLine_);
}

// 创建词法分析器的工厂函数
std::shared_ptr<Lexer> createLexer(const std::string& name, const std::string& input, 
                                const std::string& left, const std::string& right) {
    return std::make_shared<Lexer>(name, input, left, right);
}

// 将词法项类型转换为字符串的辅助函数
std::string itemTypeToString(ItemType type) {
    switch (type) {
        case ItemType::ItemError: return "Error";
        case ItemType::ItemBool: return "Bool";
        case ItemType::ItemChar: return "Char";
        case ItemType::ItemCharConstant: return "CharConstant";
        case ItemType::ItemComment: return "Comment";
        case ItemType::ItemComplex: return "Complex";
        case ItemType::ItemAssign: return "Assign";
        case ItemType::ItemDeclare: return "Declare";
        case ItemType::ItemEOF: return "EOF";
        case ItemType::ItemField: return "Field";
        case ItemType::ItemIdentifier: return "Identifier";
        case ItemType::ItemLeftDelim: return "LeftDelim";
        case ItemType::ItemLeftParen: return "LeftParen";
        case ItemType::ItemNumber: return "Number";
        case ItemType::ItemPipe: return "Pipe";
        case ItemType::ItemRawString: return "RawString";
        case ItemType::ItemRightDelim: return "RightDelim";
        case ItemType::ItemRightParen: return "RightParen";
        case ItemType::ItemSpace: return "Space";
        case ItemType::ItemString: return "String";
        case ItemType::ItemText: return "Text";
        case ItemType::ItemVariable: return "Variable";
        case ItemType::ItemKeyword: return "Keyword";
        case ItemType::ItemBlock: return "Block";
        case ItemType::ItemBreak: return "Break";
        case ItemType::ItemContinue: return "Continue";
        case ItemType::ItemDot: return "Dot";
        case ItemType::ItemDefine: return "Define";
        case ItemType::ItemElse: return "Else";
        case ItemType::ItemEnd: return "End";
        case ItemType::ItemIf: return "If";
        case ItemType::ItemNil: return "Nil";
        case ItemType::ItemRange: return "Range";
        case ItemType::ItemTemplate: return "Template";
        case ItemType::ItemWith: return "With";
        default: return "Unknown";
    }
}




// // 便于打印词法项的辅助函数
// void printItem(const Item& item) {
//     std::cout << "[" << itemTypeToString(item.type) << "] "
//              << "'" << item.val << "' "
//              << "(Line:" << item.line 
//              << ", Pos:" << item.pos << ")\n";
// }

// // 打印标记列表
// void printTokensWithFormat(const std::vector<Item>& items) {
//     std::cout << "┌─────────────────┬──────────────────────────────┬───────┬────────┐\n";
//     std::cout << "│ 类型            │ 值                           │ 行号  │ 位置   │\n";
//     std::cout << "├─────────────────┼──────────────────────────────┼───────┼────────┤\n";
    
//     for (const auto& item : items) {
//         std::string value = item.val;
//         // 替换换行符为可见表示
//         std::string displayValue = "";
//         for (char c : value) {
//             if (c == '\n') displayValue += "\\n";
//             else if (c == '\r') displayValue += "\\r";
//             else if (c == '\t') displayValue += "\\t";
//             else displayValue += c;
//         }
        
//         // 截断过长的值
//         if (displayValue.length() > 30) {
//             displayValue = displayValue.substr(0, 27) + "...";
//         }
        
//         std::cout << "│ " 
//                   << std::left << std::setw(15) << itemTypeToString(item.type) << " │ "
//                   << std::left << std::setw(30) << displayValue << " │ "
//                   << std::setw(5) << item.line << " │ "
//                   << std::setw(6) << item.pos << " │\n";
//     }
    
//     std::cout << "└─────────────────┴──────────────────────────────┴───────┴────────┘\n";
// }

// int main() {
//     // 测试一个简单的模板
//     std::string input = "Hello {{ .Name }}! {{- if gt .Value 42 -}}\nYour value is greater.\n{{- end -}}";
    
//     // 使用默认分隔符创建词法分析器
//     auto lexer = createLexer("test", input);
    
//     // 设置选项 - 启用必要的功能
//     LexOptions options;
//     options.emitComment = true;
//     options.breakOK = true;
//     options.continueOK = true;
//     lexer->setOptions(options);
    
//     // 获取所有词法标记
//     std::vector<Item> items = lexer->getAllItems();
    
//     // 打印标记
//     std::cout << "输入模板: " << input << "\n\n";
//     std::cout << "解析出 " << items.size() << " 个词法单元:\n";
//     printTokensWithFormat(items);
    
//     // 测试一个更复杂的例子，包含修剪标记和注释
//     std::string input2 = "{{ /* This is a comment */ }}\n{{ range .Items }}\n  - {{ .Name }}: {{ .Value }}{{- if .Last }} (last one!){{ end }}\n{{ end }}";
    
//     auto lexer2 = createLexer("test2", input2);
//     lexer2->setOptions(options);
    
//     std::vector<Item> items2 = lexer2->getAllItems();
    
//     std::cout << "\n\n输入模板: " << input2 << "\n\n";
//     std::cout << "解析出 " << items2.size() << " 个词法单元:\n";
//     printTokensWithFormat(items2);
    
//     return 0;
// }