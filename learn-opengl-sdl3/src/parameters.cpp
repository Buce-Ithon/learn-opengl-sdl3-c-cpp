#include "render/parameters.h"

#include "core/shader.h"
#include "render/shader_contract.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string_view>

namespace {
std::string trim(std::string_view text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin]))) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
    return std::string(text.substr(begin, end - begin));
}

bool parseType(std::string_view name, ParameterType& type) {
    if (name == "float") type = ParameterType::Float;
    else if (name == "vec2") type = ParameterType::Vec2;
    else if (name == "vec3") type = ParameterType::Vec3;
    else if (name == "vec4") type = ParameterType::Vec4;
    else if (name == "int") type = ParameterType::Int;
    else if (name == "bool") type = ParameterType::Bool;
    else return false;
    return true;
}

bool isIdentifier(std::string_view name) {
    if (name.empty() || (!std::isalpha(static_cast<unsigned char>(name.front())) && name.front() != '_')) return false;
    for (const char character : name) {
        if (!std::isalnum(static_cast<unsigned char>(character)) && character != '_') return false;
    }
    return true;
}

bool parseValues(std::string_view text, std::vector<float>& values) {
    values.clear();
    std::size_t begin = 0;
    while (begin <= text.size()) {
        const std::size_t comma = text.find(',', begin);
        const std::string_view token = text.substr(begin, comma == std::string_view::npos ? std::string_view::npos : comma - begin);
        const std::string trimmed = trim(token);
        if (trimmed.empty()) return false;
        char* end = nullptr;
        const float value = std::strtof(trimmed.c_str(), &end);
        if (end == trimmed.c_str() || *end != '\0') return false;
        values.push_back(value);
        if (comma == std::string_view::npos) break;
        begin = comma + 1;
    }
    return !values.empty();
}

std::array<float, 4> expandValues(const std::vector<float>& values, int components) {
    std::array<float, 4> expanded{};
    for (int i = 0; i < components; ++i) {
        expanded[i] = values[static_cast<std::size_t>(std::min<std::size_t>(i, values.size() - 1))];
    }
    return expanded;
}

int componentCount(ParameterType type) {
    switch (type) {
    case ParameterType::Float:
    case ParameterType::Int:
    case ParameterType::Bool: return 1;
    case ParameterType::Vec2: return 2;
    case ParameterType::Vec3: return 3;
    case ParameterType::Vec4: return 4;
    }
    return 0;
}

ParameterType parameterTypeFromGl(GLenum type, bool& supported) {
    supported = true;
    switch (type) {
    case GL_FLOAT: return ParameterType::Float;
    case GL_FLOAT_VEC2: return ParameterType::Vec2;
    case GL_FLOAT_VEC3: return ParameterType::Vec3;
    case GL_FLOAT_VEC4: return ParameterType::Vec4;
    case GL_INT: return ParameterType::Int;
    case GL_BOOL: return ParameterType::Bool;
    default:
        supported = false;
        return ParameterType::Float;
    }
}

bool parseAnnotation(std::string_view line, Parameter& parameter, std::string& error) {
    const std::size_t marker = line.find("@param");
    if (marker == std::string_view::npos) return true;
    std::istringstream stream{std::string(line.substr(marker + 6))};
    std::string typeName;
    std::string minimumText;
    std::string maximumText;
    std::string defaultText;
    if (!(stream >> parameter.name >> typeName >> minimumText >> maximumText >> defaultText)) {
        error = "Invalid @param annotation; expected: // @param <name> <type> <min> <max> <default> \"<label>\"";
        return false;
    }
    if (!isIdentifier(parameter.name)) { error = "Invalid parameter name: " + parameter.name; return false; }
    if (shader_uniform::isReserved(parameter.name)) { error = "Parameter name is reserved by the renderer: " + parameter.name; return false; }
    if (!parseType(typeName, parameter.type)) { error = "Unknown parameter type: " + typeName; return false; }

    std::vector<float> minimum;
    std::vector<float> maximum;
    std::vector<float> defaultValue;
    if (!parseValues(minimumText, minimum) || !parseValues(maximumText, maximum) || !parseValues(defaultText, defaultValue)) {
        error = "Invalid numeric value in @param annotation for " + parameter.name;
        return false;
    }

    std::string label;
    std::getline(stream, label);
    label = trim(label);
    if (label.size() >= 2 && label.front() == '"' && label.back() == '"') label = label.substr(1, label.size() - 2);
    parameter.label = label.empty() ? parameter.name : label;

    const int components = componentCount(parameter.type);
    parameter.minimum = expandValues(minimum, components);
    parameter.maximum = expandValues(maximum, components);
    parameter.defaultValue = expandValues(defaultValue, components);
    for (int i = 0; i < components; ++i) {
        if (parameter.minimum[i] > parameter.maximum[i]) { error = "Parameter minimum exceeds maximum: " + parameter.name; return false; }
        parameter.defaultValue[i] = std::clamp(parameter.defaultValue[i], parameter.minimum[i], parameter.maximum[i]);
        const float span = parameter.maximum[i] - parameter.minimum[i];
        parameter.step[i] = (parameter.type == ParameterType::Int || parameter.type == ParameterType::Bool) ? 1.0f : std::max(span / 100.0f, 0.0001f);
    }
    if (parameter.type == ParameterType::Bool) {
        for (int i = 0; i < components; ++i) {
            parameter.minimum[i] = 0.0f;
            parameter.maximum[i] = 1.0f;
            parameter.defaultValue[i] = parameter.defaultValue[i] >= 0.5f ? 1.0f : 0.0f;
        }
    }
    parameter.value = parameter.defaultValue;
    parameter.reflected = false;
    return true;
}

std::string formatValue(float value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.3g", static_cast<double>(value));
    return buffer;
}
}

int ParameterSet::componentCount(ParameterType type) { return ::componentCount(type); }

bool ParameterSet::addAnnotations(const std::filesystem::path& fragmentPath, std::string& error) {
    std::ifstream file(fragmentPath, std::ios::binary);
    if (!file) {
        error = "Unable to read shader file: " + fragmentPath.string();
        return false;
    }

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        const std::string trimmed = trim(line);
        if (!trimmed.starts_with("//") || trimmed.find("@param") == std::string::npos) continue;

        Parameter parameter;
        std::string parseError;
        if (!parseAnnotation(trimmed, parameter, parseError)) {
            error = fragmentPath.string() + ":" + std::to_string(lineNumber) + ": " + parseError;
            return false;
        }

        const auto existing = std::find_if(parameters_.begin(), parameters_.end(), [&](const Parameter& candidate) { return candidate.name == parameter.name; });
        if (existing == parameters_.end()) parameters_.push_back(std::move(parameter));
    }
    return true;
}

void ParameterSet::reflectProgram(GLuint program) {
    GLint activeUniforms = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &activeUniforms);
    for (GLint index = 0; index < activeUniforms; ++index) {
        char nameBuffer[256]{};
        GLsizei nameLength = 0;
        GLint size = 0;
        GLenum glType = 0;
        glGetActiveUniform(program, static_cast<GLuint>(index), sizeof(nameBuffer), &nameLength, &size, &glType, nameBuffer);
        const std::string_view fullName(nameBuffer, static_cast<std::size_t>(std::max(0, nameLength)));
        if (fullName.find('[') != std::string_view::npos) continue;

        bool supported = false;
        const ParameterType type = parameterTypeFromGl(glType, supported);
        if (!supported || shader_uniform::isReserved(fullName)) continue;
        if (std::any_of(parameters_.begin(), parameters_.end(), [&](const Parameter& candidate) { return candidate.name == fullName; })) continue;

        Parameter parameter;
        parameter.name = fullName;
        parameter.label = parameter.name;
        parameter.type = type;
        const int components = componentCount(type);
        for (int i = 0; i < components; ++i) {
            parameter.minimum[i] = (type == ParameterType::Int) ? 0.0f : -1.0f;
            parameter.maximum[i] = (type == ParameterType::Int) ? 10.0f : 1.0f;
            parameter.step[i] = (type == ParameterType::Int || type == ParameterType::Bool) ? 1.0f : 0.01f;
        }
        if (type == ParameterType::Bool) {
            parameter.minimum[0] = 0.0f;
            parameter.maximum[0] = 1.0f;
        }
        parameter.reflected = true;
        parameters_.push_back(std::move(parameter));
    }
}

void ParameterSet::apply(const Shader& shader) const {
    for (const Parameter& parameter : parameters_) {
        switch (parameter.type) {
        case ParameterType::Float: shader.setFloat(parameter.name, parameter.value[0]); break;
        case ParameterType::Vec2: shader.setVec2(parameter.name, parameter.value[0], parameter.value[1]); break;
        case ParameterType::Vec3: shader.setVec3(parameter.name, parameter.value[0], parameter.value[1], parameter.value[2]); break;
        case ParameterType::Vec4: shader.setVec4(parameter.name, parameter.value[0], parameter.value[1], parameter.value[2], parameter.value[3]); break;
        case ParameterType::Int:
        case ParameterType::Bool: shader.setInt(parameter.name, static_cast<int>(parameter.value[0])); break;
        }
    }
}

void ParameterSet::selectNext(int direction) {
    if (parameters_.empty()) return;
    const auto size = static_cast<long long>(parameters_.size());
    const long long next = (static_cast<long long>(selected_) + direction) % size;
    selected_ = static_cast<std::size_t>(next < 0 ? next + size : next);
}

bool ParameterSet::adjustSelected(float direction) {
    if (parameters_.empty()) return false;
    Parameter& parameter = parameters_[selected_];
    bool changed = false;
    for (int i = 0; i < componentCount(parameter.type); ++i) {
        const float next = std::clamp(parameter.value[i] + parameter.step[i] * direction, parameter.minimum[i], parameter.maximum[i]);
        if (next != parameter.value[i]) {
            parameter.value[i] = next;
            changed = true;
        }
    }
    return changed;
}

bool ParameterSet::resetSelected() {
    if (parameters_.empty()) return false;
    Parameter& parameter = parameters_[selected_];
    if (parameter.value == parameter.defaultValue) return false;
    parameter.value = parameter.defaultValue;
    return true;
}

void ParameterSet::inheritValues(const ParameterSet& previous) {
    if (previous.parameters_.empty() || parameters_.empty()) return;
    const std::string selectedName = previous.parameters_[previous.selected_].name;
    for (Parameter& parameter : parameters_) {
        const auto match = std::find_if(previous.parameters_.begin(), previous.parameters_.end(), [&](const Parameter& candidate) {
            return candidate.name == parameter.name && candidate.type == parameter.type;
        });
        if (match != previous.parameters_.end()) parameter.value = match->value;
    }
    const auto selected = std::find_if(parameters_.begin(), parameters_.end(), [&](const Parameter& candidate) { return candidate.name == selectedName; });
    selected_ = selected == parameters_.end() ? 0 : static_cast<std::size_t>(selected - parameters_.begin());
}

std::string ParameterSet::valueText(const Parameter& parameter) {
    const int components = componentCount(parameter.type);
    if (components == 1) return formatValue(parameter.value[0]);
    std::string text;
    for (int i = 0; i < components; ++i) {
        if (i != 0) text += ", ";
        text += formatValue(parameter.value[i]);
    }
    return text;
}
