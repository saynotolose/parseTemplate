// values.cpp
#include "values.h"
#include "exec.h"  // 添加包含TemplateFn的头文件
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <fstream>

namespace template_engine {

// 构造函数实现
Values::Values() : type_(Null), functionValue_(NULL) {}

Values::Values(bool b) : type_(Bool), boolValue_(b), functionValue_(NULL) {}

Values::Values(double n) : type_(Number), numberValue_(n), functionValue_(NULL) {}

Values::Values(const std::string& s) : type_(String), stringValue_(s), functionValue_(NULL) {}

Values::Values(const std::vector<Values*>& l) : type_(List), functionValue_(NULL) {
    listValue_.reserve(l.size());
    for (std::vector<Values*>::const_iterator it = l.begin(); it != l.end(); ++it) {
        if (*it) {
            listValue_.push_back(new Values(*(*it))); // 手动调用拷贝构造
        } else {
            listValue_.push_back(NULL);
        }
    }
}

Values::Values(const std::map<std::string, Values*>& m) : type_(Map), functionValue_(NULL) {
    for (std::map<std::string, Values*>::const_iterator it = m.begin(); it != m.end(); ++it) {
        if (it->second) {
            mapValue_[it->first] = new Values(*(it->second)); // 手动调用拷贝构造
        } else {
            mapValue_[it->first] = NULL;
        }
    }
}

// 添加函数构造函数
Values::Values(TemplateFn* fn) : type_(Function), functionValue_(fn) {}

// 析构函数
Values::~Values() {
    clearResources();
}

// 恢复：拷贝构造函数执行手动深拷贝
Values::Values(const Values& other) : type_(other.type_), 
    boolValue_(other.boolValue_), 
    numberValue_(other.numberValue_), 
    stringValue_(other.stringValue_),
    functionValue_(other.functionValue_) {
    // 手动深拷贝列表
    if (other.type_ == List) {
        listValue_.reserve(other.listValue_.size());
        for (std::vector<Values*>::const_iterator it = other.listValue_.begin(); it != other.listValue_.end(); ++it) {
            if (*it) {
                listValue_.push_back(new Values(*(*it)));
            } else {
                listValue_.push_back(NULL);
            }
        }
    }
    // 手动深拷贝映射
    else if (other.type_ == Map) {
        for (std::map<std::string, Values*>::const_iterator it = other.mapValue_.begin(); it != other.mapValue_.end(); ++it) {
            if (it->second) {
                mapValue_[it->first] = new Values(*(it->second));
            } else {
                mapValue_[it->first] = NULL;
            }
        }
    }
}

// 恢复：赋值运算符使用 copy-and-swap (依赖正确的拷贝构造和析构)
Values& Values::operator=(const Values& other) {
    if (this != &other) {
        Values temp(other); // 调用拷贝构造函数进行深拷贝
        std::swap(type_, temp.type_);
        std::swap(boolValue_, temp.boolValue_);
        std::swap(numberValue_, temp.numberValue_);
        std::swap(stringValue_, temp.stringValue_);
        std::swap(listValue_, temp.listValue_);
        std::swap(mapValue_, temp.mapValue_);
        std::swap(functionValue_, temp.functionValue_);
    }
    return *this;
}

// 清理资源
void Values::clearResources() {
    // 释放列表中的所有元素
    if (type_ == List) {
        for (std::vector<Values*>::iterator it = listValue_.begin(); it != listValue_.end(); ++it) {
            delete *it;
        }
        listValue_.clear();
    }
    
    // 释放映射中的所有元素
    if (type_ == Map) {
        for (std::map<std::string, Values*>::iterator it = mapValue_.begin(); it != mapValue_.end(); ++it) {
            delete it->second;
        }
        mapValue_.clear();
    }
    
    // 函数值不需要释放，它由外部管理
}

// 工厂方法实现
Values* Values::MakeNull() {
    return new Values();
}

Values* Values::MakeBool(bool b) {
    return new Values(b);
}

Values* Values::MakeNumber(double n) {
    return new Values(n);
}

Values* Values::MakeString(const std::string& s) {
    return new Values(s);
}

Values* Values::MakeList(const std::vector<Values*>& l) {
    return new Values(l);
}

Values* Values::MakeMap(const std::map<std::string, Values*>& m) {
    return new Values(m);
}

// 添加函数工厂方法
Values* Values::MakeFunction(TemplateFn* fn) {
    return new Values(fn);
}

// 类型检查实现
bool Values::IsNull() const { return type_ == Null; }
bool Values::IsBool() const { return type_ == Bool; }
bool Values::IsNumber() const { return type_ == Number; }
bool Values::IsString() const { return type_ == String; }
bool Values::IsList() const { return type_ == List; }
bool Values::IsMap() const { return type_ == Map; }
bool Values::IsFunction() const { return type_ == Function; }

// 值访问实现
bool Values::AsBool() const {
    if (!IsBool()) throw ValueError(TypeError, "not a bool");
    return boolValue_;
}

double Values::AsNumber() const {
    if (!IsNumber()) throw ValueError(TypeError, "not a number");
    return numberValue_;
}

const std::string& Values::AsString() const {
    if (!IsString()) throw ValueError(TypeError, "not a string");
    return stringValue_;
}

const std::vector<Values*>& Values::AsList() const {
    if (!IsList()) throw ValueError(TypeError, "not a list");
    return listValue_;
}

// 常量版本的AsMap()实现
const std::map<std::string, Values*>& Values::AsMap() const {
    if (!IsMap()) throw ValueError(TypeError, "not a map");
    return mapValue_;
}

// 添加非常量版本的AsMap()实现
std::map<std::string, Values*>& Values::AsMap() {
    if (!IsMap()) throw ValueError(TypeError, "not a map");
    return mapValue_;
}

// 添加函数访问方法
TemplateFn* Values::AsFunction() const {
    if (!IsFunction()) throw ValueError(TypeError, "not a function");
    return functionValue_;
}

// 分割路径为段
std::vector<std::string> Values::SplitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::string::size_type start = 0;
    std::string::size_type end = path.find('.');
    
    while (end != std::string::npos) {
        parts.push_back(path.substr(start, end - start));
        start = end + 1;
        end = path.find('.', start);
    }
    parts.push_back(path.substr(start));
    return parts;
}

// 使用路径访问值
Values* Values::Table(const std::string& path) const {
    std::vector<std::string> parts = SplitPath(path);
    const Values* current = this;
    
    for (std::vector<std::string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
        if (!current->IsMap()) {
            throw ValueError(NoTable, "not a table: " + *it);
        }
        
        const std::map<std::string, Values*>& map = current->AsMap();
        std::map<std::string, Values*>::const_iterator mapIt = map.find(*it);
        if (mapIt == map.end() || mapIt->second == NULL) {
            throw ValueError(NoValue, "no value for key: " + *it);
        }
        
        current = mapIt->second;
    }
    
    return new Values(*current);
}



// 判断是否包含特定键
bool Values::Contains(const std::string& key) const {
    if (!IsMap()) {
        return false;
    }
    const std::map<std::string, Values*>& map = AsMap();
    return map.find(key) != map.end() && map.find(key)->second != NULL;
}

// 类似数组访问
Values*& Values::operator[](const std::string& key) {
    if (!IsMap()) {
        throw ValueError(TypeError, "not a map");
    }
    return mapValue_[key];
}

Values*& Values::operator[](size_t index) {
    if (!IsList()) {
        throw ValueError(TypeError, "not a list");
    }
    if (index >= listValue_.size()) {
        throw ValueError(NoValue, "index out of range");
    }
    return listValue_[index];
}

// 调试输出
void Values::Print(std::ostream& os, int indent) const {
    std::string spaces(indent, ' ');
    
    if (IsNull()) {
        os << spaces << "null" << std::endl;
    } else if (IsBool()) {
        os << spaces << (AsBool() ? "true" : "false") << std::endl;
    } else if (IsNumber()) {
        os << spaces << AsNumber() << std::endl;
    } else if (IsString()) {
        os << spaces << "\"" << AsString() << "\"" << std::endl;
    } else if (IsList()) {
        os << spaces << "[" << std::endl;
        const std::vector<Values*>& list = AsList();
        for (std::vector<Values*>::const_iterator it = list.begin(); it != list.end(); ++it) {
            if (*it) {
                (*it)->Print(os, indent + 2);
            } else {
                os << spaces << "  null" << std::endl;
            }
        }
        os << spaces << "]" << std::endl;
    } else if (IsMap()) {
        os << spaces << "{" << std::endl;
        const std::map<std::string, Values*>& map = AsMap();
        for (std::map<std::string, Values*>::const_iterator it = map.begin(); it != map.end(); ++it) {
            os << spaces << "  " << it->first << ": ";
            if (it->second) {
                it->second->Print(os, indent + 2);
            } else {
                os << "null" << std::endl;
            }
        }
        os << spaces << "}" << std::endl;
    }
}

// 序列化为字符串
std::string Values::ToString() const {
    std::ostringstream oss;
    if (IsNull()) {
        oss << "null";
    } else if (IsBool()) {
        oss << (AsBool() ? "true" : "false");
    } else if (IsNumber()) {
        oss << AsNumber();
    } else if (IsString()) {
        // 直接输出字符串内容，不添加引号
        oss << AsString();
    } else if (IsList()) {
        // 列表的简化表示
        const std::vector<Values*>& list = AsList();
        if (list.empty()) {
            oss << "[]";
        } else {
            oss << "[";
            for (size_t i = 0; i < list.size() && i < 3; ++i) {
                if (i > 0) oss << ", ";
                if (list[i]) {
                    oss << list[i]->ToString();
                } else {
                    oss << "null";
                }
            }
            if (list.size() > 3) {
                oss << ", ...";
            }
            oss << "]";
        }
    } else if (IsMap()) {
        // Map的简化表示
        const std::map<std::string, Values*>& map = AsMap();
        if (map.empty()) {
            oss << "{}";
        } else {
            // 检查这是否是一个简单的键值对象，并尝试输出最有意义的值
            if (map.size() == 1) {
                // 如果只有一个值，直接输出该值
                oss << map.begin()->second->ToString();
            } else {
                // 输出所有键
                oss << "{";
                bool first = true;
                for (std::map<std::string, Values*>::const_iterator it = map.begin(); it != map.end(); ++it) {
                    if (!first) oss << " ";
                    first = false;
                    oss << it->first;
                }
                oss << "}";
            }
        }
    }
    return oss.str();
}


// values.cpp 中的 PathValue 方法
Values* Values::PathValue(const std::string& path) const {
    if (path.empty()) {
        return NULL;
    }
    
    std::cout << "PathValue: 访问路径 " << path << std::endl;
    
    // 分割路径
    std::vector<std::string> parts = SplitPath(path);
    std::cout << "  路径分割为 " << parts.size() << " 部分" << std::endl;
    
    // 遍历路径
    const Values* current = this;
    
    for (size_t i = 0; i < parts.size(); ++i) {
        std::cout << "    查找部分 " << i+1 << ": " << parts[i] << std::endl;
        
        if (!current || !current->IsMap()) {
            std::cout << "    当前节点不是map或为空" << std::endl;
            return NULL;
        }
        
        const std::map<std::string, Values*>& map = current->AsMap();
        std::map<std::string, Values*>::const_iterator it = map.find(parts[i]);
        
        if (it == map.end() || !it->second) {
            std::cout << "    未找到键 " << parts[i] << std::endl;
            return NULL;
        }
        
        current = it->second;
        std::cout << "    找到部分 " << parts[i] << ", 类型: " << current->TypeName() << std::endl;
    }
    
    // 创建结果的副本以避免所有权问题
    if (current) {
        return new Values(*current);
    }
    
    return NULL;
}


// 序列化
std::string Values::ToYAML() const {
    std::ostringstream oss;
    Print(oss, 0);
    return oss.str();
}

bool Values::Encode(std::ostream& out) const {
    try {
        Print(out, 0);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// 深拷贝实现
Values* Values::DeepCopy() const {
    return new Values(*this); // 利用拷贝构造函数
}

// 反序列化
Values* Values::FromYAML(const std::string& yamlContent) {
    // 简化版的YAML解析器
    Values* result = MakeMap(std::map<std::string, Values*>());
    std::istringstream iss(yamlContent);
    std::string line;
    
    while (std::getline(iss, line)) {
        // 去除前后空白
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        if (line.empty() || line[0] == '#') continue;
        
        // 寻找 key: value 模式
        std::string::size_type pos = line.find(':');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // 去除空白
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            if (!key.empty()) {
                if (value.empty()) {
                    // 空映射或空列表
                    result->mapValue_[key] = MakeMap(std::map<std::string, Values*>());
                } else if (value == "true") {
                    result->mapValue_[key] = MakeBool(true);
                } else if (value == "false") {
                    result->mapValue_[key] = MakeBool(false);
                } else if (value == "null") {
                    result->mapValue_[key] = MakeNull();
                } else if (value.find_first_not_of("-0123456789.") == std::string::npos) {
                    // 数字
                    result->mapValue_[key] = MakeNumber(std::atof(value.c_str()));
                } else {
                    // 字符串 - 去掉可能的引号
                    if ((value.size() > 0 && value[0] == '"' && value.size() > 1 && value[value.size()-1] == '"') ||
                        (value.size() > 0 && value[0] == '\'' && value.size() > 1 && value[value.size()-1] == '\'')) {
                        value = value.substr(1, value.length() - 2);
                    }
                    result->mapValue_[key] = MakeString(value);
                }
            }
        }
    }
    
    return result;
}

Values* Values::FromYAMLFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw ValueError(NoTable, "Could not open file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return FromYAML(buffer.str());
}









// 合并值
Values* CoalesceValues(const Values* base, const Values* overlay) {
    if (!base) {
        return overlay ? new Values(*overlay) : NULL;
    }
    if (!overlay) {
        return new Values(*base);
    }
    
    if (!base->IsMap() || !overlay->IsMap()) {
        return new Values(*overlay);
    }
    
    std::map<std::string, Values*> result;
    const std::map<std::string, Values*>& baseMap = base->AsMap();
    const std::map<std::string, Values*>& overlayMap = overlay->AsMap();
    
    // 复制base中的所有值
    for (std::map<std::string, Values*>::const_iterator it = baseMap.begin(); it != baseMap.end(); ++it) {
        if (it->second) {
            result[it->first] = new Values(*(it->second));
        } else {
            result[it->first] = NULL;
        }
    }
    
    // 合并overlay中的值
    for (std::map<std::string, Values*>::const_iterator it = overlayMap.begin(); it != overlayMap.end(); ++it) {
        std::map<std::string, Values*>::iterator baseIt = result.find(it->first);
        if (baseIt != result.end() && baseIt->second && it->second && 
            baseIt->second->IsMap() && it->second->IsMap()) {
            // 递归合并map
            result[it->first] = CoalesceValues(baseIt->second, it->second);
            delete baseIt->second; // 释放旧的值
        } else {
            // 直接覆盖，先删除旧的值
            if (baseIt != result.end() && baseIt->second) {
                delete baseIt->second;
            }
            
            // 复制新的值
            if (it->second) {
                result[it->first] = new Values(*(it->second));
            } else {
                result[it->first] = NULL;
            }
        }
    }
    
    return Values::MakeMap(result);
}










// 准备渲染值
Values* ToRenderValues(
    const std::string& chartName,
    const std::string& chartVersion,
    const Values* chartValues,
    const RenderOptions& options) {
    
    std::map<std::string, Values*> result;
    
    // 添加Chart信息
    std::map<std::string, Values*> chartMap;
    chartMap["Name"] = Values::MakeString(chartName);
    chartMap["Version"] = Values::MakeString(chartVersion);
    result["Chart"] = Values::MakeMap(chartMap);
    
    // 添加Release信息
    std::map<std::string, Values*> releaseMap;
    releaseMap["Name"] = Values::MakeString(options.name);
    releaseMap["Namespace"] = Values::MakeString(options.nameSpace);
    releaseMap["Revision"] = Values::MakeNumber(options.revision);
    releaseMap["IsUpgrade"] = Values::MakeBool(options.isUpgrade);
    releaseMap["IsInstall"] = Values::MakeBool(options.isInstall);
    result["Release"] = Values::MakeMap(releaseMap);
    
    // 添加Values
    if (chartValues) {
        result["Values"] = new Values(*chartValues);
    } else {
        result["Values"] = Values::MakeMap(std::map<std::string, Values*>());
    }
    
    return Values::MakeMap(result);
}

// Implementation for TypeName()
std::string Values::TypeName() const {
    switch (type_) {
        case Null:
            return "null";
        case Bool:
            return "bool";
        case Number:
            return "number";
        case String:
            return "string";
        case Map:
            return "map";
        case List:
            return "list";
        case Function:
            return "function";
        default:
            return "unknown";
    }
}

// ================== 简易YAML解析实现（C++98，无第三方库） ==================
struct SimpleYamlLine {
    int indent; // 缩进空格数
    bool isListItem;
    std::string key;
    std::string value;
};

static std::vector<std::string> SplitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::string::size_type start = 0, end;
    while ((end = text.find('\n', start)) != std::string::npos) {
        lines.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    if (start < text.size()) lines.push_back(text.substr(start));
    return lines;
}

static std::vector<SimpleYamlLine> ParseSimpleYamlLines(const std::vector<std::string>& lines) {
    std::vector<SimpleYamlLine> result;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        if (line.empty()) continue;
        size_t p = 0;
        while (p < line.size() && (line[p] == ' ')) ++p;
        if (p == line.size() || line[p] == '#') continue; // 空行或注释
        int indent = (int)p;
        std::string content = line.substr(p);
        bool isListItem = false;
        std::string key, value;
        if (content.size() >= 2 && content[0] == '-' && content[1] == ' ') {
            isListItem = true;
            value = content.substr(2);
        } else {
            size_t pos = content.find(':');
            if (pos != std::string::npos) {
                key = content.substr(0, pos);
                value = content.substr(pos + 1);
                if (!value.empty() && value[0] == ' ') value = value.substr(1);
            }
        }
        
        // --- 新增：去除 value 末尾的注释 --- 
        size_t commentPos = value.find('#');
        if (commentPos != std::string::npos) {
            // 检查 # 是否在引号内（非常简化的检查，仅处理结尾引号）
            bool inQuotes = (!value.empty() && value[0] == '"' && value[value.size()-1] == '"') ||
                            (!value.empty() && value[0] == '\'' && value[value.size()-1] == '\'');
            if (!inQuotes) { // 如果不在引号内（或无法判断引号），则去除注释
                 value = value.substr(0, commentPos);
            }
        }
        // 去除 value 末尾可能存在的空格
        size_t endPos = value.find_last_not_of(" \t");
        if (endPos != std::string::npos) {
            value = value.substr(0, endPos + 1);
        } else if (value.find_first_of(" \t") == 0 && value.length() > 0) {
             // 如果原始值全是空格，则置空
             value = "";
        }
        // --- 结束注释去除 --- 
        
        SimpleYamlLine yl;
        yl.indent = indent;
        yl.isListItem = isListItem;
        yl.key = key;
        yl.value = value;
        result.push_back(yl);
    }
    return result;
}

static Values* ParseSimpleYamlScalar(const std::string& value) {
    if (value == "null" || value == "~") return Values::MakeNull();
    if (value == "true") return Values::MakeBool(true);
    if (value == "false") return Values::MakeBool(false);
    if (!value.empty() && value[0] == '"' && value[value.size()-1] == '"')
        return Values::MakeString(value.substr(1, value.size()-2));
    if (!value.empty() && value[0] == '\'' && value[value.size()-1] == '\'')
        return Values::MakeString(value.substr(1, value.size()-2));
    // 尝试解析为数字
    char* endptr = 0;
    double num = strtod(value.c_str(), &endptr);
    if (endptr != value.c_str() && *endptr == '\0')
        return Values::MakeNumber(num);
    // 默认字符串
    return Values::MakeString(value);
}

static Values* ParseSimpleYamlBlock(const std::vector<SimpleYamlLine>& lines, int& idx, int parentIndent) {
    if (idx >= (int)lines.size()) return Values::MakeNull();

    // 判断是列表还是map
    if (lines[idx].isListItem) {
        // 列表
        std::vector<Values*> list;
        while (idx < (int)lines.size() && lines[idx].isListItem && lines[idx].indent == parentIndent) {
            // 判断本行是标量还是map
            if (!lines[idx].value.empty()) {
                // 进一步判断 value 是否为 key: value 形式（即 - name: ENV）
                size_t pos = lines[idx].value.find(':');
                if (pos != std::string::npos && pos != lines[idx].value.length() - 1) {
                    // 形如 - name: ENV，解析为map
                    std::string key = lines[idx].value.substr(0, pos);
                    std::string value = lines[idx].value.substr(pos + 1);
                    // 去除空白
                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);

                    std::map<std::string, Values*> map;
                    map[key] = ParseSimpleYamlScalar(value);

                    // 检查下方是否还有同级缩进的字段，属于同一个map
                    int itemIndent = lines[idx].indent;
                    int nextIndent = itemIndent + 2;
                    ++idx;
                    while (idx < (int)lines.size() && !lines[idx].isListItem && lines[idx].indent == nextIndent) {
                        std::string subKey = lines[idx].key;
                        std::string subValue = lines[idx].value;
                        if (!subKey.empty()) {
                            if (!subValue.empty()) {
                                map[subKey] = ParseSimpleYamlScalar(subValue);
                                ++idx;
                            } else {
                                ++idx;
                                map[subKey] = ParseSimpleYamlBlock(lines, idx, nextIndent + 2);
                            }
                        } else {
                            ++idx;
                        }
                    }
                    list.push_back(Values::MakeMap(map));
                } else {
                    // 普通标量
                list.push_back(ParseSimpleYamlScalar(lines[idx].value));
                ++idx;
                }
            } else {
                // 该 list item 没有直接 value，递归解析为 map 或 list
                ++idx;
                list.push_back(ParseSimpleYamlBlock(lines, idx, parentIndent + 2));
            }
        }
        return Values::MakeList(list);
    } else {
        // map
        std::map<std::string, Values*> map;
        while (idx < (int)lines.size() && lines[idx].indent == parentIndent && !lines[idx].isListItem) {
            std::string key = lines[idx].key;
            std::string value = lines[idx].value;
            int curIndent = lines[idx].indent;
            if (!value.empty()) {
                // 处理空map/空list
                if (value == "{}") {
                    map[key] = Values::MakeMap(std::map<std::string, Values*>());
                } else if (value == "[]") {
                    map[key] = Values::MakeList(std::vector<Values*>());
                } else if (value == "|" || value == ">") {
                    // 多行字符串
                    std::string multiLine;
                    ++idx;
                    while (idx < (int)lines.size() && lines[idx].indent > curIndent) {
                        multiLine += lines[idx].value;
                        multiLine += "\n";
                        ++idx;
                    }
                    map[key] = Values::MakeString(multiLine);
                    continue;
                } else {
                    map[key] = ParseSimpleYamlScalar(value);
                }
                ++idx;
            } else {
                ++idx;
                map[key] = ParseSimpleYamlBlock(lines, idx, parentIndent + 2);
            }
        }
        return Values::MakeMap(map);
    }
}

Values* ParseSimpleYAML(const std::string& yamlText) {
    std::vector<std::string> lines = SplitLines(yamlText);
    std::vector<SimpleYamlLine> parsedLines = ParseSimpleYamlLines(lines);
    int idx = 0;
    return ParseSimpleYamlBlock(parsedLines, idx, 0);
}

Values* ParseSimpleYAMLFile(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        throw ValueError(NoTable, "Could not open file: " + filename);
    }
    std::stringstream buffer;
    std::string line;
    while (std::getline(file, line)) {
        buffer << line << "\n";
    }
    return ParseSimpleYAML(buffer.str());
}

} // namespace template_engine