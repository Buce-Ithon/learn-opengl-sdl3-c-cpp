#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include <glad/glad.h>

class Shader;

enum class ParameterType { Float, Vec2, Vec3, Vec4, Int, Bool };

struct Parameter {
    std::string name;
    std::string label;
    ParameterType type = ParameterType::Float;
    std::array<float, 4> value{};
    std::array<float, 4> defaultValue{};
    std::array<float, 4> minimum{};
    std::array<float, 4> maximum{};
    std::array<float, 4> step{};
    bool reflected = false;
};

// User-tunable uniforms for one effect.
//
// Parameters are discovered from `// @param` annotations in the fragment shaders, and any
// remaining active uniforms are added through GL reflection as a fallback. Framework-owned
// uniforms (resolution, time, input textures, interaction state) are never exposed.
class ParameterSet {
public:
    [[nodiscard]] bool addAnnotations(const std::filesystem::path& fragmentPath, std::string& error);
    void reflectProgram(GLuint program);
    void apply(const Shader& shader) const;

    [[nodiscard]] const std::vector<Parameter>& parameters() const noexcept { return parameters_; }
    [[nodiscard]] std::size_t selectedIndex() const noexcept { return selected_; }
    [[nodiscard]] bool empty() const noexcept { return parameters_.empty(); }

    void selectNext(int direction);
    [[nodiscard]] bool adjustSelected(float direction);
    [[nodiscard]] bool resetSelected();
    void inheritValues(const ParameterSet& previous);

    [[nodiscard]] static std::string valueText(const Parameter& parameter);
    [[nodiscard]] static int componentCount(ParameterType type);

private:
    std::vector<Parameter> parameters_;
    std::size_t selected_ = 0;
};
