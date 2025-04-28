// values.cpp
#include "values.h"
#include <regex>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace template_engine {

// Values构造函数实现
Values::Values(const ValueType& value) : value_(value) {}
Values::Values(ValueType&& value) : value_(std::move(value)) {}

// 创建辅助函数实现
std::shared_ptr<Values> Values::MakeMap() {
    return std::make_shared<Values>(std::map<std::string, std::shared_ptr<Values>>{});
}

std::shared_ptr<Values> Values::MakeList() {
    return std::make_shared<Values>(std::vector<std::shared_ptr<Values>>{});
}

std::shared_ptr<Values> Values::MakeString(const std::string& s) {
    return std::make_shared<Values>(s);
}

std::shared_ptr<Values> Values::MakeNumber(double n) {
    return std::make_shared<Values>(n);
}

std::shared_ptr<Values> Values::MakeBool(bool b) {
    return std::make_shared<Values>(b);
}

std::shared_ptr<Values> Values::MakeNull() {
    return std::make_shared<Values>(nullptr);
}

// 类型检查实现
bool Values::IsMap() const {
    return std::holds_alternative<std::map<std::string, std::shared_ptr<Values>>>(value_);
}

bool Values::IsList() const {
    return std::holds_alternative<std::vector<std::shared_ptr<Values>>>(value_);
}

bool Values::IsString() const {
    return std::holds_alternative<std::string>(value_);
}

bool Values::IsNumber() const {
    return std::holds_alternative<double>(value_) || 
           std::holds_alternative<int64_t>(value_);
}

bool Values::IsBool() const {
    return std::holds_alternative<bool>(value_);
}

bool Values::IsNull() const {
    return std::holds_alternative<std::nullptr_t>(value_);
}

// 访问器实现
std::map<std::string, std::shared_ptr<Values>>& Values::AsMap() {
    if (!IsMap()) {
        throw ValueError(ValueErrorType::TypeError, "Value is not a map");
    }
    return std::get<std::map<std::string, std::shared_ptr<Values>>>(value_);
}

const std::map<std::string, std::shared_ptr<Values>>& Values::AsMap() const {
    if (!IsMap()) {
        throw ValueError(ValueErrorType::TypeError, "Value is not a map");
    }
    return std::get<std::map<std::string, std::shared_ptr<Values>>>(value_);
}

std::vector<std::shared_ptr<Values>>& Values::AsList() {
    if (!IsList()) {
        throw ValueError(ValueErrorType::TypeError, "Value is not a list");
    }
    return std::get<std::vector<std::shared_ptr<Values>>>(value_);
}

const std::vector<std::shared_ptr<Values>>& Values::AsList() const {
    if (!IsList()) {
        throw ValueError(ValueErrorType::TypeError, "Value is not a list");
    }
    return std::get<std::vector<std::shared_ptr<Values>>>(value_);
}

std::string& Values::AsString() {
    if (!IsString()) {
        throw ValueError(ValueErrorType::TypeError, "Value is not a string");
    }
    return std::get<std::string>(value_);
}

const std::string& Values::AsString() const {
    if (!IsString()) {
        throw ValueError(ValueErrorType::TypeError, "Value is not a string");
    }
    return std::get<std::string>(value_);
}

double Values::AsNumber() const {
    if (std::holds_alternative<double>(value_)) {
        return std::get<double>(value_);
    } else if (std::holds_alternative<int64_t>(value_)) {
        return static_cast<double>(std::get<int64_t>(value_));
    }
    throw ValueError(ValueErrorType::TypeError, "Value is not a number");
}

bool Values::AsBool() const {
    if (!IsBool()) {
        throw ValueError(ValueErrorType::TypeError, "Value is not a boolean");
    }
    return std::get<bool>(value_);
}

// 分割路径
std::vector<std::string> Values::SplitPath(const std::string& path) {
    std::vector<std::string> parts;
    
    size_t start = 0;
    size_t pos = 0;
    while ((pos = path.find('.', start)) != std::string::npos) {
        if (pos > start) {
            parts.push_back(path.substr(start, pos - start));
        }
        start = pos + 1;
    }
    
    if (start < path.length()) {
        parts.push_back(path.substr(start));
    }
    
    return parts;
}

// Table实现 - 获取嵌套表
// Table实现 - 获取嵌套表
std::shared_ptr<Values> Values::Table(const std::string& path) const {
    auto parts = SplitPath(path);
    auto current = const_cast<Values*>(this);
    
    for (const auto& part : parts) {
        if (!current->IsMap()) {
            std::cerr << "错误: 尝试在非Map类型的值中查找表: " << part << std::endl;
            throw ValueError(ValueErrorType::NoTable, 
                           "Table lookup in non-map type at: " + part);
        }
        
        auto& table = current->AsMap();
        auto it = table.find(part);
        if (it == table.end()) {
            std::cerr << "错误: 找不到表: " << part << std::endl;
            throw ValueError(ValueErrorType::NoTable, 
                           "Table not found: " + part);
        }
        
        if (!it->second) {
            std::cerr << "错误: 路径上的值为空: " << part << std::endl;
            throw ValueError(ValueErrorType::NoTable, 
                           "Null value in path: " + part);
        }
        
        current = it->second.get();
    }
    
    return std::shared_ptr<Values>(current->DeepCopy());
}

// 按路径获取值
// std::optional<std::shared_ptr<Values>> Values::PathValue(const std::string& path) const {
//     if (path.empty()) {
//         return std::nullopt;
//     }
    
//     auto parts = SplitPath(path);
//     if (parts.empty()) {
//         return std::nullopt;
//     }
    
//     // 获取最终键
//     std::string key = parts.back();
//     parts.pop_back();
    
//     try {
//         // 获取父表
//         const Values* parent = this;
//         if (!parts.empty()) {
//             std::string parentPath;
//             for (size_t i = 0; i < parts.size(); ++i) {
//                 if (i > 0) parentPath += ".";
//                 parentPath += parts[i];
//             }
//             parent = Table(parentPath).get();
//         }
        
//         if (!parent->IsMap()) {
//             return std::nullopt;
//         }
        
//         const auto& parentMap = parent->AsMap();
//         auto it = parentMap.find(key);
//         if (it != parentMap.end()) {
//             return it->second->DeepCopy();
//         }
//     } catch (const ValueError&) {
//         // 如果路径上任何部分找不到，则返回空
//     }
    
//     return std::nullopt;
// }

std::optional<std::shared_ptr<Values>> Values::PathValue(const std::string& path) const {
    std::cout << "PathValue: 尝试访问路径 " << path << std::endl;
    
    // 分割路径
    std::vector<std::string> parts = SplitPath(path);
    
    if (parts.empty()) {
        std::cout << "PathValue: 空路径" << std::endl;
        return std::nullopt;
    }
    
    std::cout << "PathValue: 路径分割为 " << parts.size() << " 个部分" << std::endl;
    for (const auto& part : parts) {
        std::cout << "  部分: " << part << std::endl;
    }
    
    // 从当前值开始遍历路径
    std::shared_ptr<Values> current = const_cast<Values*>(this)->shared_from_this();
    
    for (size_t i = 0; i < parts.size(); ++i) {
        if (!current || !current->IsMap()) {
            std::cout << "PathValue: " << parts[i] << " 不是map或为空" << std::endl;
            return std::nullopt;
        }
        
        const auto& map = current->AsMap();
        auto it = map.find(parts[i]);
        if (it == map.end()) {
            std::cout << "PathValue: 未找到键 " << parts[i] << std::endl;
            return std::nullopt;
        }
        
        current = it->second;
        std::cout << "PathValue: 找到部分 " << parts[i] << std::endl;
    }
    
    return current;
}

// 判断是否包含键
bool Values::Contains(const std::string& key) const {
    if (!IsMap()) {
        return false;
    }
    const auto& map = AsMap();
    return map.find(key) != map.end();
}

// 操作符[]实现
std::shared_ptr<Values>& Values::operator[](const std::string& key) {
    if (!IsMap()) {
        value_ = std::map<std::string, std::shared_ptr<Values>>{};
    }
    auto& map = AsMap();
    if (map.find(key) == map.end()) {
        map[key] = MakeNull();
    }
    return map[key];
}

std::shared_ptr<Values>& Values::operator[](size_t index) {
    if (!IsList()) {
        value_ = std::vector<std::shared_ptr<Values>>{};
    }
    auto& list = AsList();
    if (index >= list.size()) {
        list.resize(index + 1);
    }
    if (!list[index]) {
        list[index] = MakeNull();
    }
    return list[index];
}

// 深拷贝实现
std::shared_ptr<Values> Values::DeepCopy() const {
    if (IsMap()) {
        auto result = MakeMap();
        for (const auto& [key, value] : AsMap()) {
            if (value) {
                result->AsMap()[key] = value->DeepCopy();
            } else {
                result->AsMap()[key] = nullptr;
            }
        }
        return result;
    } else if (IsList()) {
        auto result = MakeList();
        auto& resultList = result->AsList();
        for (const auto& item : AsList()) {
            if (item) {
                resultList.push_back(item->DeepCopy());
            } else {
                resultList.push_back(nullptr);
            }
        }
        return result;
    } else {
        // 基本类型可以直接创建新对象
        return std::make_shared<Values>(value_);
    }
}

// 输出调试信息
void Values::Print(std::ostream& os, int indent) const {
    std::string padding(indent * 2, ' ');

    if (IsNull()) {
        os << padding << "null" << std::endl;
    } else if (IsBool()) {
        os << padding << (AsBool() ? "true" : "false") << std::endl;
    } else if (IsNumber()) {
        os << padding << AsNumber() << std::endl;
    } else if (IsString()) {
        os << padding << "\"" << AsString() << "\"" << std::endl;
    } else if (IsMap()) {
        os << padding << "{" << std::endl;
        printMap(AsMap(), os, indent + 1);
        os << padding << "}" << std::endl;
    } else if (IsList()) {
        os << padding << "[" << std::endl;
        printList(AsList(), os, indent + 1);
        os << padding << "]" << std::endl;
    }
}

void Values::printMap(const std::map<std::string, std::shared_ptr<Values>>& map, 
                     std::ostream& os, int indent) const {
    std::string padding(indent * 2, ' ');
    
    for (const auto& [key, value] : map) {
        os << padding << "\"" << key << "\": ";
        if (value) {
            if (value->IsMap() || value->IsList()) {
                os << std::endl;
                value->Print(os, indent + 1);
            } else {
                value->Print(os, 0);
            }
        } else {
            os << "null" << std::endl;
        }
    }
}

void Values::printList(const std::vector<std::shared_ptr<Values>>& list, 
                      std::ostream& os, int indent) const {
    std::string padding(indent * 2, ' ');
    
    for (const auto& item : list) {
        os << padding;
        if (item) {
            if (item->IsMap() || item->IsList()) {
                os << std::endl;
                item->Print(os, indent + 1);
            } else {
                item->Print(os, 0);
            }
        } else {
            os << "null" << std::endl;
        }
    }
}

// YAML序列化和反序列化 - 这里只是一个简化实现，实际使用应依赖yaml-cpp库
std::string Values::ToYAML() const {
    std::stringstream ss;
    Print(ss, 0);
    return ss.str();
}

bool Values::Encode(std::ostream& out) const {
    try {
        out << ToYAML();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// 简化版的FromYAML - 实际应该使用yaml-cpp库
std::shared_ptr<Values> Values::FromYAML(const std::string& yamlContent) {
    // 这里是极其简化的YAML解析器，仅作示范
    // 实际实现应该使用yaml-cpp库
    
    auto result = MakeMap();
    std::stringstream ss(yamlContent);
    std::string line;
    
    while (std::getline(ss, line)) {
        // 去除前后空白
        line = std::regex_replace(line, std::regex("^\\s+|\\s+$"), "");
        if (line.empty() || line[0] == '#') continue;
        
        // 寻找 key: value 模式
        auto pos = line.find(':');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // 去除空白
            key = std::regex_replace(key, std::regex("^\\s+|\\s+$"), "");
            value = std::regex_replace(value, std::regex("^\\s+|\\s+$"), "");
            
            if (!key.empty()) {
                if (value.empty()) {
                    // 空映射或空列表
                    result->AsMap()[key] = MakeMap();
                } else if (value == "true") {
                    result->AsMap()[key] = MakeBool(true);
                } else if (value == "false") {
                    result->AsMap()[key] = MakeBool(false);
                } else if (value == "null") {
                    result->AsMap()[key] = MakeNull();
                } else if (std::regex_match(value, std::regex("^-?\\d+(\\.\\d+)?$"))) {
                    // 数字
                    result->AsMap()[key] = MakeNumber(std::stod(value));
                } else {
                    // 字符串 - 去掉可能的引号
                    if ((value.front() == '"' && value.back() == '"') ||
                        (value.front() == '\'' && value.back() == '\'')) {
                        value = value.substr(1, value.length() - 2);
                    }
                    result->AsMap()[key] = MakeString(value);
                }
            }
        }
    }
    
    return result;
}

std::shared_ptr<Values> Values::FromYAMLFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw ValueError(ValueErrorType::NoTable, "Could not open file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return FromYAML(buffer.str());
}

// 合并值
std::shared_ptr<Values> CoalesceValues(const std::shared_ptr<Values>& base, 
                                       const std::shared_ptr<Values>& overlay) {
    // 如果base为空，返回overlay的副本
    if (!base || base->IsNull()) {
        return overlay ? overlay->DeepCopy() : Values::MakeNull();
    }
    
    // 如果overlay为空，返回base的副本
    if (!overlay || overlay->IsNull()) {
        return base->DeepCopy();
    }
    
    // 如果两者都是map，递归合并
    if (base->IsMap() && overlay->IsMap()) {
        auto result = Values::MakeMap();
        
        // 首先复制base的所有键
        for (const auto& [key, value] : base->AsMap()) {
            result->AsMap()[key] = value->DeepCopy();
        }
        
        // 然后合并overlay的键
        for (const auto& [key, overlayValue] : overlay->AsMap()) {
            auto baseIt = base->AsMap().find(key);
            if (baseIt != base->AsMap().end()) {
                // 键存在于base中，需要递归合并
                result->AsMap()[key] = CoalesceValues(baseIt->second, overlayValue);
            } else {
                // 键不存在于base中，直接复制
                result->AsMap()[key] = overlayValue->DeepCopy();
            }
        }
        
        return result;
    }
    
    // 其他情况，overlay覆盖base
    return overlay->DeepCopy();
}

// 准备渲染值
std::shared_ptr<Values> ToRenderValues(
    const std::string& chartName,
    const std::string& chartVersion,
    const std::shared_ptr<Values>& chartValues,
    const RenderOptions& options) {
    
    // 创建顶层值对象
    auto topLevel = Values::MakeMap();
    
    // 添加图表元数据
    auto chart = Values::MakeMap();
    chart->AsMap()["Name"] = Values::MakeString(chartName);
    chart->AsMap()["Version"] = Values::MakeString(chartVersion);
    topLevel->AsMap()["Chart"] = chart;
    
    // 添加发布信息
    auto release = Values::MakeMap();
    release->AsMap()["Name"] = Values::MakeString(options.name);
    release->AsMap()["Namespace"] = Values::MakeString(options.nameSpace);
    release->AsMap()["IsUpgrade"] = Values::MakeBool(options.isUpgrade);
    release->AsMap()["IsInstall"] = Values::MakeBool(options.isInstall);
    release->AsMap()["Revision"] = Values::MakeNumber(options.revision);
    release->AsMap()["Service"] = Values::MakeString("Helm-CPP");
    topLevel->AsMap()["Release"] = release;
    
    // 添加能力信息（这里简化了，实际Helm有更复杂的Capabilities对象）
    auto capabilities = Values::MakeMap();
    auto kubeVersion = Values::MakeMap();
    kubeVersion->AsMap()["Major"] = Values::MakeString("1");
    kubeVersion->AsMap()["Minor"] = Values::MakeString("23");
    capabilities->AsMap()["KubeVersion"] = kubeVersion;
    topLevel->AsMap()["Capabilities"] = capabilities;
    
    // 合并图表值
    auto values = chartValues ? chartValues->DeepCopy() : Values::MakeMap();
    topLevel->AsMap()["Values"] = values;
    
    return topLevel;
}

} // namespace template_engine