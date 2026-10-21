#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <iterator>
#include <string>
#include <fstream>
#include <iostream>

#include <unordered_map>

class Shader {
public:
    // program ID
    GLuint ID;

    Shader(const char* vsPath, const char* fsPath) {
        // 1. retrieve the vertex/fragment shader source code from filePath
        std::ifstream vsFile;
        std::ifstream fsFile;
        std::string vCode;
        std::string fCode;

        // ensure ifstream objects can throw exceptions
        vsFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fsFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            // open files
            vsFile.open(vsPath);
            fsFile.open(fsPath);
            // directly get string from file (>=C++11)
            vCode = std::string((std::istreambuf_iterator<char>(vsFile)), std::istreambuf_iterator<char>());
            fCode = std::string((std::istreambuf_iterator<char>(fsFile)), std::istreambuf_iterator<char>());
        } catch (std::ifstream::failure &e) {
            std::cout << "ERROR::SHADER_FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        }
        const char* vsCode = vCode.c_str();
        const char* fsCode = fCode.c_str();
        
        // 2. compile and link shaders
        // vertex shader
        GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vsCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment shader
        GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fsCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // shader program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete shaders as they're linked into program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }
    // use/active shader program
    void use() const {
        glUseProgram(ID);
    }
    // utility uniform functions
    void setBool(const std::string &name, bool value) const {
        glUniform1i(getUniformLocation(name), static_cast<GLint>(value));
    }
    void setInt(const std::string &name, int value) const {
        glUniform1i(getUniformLocation(name), value);
    }
    void setFloat(const std::string &name, float value) const {
        glUniform1f(getUniformLocation(name), value);
    }
    void setVec2(const std::string &name, const glm::vec2 &value) const { 
        glUniform2fv(getUniformLocation(name), 1, &value[0]); 
    }
    void setVec2(const std::string &name, float x, float y) const { 
        glUniform2f(getUniformLocation(name), x, y); 
    }
    void setVec3(const std::string &name, const glm::vec3 &value) const { 
        glUniform3fv(getUniformLocation(name), 1, &value[0]); 
    }
    void setVec3(const std::string &name, float x, float y, float z) const { 
        glUniform3f(getUniformLocation(name), x, y, z); 
    }
    void setVec4(const std::string &name, const glm::vec4 &value) const { 
        glUniform4fv(getUniformLocation(name), 1, &value[0]); 
    }
    void setVec4(const std::string &name, float x, float y, float z, float w) const { 
        glUniform4f(getUniformLocation(name), x, y, z, w); 
    }
    void setMat2(const std::string &name, const glm::mat2 &mat) const {
        glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
    }
    void setMat3(const std::string &name, const glm::mat3 &mat) const {
        glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
    }
    void setMat4(const std::string &name, const glm::mat4 &mat) const {
        glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
    }
private:
    // utility function for checking shader compilation/linking errors
    void checkCompileErrors(GLuint shader, std::string type) {
        GLint success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << '\n' << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << '\n' << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
    // cache accelerate for glUniform** function
    mutable std::unordered_map<std::string, GLint> uniformCache;
    // add cache before finding uniform param
    GLint getUniformLocation(const std::string &name) const {
        auto it = uniformCache.find(name);
        if (it != uniformCache.end()) return it->second;
        GLint loc = glGetUniformLocation(ID, name.c_str());
        uniformCache.emplace(name, loc);
        return loc;
    }
};