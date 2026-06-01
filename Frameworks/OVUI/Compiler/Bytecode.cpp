#include "Bytecode.h"
#include <fstream>
#include <cstring>
#include <cstdio>
#include <limits>

namespace ovui {

void BytecodeModule::add_instruction(BytecodeOp op, int32_t a0, int32_t a1, int32_t a2, int32_t a3) {
    if (instructions.size() >= MAX_INSTRUCTIONS) return;
    instructions.push_back({op, {a0, a1, a2, a3}});
}

int32_t BytecodeModule::add_constant_int(int64_t v) {
    if (constants.size() >= MAX_CONSTANTS) return -1;
    constants.push_back({BytecodeConstant::Int, v, 0, ""});
    return (int32_t)(constants.size() - 1);
}

int32_t BytecodeModule::add_constant_float(float v) {
    if (constants.size() >= MAX_CONSTANTS) return -1;
    constants.push_back({BytecodeConstant::Float, 0, v, ""});
    return (int32_t)(constants.size() - 1);
}

int32_t BytecodeModule::add_constant_string(const std::string& v) {
    if (constants.size() >= MAX_CONSTANTS) return -1;
    if (v.size() > MAX_STRING_LEN) return -1;
    constants.push_back({BytecodeConstant::String, 0, 0, v});
    return (int32_t)(constants.size() - 1);
}

bool BytecodeModule::validate_magic(const uint8_t* header, size_t size) {
    if (size < 8) return false;
    uint32_t magic; std::memcpy(&magic, header, 4);
    return magic == MAGIC;
}

static std::string safe_read_str(std::ifstream& f, uint32_t max_len, bool& ok) {
    ok = false;
    uint32_t slen = 0;
    if (!f.read(reinterpret_cast<char*>(&slen), 4)) return "";
    if (slen > max_len) return "";
    std::string s;
    s.resize(slen);
    if (!f.read(s.data(), slen)) return "";
    ok = true;
    return s;
}

bool BytecodeModule::save(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    uint32_t magic = MAGIC, ver = VERSION;
    if (!f.write(reinterpret_cast<const char*>(&magic), 4)) return false;
    if (!f.write(reinterpret_cast<const char*>(&ver), 4)) return false;

    uint32_t ic = static_cast<uint32_t>(instructions.size());
    if (!f.write(reinterpret_cast<const char*>(&ic), 4)) return false;
    if (ic > 0) {
        if (!f.write(reinterpret_cast<const char*>(instructions.data()),
                     ic * sizeof(BytecodeInstruction))) return false;
    }

    uint32_t cc = static_cast<uint32_t>(constants.size());
    if (!f.write(reinterpret_cast<const char*>(&cc), 4)) return false;
    for (auto& c : constants) {
        uint32_t type = static_cast<uint32_t>(c.type);
        if (!f.write(reinterpret_cast<const char*>(&type), 4)) return false;
        switch (c.type) {
            case BytecodeConstant::Int:
                if (!f.write(reinterpret_cast<const char*>(&c.int_val), 8)) return false;
                break;
            case BytecodeConstant::Float:
                if (!f.write(reinterpret_cast<const char*>(&c.float_val), 4)) return false;
                break;
            case BytecodeConstant::String: {
                if (c.str_val.size() > MAX_STRING_LEN) return false;
                uint32_t slen = static_cast<uint32_t>(c.str_val.size());
                if (!f.write(reinterpret_cast<const char*>(&slen), 4)) return false;
                if (!f.write(c.str_val.data(), slen)) return false;
                break;
            }
        }
    }
    return true;
}

std::unique_ptr<BytecodeModule> BytecodeModule::load(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return nullptr;

    std::streamsize file_size = f.tellg();
    if (file_size < 0 || static_cast<uint64_t>(file_size) > MAX_MODULE_SIZE) return nullptr;
    f.seekg(0, std::ios::beg);

    uint32_t magic = 0, ver = 0;
    if (!f.read(reinterpret_cast<char*>(&magic), 4)) return nullptr;
    if (magic != MAGIC) {
        fprintf(stderr, "[BytecodeModule] Invalid magic: 0x%08X\n", magic);
        return nullptr;
    }
    if (!f.read(reinterpret_cast<char*>(&ver), 4)) return nullptr;
    if (ver != VERSION) {
        fprintf(stderr, "[BytecodeModule] Unsupported version: %u\n", ver);
        return nullptr;
    }

    uint32_t ic = 0;
    if (!f.read(reinterpret_cast<char*>(&ic), 4)) return nullptr;
    if (ic > MAX_INSTRUCTIONS) {
        fprintf(stderr, "[BytecodeModule] Instruction count exceeds limit: %u\n", ic);
        return nullptr;
    }

    auto mod = std::make_unique<BytecodeModule>();
    if (ic > 0) {
        mod->instructions.resize(ic);
        if (!f.read(reinterpret_cast<char*>(mod->instructions.data()),
                    ic * sizeof(BytecodeInstruction))) {
            fprintf(stderr, "[BytecodeModule] Truncated instruction data\n");
            return nullptr;
        }
    }

    uint32_t cc = 0;
    if (!f.read(reinterpret_cast<char*>(&cc), 4)) return nullptr;
    if (cc > MAX_CONSTANTS) {
        fprintf(stderr, "[BytecodeModule] Constant count exceeds limit: %u\n", cc);
        return nullptr;
    }

    for (uint32_t i = 0; i < cc; i++) {
        uint32_t type_raw = 0;
        if (!f.read(reinterpret_cast<char*>(&type_raw), 4)) {
            fprintf(stderr, "[BytecodeModule] Truncated constant type at index %u\n", i);
            return nullptr;
        }
        if (type_raw > 2) {
            fprintf(stderr, "[BytecodeModule] Invalid constant type %u at index %u\n", type_raw, i);
            return nullptr;
        }

        BytecodeConstant c;
        c.type = static_cast<BytecodeConstant::Type>(type_raw);

        switch (c.type) {
            case BytecodeConstant::Int:
                if (!f.read(reinterpret_cast<char*>(&c.int_val), 8)) return nullptr;
                break;
            case BytecodeConstant::Float:
                if (!f.read(reinterpret_cast<char*>(&c.float_val), 4)) return nullptr;
                break;
            case BytecodeConstant::String: {
                uint32_t slen = 0;
                if (!f.read(reinterpret_cast<char*>(&slen), 4)) return nullptr;
                if (slen > MAX_STRING_LEN) {
                    fprintf(stderr, "[BytecodeModule] String too long at constant %u: %u\n", i, slen);
                    return nullptr;
                }
                c.str_val.resize(slen);
                if (!f.read(c.str_val.data(), slen)) {
                    fprintf(stderr, "[BytecodeModule] Truncated string at constant %u\n", i);
                    return nullptr;
                }
                break;
            }
        }
        mod->constants.push_back(std::move(c));
    }

    return mod;
}

std::optional<const BytecodeConstant*> BytecodeModule::get_constant(int32_t id) const {
    if (id < 0 || static_cast<size_t>(id) >= constants.size()) return std::nullopt;
    return &constants[id];
}

std::optional<const BytecodeConstant*> BytecodeModule::get_constant_string(int32_t id) const {
    auto c = get_constant(id);
    if (!c || (*c)->type != BytecodeConstant::String) return std::nullopt;
    return c;
}

std::optional<const BytecodeConstant*> BytecodeModule::get_constant_int(int32_t id) const {
    auto c = get_constant(id);
    if (!c || (*c)->type != BytecodeConstant::Int) return std::nullopt;
    return c;
}

std::optional<const BytecodeConstant*> BytecodeModule::get_constant_float(int32_t id) const {
    auto c = get_constant(id);
    if (!c || (*c)->type != BytecodeConstant::Float) return std::nullopt;
    return c;
}

void BytecodeModule::dump() const {
    printf("Bytecode Module: %zu instructions, %zu constants\n", instructions.size(), constants.size());
    for (size_t i = 0; i < instructions.size(); i++) {
        auto& in = instructions[i];
        printf("  %04zu: op=%d args=[%d,%d,%d,%d]\n", i, (int)in.op,
               in.args[0], in.args[1], in.args[2], in.args[3]);
    }
    printf("Constants:\n");
    for (size_t i = 0; i < constants.size(); i++) {
        auto& c = constants[i];
        if (c.type == BytecodeConstant::Int) printf("  %zu: int %ld\n", i, c.int_val);
        else if (c.type == BytecodeConstant::Float) printf("  %zu: float %f\n", i, c.float_val);
        else printf("  %zu: str '%s'\n", i, c.str_val.c_str());
    }
}

} // namespace ovui
