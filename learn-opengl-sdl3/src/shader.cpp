#include "core/shader.h"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace {
std::string readTextFile(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error(std::string("Unable to read shader file: ") + path);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

GLuint compileShader(GLenum type, const std::string& source, const char* path) {
    const char* sourcePointer = source.c_str();
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &sourcePointer, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) return shader;

    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string log(static_cast<std::size_t>(length), '\0');
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    glDeleteShader(shader);
    throw std::runtime_error(std::string("Shader compilation failed for ") + path + ":\n" + log);
}
}

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    const std::string vertexSource = readTextFile(vertexPath);
    const std::string fragmentSource = readTextFile(fragmentPath);
    const GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSource, vertexPath);

    try {
        const GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSource, fragmentPath);
        id_ = glCreateProgram();
        glAttachShader(id_, vertex);
        glAttachShader(id_, fragment);
        glLinkProgram(id_);
        glDeleteShader(fragment);
    } catch (...) {
        glDeleteShader(vertex);
        throw;
    }
    glDeleteShader(vertex);

    if (!isLinked()) {
        GLint length = 0;
        glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &length);
        std::string log(static_cast<std::size_t>(length), '\0');
        glGetProgramInfoLog(id_, length, nullptr, log.data());
        glDeleteProgram(std::exchange(id_, 0));
        throw std::runtime_error("Shader program link failed:\n" + log);
    }
}

Shader::~Shader() { if (id_ != 0) glDeleteProgram(id_); }

Shader::Shader(Shader&& other) noexcept
    : id_(std::exchange(other.id_, 0)), uniformCache_(std::move(other.uniformCache_)) {}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this == &other) return *this;
    if (id_ != 0) glDeleteProgram(id_);
    id_ = std::exchange(other.id_, 0);
    uniformCache_ = std::move(other.uniformCache_);
    return *this;
}

bool Shader::isLinked() const {
    GLint linked = GL_FALSE;
    glGetProgramiv(id_, GL_LINK_STATUS, &linked);
    return linked == GL_TRUE;
}

void Shader::use() const { glUseProgram(id_); }
void Shader::setBool(const std::string& name, bool value) const { glUniform1i(uniformLocation(name), value); }
void Shader::setInt(const std::string& name, int value) const { glUniform1i(uniformLocation(name), value); }
void Shader::setFloat(const std::string& name, float value) const { glUniform1f(uniformLocation(name), value); }
void Shader::setVec2(const std::string& name, const glm::vec2& value) const { glUniform2fv(uniformLocation(name), 1, &value[0]); }
void Shader::setVec2(const std::string& name, float x, float y) const { glUniform2f(uniformLocation(name), x, y); }
void Shader::setVec3(const std::string& name, const glm::vec3& value) const { glUniform3fv(uniformLocation(name), 1, &value[0]); }
void Shader::setVec3(const std::string& name, float x, float y, float z) const { glUniform3f(uniformLocation(name), x, y, z); }
void Shader::setVec4(const std::string& name, const glm::vec4& value) const { glUniform4fv(uniformLocation(name), 1, &value[0]); }
void Shader::setVec4(const std::string& name, float x, float y, float z, float w) const { glUniform4f(uniformLocation(name), x, y, z, w); }
void Shader::setMat2(const std::string& name, const glm::mat2& matrix) const { glUniformMatrix2fv(uniformLocation(name), 1, GL_FALSE, &matrix[0][0]); }
void Shader::setMat3(const std::string& name, const glm::mat3& matrix) const { glUniformMatrix3fv(uniformLocation(name), 1, GL_FALSE, &matrix[0][0]); }
void Shader::setMat4(const std::string& name, const glm::mat4& matrix) const { glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, &matrix[0][0]); }

GLint Shader::uniformLocation(const std::string& name) const {
    if (const auto it = uniformCache_.find(name); it != uniformCache_.end()) return it->second;
    const GLint location = glGetUniformLocation(id_, name.c_str());
    uniformCache_.emplace(name, location);
    return location;
}
