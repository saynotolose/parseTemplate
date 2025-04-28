#include "lexer.h"
#include <iostream>
#include <sstream>
#include <cstdarg>
#include <algorithm>
#include <iostream>
#include <iomanip>

// ��������
const int EOF_RUNE = -1;
const std::string LEFT_COMMENT = "/*";
const std::string RIGHT_COMMENT = "*/";
const std::string SPACE_CHARS = " \t\r\n";
const char TRIM_MARKER = '-';
const Pos TRIM_MARKER_LEN = 2;  // ��Ǽ�ǰ��ո�

// ʵ��Item��toString����
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

// �����ؼ���ӳ��
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

// ���ߺ���ʵ��

bool isSpace(int r) {
    // ����ո�
    return r == ' ' || r == '\t' || r == '\r' || r == '\n';
}

bool isAlphaNumeric(int r) {
    // �»��ߡ���Сд��ĸ������
    return r == '_' || std::isalpha(r) || std::isdigit(r);
}

bool hasLeftTrimMarker(const std::string& s) {
    return s.length() >= 2 && s[0] == TRIM_MARKER && isSpace(s[1]);
}

bool hasRightTrimMarker(const std::string& s) {
    return s.length() >= 2 && isSpace(s[0]) && s[1] == TRIM_MARKER;
}

// �����������������հ��ַ��ĳ���
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

// �ʷ����������캯��
Lexer::Lexer(const std::string& name, const std::string& input, 
             const std::string& left, const std::string& right)
    : name_(name), input_(input), 
      leftDelim_(left.empty() ? "{{" : left), 
      rightDelim_(right.empty() ? "}}" : right),
      pos_(0), start_(0), atEOF_(false), parenDepth_(0), line_(1), startLine_(1),
      insideAction_(false), lastPos_(0), lastType_(ItemType::ItemError) {
    
    keyMap_ = buildKeywordMap();
}

// ���ôʷ�������ѡ��
void Lexer::setOptions(const LexOptions& options) {
    options_ = options;
}

// ���������е���һ���ַ�
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

// �鿴�������������е���һ���ַ�
int Lexer::peek() const {
    if (pos_ >= input_.length()) {
        return EOF_RUNE;
    }
    return input_[pos_];
}

// ����һ���ַ�
void Lexer::backup() {
    if (!atEOF_ && pos_ > 0) {
        pos_--;
        if (pos_ < input_.length() && input_[pos_] == '\n') {
            line_--;
        }
    }
}

// Ϊ���󴴽�һ��Item
Item Lexer::errorItem(const std::string& format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format.c_str(), args);
    va_end(args);
    
    return Item(ItemType::ItemError, start_, buffer, startLine_);
}

// �����˵�֮ǰ�Ĵ���������
void Lexer::ignore() {
    start_ = pos_;
    startLine_ = line_;
}

// �����һ���ַ�������Ч���ϣ���������
bool Lexer::accept(const std::string& valid) {
    int r = next();
    if (r != EOF_RUNE && valid.find(static_cast<char>(r)) != std::string::npos) {
        return true;
    }
    backup();
    return false;
}

// ������Ч�����е�һ���ַ�
void Lexer::acceptRun(const std::string& valid) {
    while (true) {
        int r = next();
        if (r == EOF_RUNE || valid.find(static_cast<char>(r)) == std::string::npos) {
            break;
        }
    }
    backup();
}

// �����Ƿ����ҷָ�����
std::pair<bool, bool> Lexer::atRightDelim() {
    // �����޼���ǵ��ҷָ���
    if (pos_ > 0 && hasRightTrimMarker(input_.substr(pos_-1)) && 
        pos_-1+TRIM_MARKER_LEN+rightDelim_.length() <= input_.length() && 
        input_.substr(pos_-1+TRIM_MARKER_LEN, rightDelim_.length()) == rightDelim_) {
        return {true, true};
    }
    // ��鲻���޼���ǵ��ҷָ���
    if (pos_+rightDelim_.length() <= input_.length() && 
        input_.substr(pos_, rightDelim_.length()) == rightDelim_) {
        return {true, false};
    }
    return {false, false};
}

// �����Ƿ�����ֹ����
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

// ɨ������
bool Lexer::scanNumber() {
    // ��ѡ��ǰ������
    accept("+-");
    // ��ʮ��������
    std::string digits = "0123456789_";
    if (accept("0")) {
        // ע�⣺�������е�ǰ��0����ʾ�˽���
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
    // ��������
    accept("i");
    // ��һ���ַ���������ĸ����
    if (isAlphaNumeric(peek())) {
        next();
        return false;
    }
    return true;
}

// ��ȡ��һ���ʷ���Ԫ
Item Lexer::nextItem() {
    // ����Ѿ������ļ�����������EOF
    if (atEOF_) {
        return Item(ItemType::ItemEOF, pos_, "EOF", line_);
    }

    // ���ÿ�ʼλ�ú��к�
    start_ = pos_;
    startLine_ = line_;

    // ���ݵ�ǰ״̬������Ӧ�Ĵ�����
    if (insideAction_) {
        return lexInsideAction();
    } else {
        return lexText();
    }
}

// ��ȡ������Ŀ
std::vector<Item> Lexer::getAllItems() {
    std::vector<Item> items;
    Item item;
    do {
        item = nextItem();
        items.push_back(item);
    } while (item.type != ItemType::ItemEOF && item.type != ItemType::ItemError);
    return items;
}

// ������ͨ�ı�
Item Lexer::lexText() {
    size_t x = input_.find(leftDelim_, pos_);
    if (x != std::string::npos) {
        if (x > pos_) {
            // �����ı�Ƭ��
            pos_ = x;
            Item result(ItemType::ItemText, start_, input_.substr(start_, pos_ - start_), startLine_);
            return result;
        }
        // ������ָ���
        return lexLeftDelim();
    }
    
    // �����������
    pos_ = input_.length();
    if (pos_ > start_) {
        // ���������ı�Ƭ��
        Item result(ItemType::ItemText, start_, input_.substr(start_, pos_ - start_), startLine_);
        return result;
    }
    // ������EOF
    atEOF_ = true;
    return Item(ItemType::ItemEOF, pos_, "EOF", line_);
}

// ������ָ���
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
    
    // ����Ƿ���ע��
    if (pos_ + afterMarker + LEFT_COMMENT.length() <= input_.length() &&
        input_.substr(pos_ + afterMarker, LEFT_COMMENT.length()) == LEFT_COMMENT) {
        pos_ += afterMarker;
        ignore();
        return lexComment();
    }
    
    // ����ע�ͣ�������ָ�����
    Item result(ItemType::ItemLeftDelim, start_, input_.substr(start_, pos_ - start_), startLine_);
    insideAction_ = true;
    pos_ += afterMarker;
    parenDepth_ = 0;
    ignore();
    return result;
}

// ����ע��
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
    
    // ���������ע�ͣ��ͼ���������һ����
    return nextItem();
}

// �����ҷָ���
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

// ��������ڲ�������
Item Lexer::lexInsideAction() {
    // ����Ƿ����ҷָ�����
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
            backup(); // �Żؿո�
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

// ����ո�
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
    
    // ����޼���ǵ��ҷָ���
    if (pos_ > 0 && hasRightTrimMarker(input_.substr(pos_-1)) && 
        pos_-1+TRIM_MARKER_LEN+rightDelim_.length() <= input_.length() && 
        input_.substr(pos_-1+TRIM_MARKER_LEN, rightDelim_.length()) == rightDelim_) {
        backup(); // �ڿո�֮ǰ
        if (numSpaces == 1) {
            return lexRightDelim(); // ֱ�Ӵ���ָ���
        }
    }
    
    Item result(ItemType::ItemSpace, start_, input_.substr(start_, pos_ - start_), startLine_);
    return result;
}

// �����ʶ��
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

// �����ֶ�
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

// �������
Item Lexer::lexVariable() {
    if (atTerminator()) { // ������"$"
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

// �����ַ�����
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

// ��������
Item Lexer::lexNumber() {
    if (!scanNumber()) {
        return errorItem("bad number syntax: %s", input_.substr(start_, pos_ - start_).c_str());
    }
    
    int sign = peek();
    if (sign == '+' || sign == '-') {
        // ����: 1+2i
        next();
        if (!scanNumber() || input_[pos_-1] != 'i') {
            return errorItem("bad number syntax: %s", input_.substr(start_, pos_ - start_).c_str());
        }
        return Item(ItemType::ItemComplex, start_, input_.substr(start_, pos_ - start_), startLine_);
    }
    
    return Item(ItemType::ItemNumber, start_, input_.substr(start_, pos_ - start_), startLine_);
}

// ���������ַ���
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

// ����ԭʼ�����ַ���
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

// �����ʷ��������Ĺ�������
std::shared_ptr<Lexer> createLexer(const std::string& name, const std::string& input, 
                                const std::string& left, const std::string& right) {
    return std::make_shared<Lexer>(name, input, left, right);
}

// ���ʷ�������ת��Ϊ�ַ����ĸ�������
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




// // ���ڴ�ӡ�ʷ���ĸ�������
// void printItem(const Item& item) {
//     std::cout << "[" << itemTypeToString(item.type) << "] "
//              << "'" << item.val << "' "
//              << "(Line:" << item.line 
//              << ", Pos:" << item.pos << ")\n";
// }

// // ��ӡ����б�
// void printTokensWithFormat(const std::vector<Item>& items) {
//     std::cout << "�������������������������������������Щ������������������������������������������������������������Щ��������������Щ�����������������\n";
//     std::cout << "�� ����            �� ֵ                           �� �к�  �� λ��   ��\n";
//     std::cout << "�������������������������������������੤�����������������������������������������������������������੤�������������੤����������������\n";
    
//     for (const auto& item : items) {
//         std::string value = item.val;
//         // �滻���з�Ϊ�ɼ���ʾ
//         std::string displayValue = "";
//         for (char c : value) {
//             if (c == '\n') displayValue += "\\n";
//             else if (c == '\r') displayValue += "\\r";
//             else if (c == '\t') displayValue += "\\t";
//             else displayValue += c;
//         }
        
//         // �ضϹ�����ֵ
//         if (displayValue.length() > 30) {
//             displayValue = displayValue.substr(0, 27) + "...";
//         }
        
//         std::cout << "�� " 
//                   << std::left << std::setw(15) << itemTypeToString(item.type) << " �� "
//                   << std::left << std::setw(30) << displayValue << " �� "
//                   << std::setw(5) << item.line << " �� "
//                   << std::setw(6) << item.pos << " ��\n";
//     }
    
//     std::cout << "�������������������������������������ة������������������������������������������������������������ة��������������ة�����������������\n";
// }

// int main() {
//     // ����һ���򵥵�ģ��
//     std::string input = "Hello {{ .Name }}! {{- if gt .Value 42 -}}\nYour value is greater.\n{{- end -}}";
    
//     // ʹ��Ĭ�Ϸָ��������ʷ�������
//     auto lexer = createLexer("test", input);
    
//     // ����ѡ�� - ���ñ�Ҫ�Ĺ���
//     LexOptions options;
//     options.emitComment = true;
//     options.breakOK = true;
//     options.continueOK = true;
//     lexer->setOptions(options);
    
//     // ��ȡ���дʷ����
//     std::vector<Item> items = lexer->getAllItems();
    
//     // ��ӡ���
//     std::cout << "����ģ��: " << input << "\n\n";
//     std::cout << "������ " << items.size() << " ���ʷ���Ԫ:\n";
//     printTokensWithFormat(items);
    
//     // ����һ�������ӵ����ӣ������޼���Ǻ�ע��
//     std::string input2 = "{{ /* This is a comment */ }}\n{{ range .Items }}\n  - {{ .Name }}: {{ .Value }}{{- if .Last }} (last one!){{ end }}\n{{ end }}";
    
//     auto lexer2 = createLexer("test2", input2);
//     lexer2->setOptions(options);
    
//     std::vector<Item> items2 = lexer2->getAllItems();
    
//     std::cout << "\n\n����ģ��: " << input2 << "\n\n";
//     std::cout << "������ " << items2.size() << " ���ʷ���Ԫ:\n";
//     printTokensWithFormat(items2);
    
//     return 0;
// }