#pragma once

#include <Core/Types.h>
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <memory>
#include <functional>
#include <optional>

namespace ovui {

enum class BytecodeOp : uint8_t {
    Nop = 0,
    CreateWidget,       // type_id, widget_id
    SetAttribute,       // widget_id, key_id, value_id
    AddChild,           // parent_id, child_id
    SetStyleClass,      // widget_id, class_id
    SetDirection,       // widget_id, direction(0=col,1=row)
    SetPadding,         // widget_id, top,right,bottom,left
    SetGap,             // widget_id, gap
    SetText,            // widget_id, text_id
    SetEventHandler,    // widget_id, event_type, handler_id
    PushWidget,         // widget_id (push to stack)
    PopWidget,          // (pop from stack)
    SetRoot,            // widget_id
    Layout,             // width, height
    End                 // end of program
};

struct BytecodeInstruction {
    BytecodeOp op = BytecodeOp::Nop;
    int32_t args[4] = {0,0,0,0};
};

struct BytecodeConstant {
    enum Type : uint8_t { Int, Float, String } type;
    int64_t int_val = 0;
    float float_val = 0;
    std::string str_val;
};

class BytecodeModule {
public:
    static constexpr uint32_t MAGIC = 0x4F564243;
    static constexpr uint32_t VERSION = 1;

    static constexpr uint32_t MAX_INSTRUCTIONS = 1'000'000;
    static constexpr uint32_t MAX_CONSTANTS    = 500'000;
    static constexpr uint32_t MAX_STRING_LEN   = 1'048'576;
    static constexpr uint32_t MAX_MODULE_SIZE  = 256 * 1024 * 1024;

    std::vector<BytecodeInstruction> instructions;
    std::vector<BytecodeConstant> constants;

    void add_instruction(BytecodeOp op, int32_t a0=0, int32_t a1=0, int32_t a2=0, int32_t a3=0);
    int32_t add_constant_int(int64_t v);
    int32_t add_constant_float(float v);
    int32_t add_constant_string(const std::string& v);

    bool save(const std::string& path) const;
    static std::unique_ptr<BytecodeModule> load(const std::string& path);

    std::optional<const BytecodeConstant*> get_constant(int32_t id) const;
    std::optional<const BytecodeConstant*> get_constant_string(int32_t id) const;
    std::optional<const BytecodeConstant*> get_constant_int(int32_t id) const;
    std::optional<const BytecodeConstant*> get_constant_float(int32_t id) const;

    void dump() const;
    size_t size() const { return instructions.size(); }
    static bool validate_magic(const uint8_t* header, size_t size);
};

} // namespace ovui
