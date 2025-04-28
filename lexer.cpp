#include "lexer.h"
#include <iostream>
#include <sstream>
#include <cstdarg>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cctype>
#include <cstdio>
#include <cstring>

// Item 类构造函数实现
Item::Item() : type(ItemError), pos(0), val(""), line(0) {}

Item::Item(ItemType t, Pos p, const std::string& v, int l) 
    : type(t), pos(p), val(v), line(l) {
    std::cout << "Token: " << itemTypeToString(t) << " 行=" << l << " 值=\"" << v << "\"" << std::endl;
}

// 常量定义
const int EOF_RUNE = -1;
const std::string LEFT_COMMENT = "/*";
const std::string RIGHT_COMMENT = "*/";
const std::string SPACE_CHARS = " \t\r\n";
const char TRIM_MARKER = '-';
const Pos TRIM_MARKER_LEN = 2;  // 修剪标记的长度（例如 {{- 或 -}}）

// 将 Item 转换为字符串以供调试
std::string Item::toString() const {
    std::ostringstream oss;
    oss << itemTypeToString(type) << "(" << val << ")";
    return oss.str();
}

// 构建关键字映射
std::map<std::string, ItemType> buildKeywordMap() {
    std::map<std::string, ItemType> m;
    m["block"] = ItemBlock;
    m["break"] = ItemBreak;
    m["continue"] = ItemContinue;
    m["define"] = ItemDefine;
    m["else"] = ItemElse;
    m["end"] = ItemEnd;
    m["if"] = ItemIf;
    m["nil"] = ItemNil;
    m["range"] = ItemRange;
    m["template"] = ItemTemplate;
    m["with"] = ItemWith;
    return m;
}

// 辅助函数

bool isSpace(int r) {
    // 判断是否是空格字符
    return r == ' ' || r == '\t' || r == '\r' || r == '\n';
}

bool isAlphaNumeric(int r) {
    // 判断是否是字母数字字符或下划线
    return isalnum(r) || r == '_';
}

bool hasLeftTrimMarker(const std::string& s) {
    return s.length() >= 2 && s[0] == '-' && s[1] == ' ';
}

bool hasRightTrimMarker(const std::string& s) {
    return s.length() >= 2 && s[s.length()-2] == ' ' && s[s.length()-1] == '-';
}

// 计算字符串右侧的空白长度
Pos rightTrimLength(const std::string& s) {
    Pos i = s.length();
    while (i > 0 && isSpace(s[i-1])) {
        i--;
    }
    return s.length() - i;
}

Pos leftTrimLength(const std::string& s) {
    Pos i = 0;
    while (i < s.length() && isSpace(s[i])) {
        i++;
    }
    return i;
}

// Lexer 构造函数
Lexer::Lexer(const std::string& name, const std::string& input, 
             const std::string& left, const std::string& right)
    : name_(name), input_(input), 
      leftDelim_(left.empty() ? "{{" : left), 
      rightDelim_(right.empty() ? "}}" : right),
      pos_(0), start_(0), atEOF_(false), parenDepth_(0), line_(1), startLine_(1),
      insideAction_(false), lastPos_(0), lastType_(ItemEOF), keyMap_(buildKeywordMap()) {
}

// 设置词法分析器选项
void Lexer::setOptions(const LexOptions& options) {
    options_ = options;
}

// 读取下一个字符
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

// 查看下一个字符但不移动位置
int Lexer::peek() const {
    if (pos_ >= input_.length()) {
        return EOF_RUNE;
    }
    return input_[pos_];
}

// 回退一个字符
void Lexer::backup() {
    if (!atEOF_ && pos_ > 0) {
        pos_--;
        if (pos_ < input_.length() && input_[pos_] == '\n') {
            line_--;
        }
    }
}

// 创建错误 Item
Item Lexer::errorItem(const std::string& format, ...) {
    va_list args;
    va_start(args, format);
    char buf[1024];
    vsnprintf(buf, sizeof(buf), format.c_str(), args);
    va_end(args);
    
    return Item(ItemError, pos_, buf, line_);
}

// 忽略当前累积的 token
void Lexer::ignore() {
    start_ = pos_;
    startLine_ = line_;
}

// 如果下一个字符在有效集合中，则接受它
bool Lexer::accept(const std::string& valid) {
    if (pos_ >= input_.length()) {
        return false;
    }
    if (valid.find(input_[pos_]) != std::string::npos) {
        pos_++;
        return true;
    }
    return false;
}

// 接受有效集合中的连续字符
void Lexer::acceptRun(const std::string& valid) {
    while (pos_ < input_.length() && valid.find(input_[pos_]) != std::string::npos) {
        pos_++;
    }
}

// 检查当前位置是否是右定界符，并检查是否有修剪标记
std::pair<bool, bool> Lexer::atRightDelim() {
    if (pos_ + rightDelim_.length() > input_.length()) {
        return std::make_pair(false, false);
    }
    
    std::string rightPart = input_.substr(pos_, rightDelim_.length());
    bool isDelim = (rightPart == rightDelim_);
    
    // 检查是否是修剪标记
    bool hasTrimMarker = false;
    if (pos_ >= TRIM_MARKER_LEN && 
        hasRightTrimMarker(input_.substr(pos_ - TRIM_MARKER_LEN, TRIM_MARKER_LEN))) {
        hasTrimMarker = true;
    }
    
    return std::make_pair(isDelim, hasTrimMarker);
}

// 检查当前位置是否是动作命令的的终止符
bool Lexer::atTerminator() {
    if (pos_ >= input_.length()) {
        return true;
    }
    int r = input_[pos_];
    return r == ' ' || r == '\t' || r == '\n' || r == '\r' || r == ';' || r == ',';
}

// 扫描数字
bool Lexer::scanNumber() {
    // 允许可选的正负号
    accept("+-");
    // 假设是十进制数
    std::string digits = "0123456789_";
    if (accept("0")) {
        // 处理不同进制前缀
        if (accept("xX")) {
            // 十六进制
            digits = "0123456789abcdefABCDEF_";
        } else if (accept("oO")) {
            // 八进制 (C++14 标准，但 GCC 可能作为扩展支持)
            digits = "01234567_";
        } else if (accept("bB")) {
            // 二进制 (C++14 标准，但 GCC 可能作为扩展支持)
            digits = "01_";
        }
    }
    acceptRun(digits); // 扫描数字部分
    if (accept(".")) { // 小数点
        acceptRun(digits);
    }
    if (digits.length() == 10+1 && accept("eE")) { // 指数 (十进制)
        accept("+-");
        acceptRun("0123456789_");
    }
    if (digits.length() == 16+6+1 && accept("pP")) { // 指数 (十六进制 C99/C++17)
        accept("+-");
        acceptRun("0123456789_");
    }
    // 可能是虚数？ (Go 语言特性)
    accept("i");
    // 检查后面是否紧跟字母数字（无效数字格式）
    if (isAlphaNumeric(peek())) {
        next();
        return false;
    }
    return true;
}

// 获取下一个词法项（状态机入口）
Item Lexer::nextItem() {
    if (pos_ >= input_.length()) {
        if (atEOF_) {
            return Item(ItemEOF, pos_, "EOF", line_);
        }
        atEOF_ = true;
        return Item(ItemEOF, pos_, "EOF", line_);
    }

    if (insideAction_) {
        return lexInsideAction();
    }
    return lexText();
}

// 解析普通文本状态
Item Lexer::lexText() {
    // 查找第一个定界符
    size_t x = input_.find(leftDelim_, pos_);
    if (x != std::string::npos) { // 找到了
        if (x > pos_) { // 定界符不在当前位置，说明前面有文本
            // 逐个读取字符直到定界符 x，以便更新行号
            while (pos_ < x) {
                next();
            }
            // 现在 pos_ 等于 x，且 line_ 已经更新
            Item result(ItemText, start_, input_.substr(start_, pos_ - start_), startLine_);
            ignore(); // 忽略 start_ 之前的部分，准备处理定界符
            return result;
        }
        // 直接处理定界符
        ignore(); // 确保start_是正确的
        return lexLeftDelim();
    }
    
    // 没有找到定界符，处理剩余的部分
    size_t endPos = input_.length();
    if (endPos > pos_) { // 检查是否还有剩余文本
        // 逐个读取剩余所有字符，以便更新行号
        while (pos_ < endPos) {
            next();
        }
        // 处理剩余的部分
        Item result(ItemText, start_, input_.substr(start_, pos_ - start_), startLine_);
        // 注意：这里不需要调用 ignore()，因为后面没有 token 了
        return result;
    }

    // 文件结束
    atEOF_ = true;
    // 文件结束时，使用当前的 line_
    return Item(ItemEOF, pos_, "EOF", line_);
}

// 解析左定界符状态
Item Lexer::lexLeftDelim() {
    // 记录定界符的起始位置
    Pos delimStart = pos_;
    pos_ += leftDelim_.length();
    
    // 处理空格
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
    
    // 处理定界符
    Item result(ItemLeftDelim, delimStart, leftDelim_, startLine_);
    
    // 设置状态
    insideAction_ = true;
    pos_ += afterMarker;
    parenDepth_ = 0;
    ignore(); // 忽略start_位置
    
    return result;
}

// 解析注释状态
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
    Item result(ItemComment, start_, input_.substr(start_, pos_ - start_), startLine_);
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
    
    // 检查后面是否紧跟字母数字（无效数字格式）
    return nextItem();
}

// 解析右定界符状态
Item Lexer::lexRightDelim() {
    // 记录起始位置
    Pos delimStart = pos_;
    
    // 检查前面是否有空格
    bool trimSpace = false;
    if (pos_ >= TRIM_MARKER_LEN && 
        hasRightTrimMarker(input_.substr(pos_ - TRIM_MARKER_LEN, TRIM_MARKER_LEN))) {
        trimSpace = true;
        pos_ = pos_ - TRIM_MARKER_LEN;
    }
    
    // 处理定界符
    Item result(ItemRightDelim, delimStart, rightDelim_, startLine_);
    
    // 移动到定界符之后
    pos_ += rightDelim_.length();
    
    // 处理后面是否有空格
    if (trimSpace && pos_ < input_.length()) {
        pos_ += leftTrimLength(input_.substr(pos_));
    }
    
    // 设置状态
    insideAction_ = false;
    ignore();
    
    return result;
}

// 解析 Action 内部状态
Item Lexer::lexInsideAction() {
    // 处理前面的空格
    if (peek() == ' ' || peek() == '\t' || peek() == '\r' || peek() == '\n') {
        return lexSpace();
    }
    ignore(); // 忽略start_位置
    
    // 检查是否是EOF
    if (peek() == EOF_RUNE) {
        return errorItem("unclosed action");
    }
    
    // 检查是否是定界符
    auto [isDelim, hasTrimMarker] = atRightDelim();
    if (isDelim) {
        if (parenDepth_ == 0) {
            return lexRightDelim();
        }
        return errorItem("unclosed left paren");
    }
    
    // 根据第一个字符确定状态
    int r = next();
    switch (r) {
        case ' ': case '\t': case '\r': case '\n':
            // 处理空格
            backup();
            return lexSpace();
            
        case '=':
            // 处理赋值
            return Item(ItemAssign, start_, "=", startLine_);
            
        case ':':
            // 处理声明
            if (next() != '=') {
                return errorItem("expected :=");
            }
            return Item(ItemDeclare, start_, ":=", startLine_);
            
        case '|':
            // 处理管道
            return Item(ItemPipe, start_, "|", startLine_);
            
        case '"':
            // 处理字符串
            return lexQuote();
            
        case '`':
            // 处理原始字符串
            return lexRawQuote();
            
        case '\'':
            // 处理字符
            return lexChar();
            
        case '$':
            // 处理变量
            return lexVariable();
            
        case '(':
            // 处理左括号
            parenDepth_++;
            return Item(ItemLeftParen, start_, "(", startLine_);
            
        case ')':
            // 处理右括号
            parenDepth_--;
            if (parenDepth_ < 0) {
                return errorItem("unexpected right paren");
            }
            return Item(ItemRightParen, start_, ")", startLine_);
            
        case '.':
            // 检查是否是数字
            if (pos_ < input_.length() && isdigit(input_[pos_])) {
                backup();
                return lexNumber();
            }
            
            // （无效数字格式）
            if (pos_ < input_.length() && isAlphaNumeric(input_[pos_])) {
                return lexField();
            }
            
            return Item(ItemDot, start_, ".", startLine_);
            
        case '+': case '-': case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            // 处理数字
            backup();
            return lexNumber();
            
        default:
            // 识别未知字符或关键字
            if (isalpha(r) || r == '_') {
                // 识别关键字
                backup();
                return lexIdentifier();
            } else if (r <= 127 && isprint(r)) {
                // 处理可打印字符
                return Item(ItemChar, start_, std::string(1, r), startLine_);
            }
            
            // 识别未知字符
            return errorItem("unrecognized character in action: %c", r);
    }
}

// 解析空格状态
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
    
    
    if (pos_ > 0 && hasRightTrimMarker(input_.substr(pos_-1)) && 
        pos_-1+TRIM_MARKER_LEN+rightDelim_.length() <= input_.length() && 
        input_.substr(pos_-1+TRIM_MARKER_LEN, rightDelim_.length()) == rightDelim_) {
        backup(); 
        if (numSpaces == 1) {
            return lexRightDelim(); 
        }
    }
    
    Item result(ItemSpace, start_, input_.substr(start_, pos_ - start_), startLine_);
    // std::cout << "[LEX] 生成空格token: [" << result.val << "] 行:" << result.line << " pos:" << result.pos << std::endl;
    return result;
}

// 解析标识符状态
Item Lexer::lexIdentifier() {
    // 获取识别的单词
    while (isAlphaNumeric(peek())) {
        next();
    }
    
    std::string word = input_.substr(start_, pos_ - start_);
    
    // 检查是否是关键字
    auto it = keyMap_.find(word);
    if (it != keyMap_.end()) {
        // 是关键字
        ItemType keyword = it->second;
        
        // 处理break和continue关键字
        if ((keyword == ItemBreak && !options_.breakOK) || 
            (keyword == ItemContinue && !options_.continueOK)) {
            return Item(ItemIdentifier, start_, word, startLine_);
        }
        
        // 返回关键字
        return Item(keyword, start_, word, startLine_);
    }
    
    // 检查是否是布尔值
    if (word == "true" || word == "false") {
        return Item(ItemBool, start_, word, startLine_);
    }
    
    // 返回普通标识符
    return Item(ItemIdentifier, start_, word, startLine_);
}

Item Lexer::lexField() {
    
    std::string fieldName = ".";
    
    
    while (pos_ < input_.length() && isAlphaNumeric(peek())) {
        fieldName += next();
    }
    
    
    std::cout << "解析字段: " << fieldName << std::endl;
    
    return Item(ItemField, start_, fieldName, startLine_);
}

// 解析变量状态 (以 '$' 开头)
Item Lexer::lexVariable() {
    std::string varName = "$";
    
    // 检查是否只包含 $
    if (pos_ >= input_.length() || !isAlphaNumeric(peek())) {
        return Item(ItemVariable, start_, "$", startLine_);
    }
    
    // 获取变量名
    while (pos_ < input_.length() && isAlphaNumeric(peek())) {
        varName += next();
    }
    
    std::cout << "解析变量: " << varName << std::endl;
    return Item(ItemVariable, start_, varName, startLine_);
}

// 解析单个字符状态 (Action 内部)
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
    return Item(ItemCharConstant, start_, input_.substr(start_, pos_ - start_), startLine_);
}

// 解析数字状态 (Action 内部)
Item Lexer::lexNumber() {
    if (!scanNumber()) {
        return errorItem("bad number syntax: %s", input_.substr(start_, pos_ - start_).c_str());
    }
    
    int sign = peek();
    if (sign == '+' || sign == '-') {
        // : 1+2i
        next();
        if (!scanNumber() || input_[pos_-1] != 'i') {
            return errorItem("bad number syntax: %s", input_.substr(start_, pos_ - start_).c_str());
        }
        return Item(ItemComplex, start_, input_.substr(start_, pos_ - start_), startLine_);
    }
    
    return Item(ItemNumber, start_, input_.substr(start_, pos_ - start_), startLine_);
}

// 解析带引号字符串状态
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
    return Item(ItemString, start_, input_.substr(start_, pos_ - start_), startLine_);
}

// 解析反引号原始字符串状态
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
    return Item(ItemRawString, start_, input_.substr(start_, pos_ - start_), startLine_);
}

// 工厂函数：创建 Lexer 实例
Lexer* createLexer(const std::string& name, const std::string& input, 
                        const std::string& left, const std::string& right) {
    return new Lexer(name, input, left, right);
}

// 将 ItemType 枚举转换为字符串
std::string itemTypeToString(ItemType type) {
    switch (type) {
        case ItemError: return "error";
        case ItemBool: return "bool";
        case ItemChar: return "char";
        case ItemCharConstant: return "charConstant";
        case ItemComment: return "comment";
        case ItemComplex: return "complex";
        case ItemAssign: return "assign";
        case ItemDeclare: return "declare";
        case ItemEOF: return "EOF";
        case ItemField: return "field";
        case ItemIdentifier: return "identifier";
        case ItemLeftDelim: return "leftDelim";
        case ItemLeftParen: return "leftParen";
        case ItemNumber: return "number";
        case ItemPipe: return "pipe";
        case ItemRawString: return "rawString";
        case ItemRightDelim: return "rightDelim";
        case ItemRightParen: return "rightParen";
        case ItemSpace: return "space";
        case ItemString: return "string";
        case ItemText: return "text";
        case ItemVariable: return "variable";
        case ItemKeyword: return "keyword";
        case ItemBlock: return "block";
        case ItemBreak: return "break";
        case ItemContinue: return "continue";
        case ItemDot: return "dot";
        case ItemDefine: return "define";
        case ItemElse: return "else";
        case ItemEnd: return "end";
        case ItemIf: return "if";
        case ItemNil: return "nil";
        case ItemRange: return "range";
        case ItemTemplate: return "template";
        case ItemWith: return "with";
        default: return "unknown";
    }
}

// // ????
// void printItem(const Item& item) {
//     std::cout << "[" << itemTypeToString(item.type) << "] "
//              << "'" << item.val << "' "
//              << "(Line:" << item.line 
//              << ", Pos:" << item.pos << ")\n";
// }

// // ?
// void printTokensWithFormat(const std::vector<Item>& items) {
//     std::cout << "?????????????????????????????????????��????????????????????????????????????????????????????????????��??????????????��?????????????????\n";
//     std::cout << "?? ????            ?? ?                           ?? ?��?  ?? ��??   ??\n";
//     std::cout << "????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????\n";
    
//     for (const auto& item : items) {
//         std::string value = item.val;
//         // ?�I???��????????
//         std::string displayValue = "";
//         for (char c : value) {
//             if (c == '\n') displayValue += "\\n";
//             else if (c == '\r') displayValue += "\\r";
//             else if (c == '\t') displayValue += "\\t";
//             else displayValue += c;
//         }
        
//         // ?????????
//         if (displayValue.length() > 30) {
//             displayValue = displayValue.substr(0, 27) + "...";
//         }
        
//         std::cout << "?? " 
//                   << std::left << std::setw(15) << itemTypeToString(item.type) << " ?? "
//                   << std::left << std::setw(30) << displayValue << " ?? "
//                   << std::setw(5) << item.line << " ?? "
//                   << std::setw(6) << item.pos << " ??\n";
//     }
    
//     std::cout << "???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????\n";
// }

// int main() {
//     // ?????????????
//     std::string input = "Hello {{ .Name }}! {{- if gt .Value 42 -}}\nYour value is greater.\n{{- end -}}";
    
//     // ???????????????????????
//     auto lexer = createLexer("test", input);
    
//     // ??????? - ???????????
//     LexOptions options;
//     options.emitComment = true;
//     options.breakOK = true;
//     options.continueOK = true;
//     lexer->setOptions(options);
    
//     // ??????��???????
//     std::vector<Item> items = lexer->getAllItems();
    
//     // ????????
//     std::cout << "???????: " << input << "\n\n";
//     std::cout << "?????? " << items.size() << " ????????:\n";
//     printTokensWithFormat(items);
    
//     // ???????????????????????????????????
//     std::string input2 = "{{ /* This is a comment */ }}\n{{ range .Items }}\n  - {{ .Name }}: {{ .Value }}{{- if .Last }} (last one!){{ end }}\n{{ end }}";
    
//     auto lexer2 = createLexer("test2", input2);
//     lexer2->setOptions(options);
    
//     std::vector<Item> items2 = lexer2->getAllItems();
    
//     std::cout << "\n\n???????: " << input2 << "\n\n";
//     std::cout << "?????? " << items2.size() << " ????????:\n";
//     printTokensWithFormat(items2);
    
//     return 0;
// }