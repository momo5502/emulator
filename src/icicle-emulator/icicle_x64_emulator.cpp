#define ICICLE_EMULATOR_IMPL
#include "icicle_x64_emulator.hpp"

using icicle_emulator = struct icicle_emulator_;

extern "C"
{
    icicle_emulator* icicle_create_emulator();
    int32_t icicle_protect_memory(icicle_emulator*, uint64_t address, uint64_t length, uint8_t permissions);
    int32_t icicle_map_memory(icicle_emulator*, uint64_t address, uint64_t length, uint8_t permissions);
    int32_t icicle_unmap_memory(icicle_emulator*, uint64_t address, uint64_t length);
    int32_t icicle_read_memory(icicle_emulator*, uint64_t address, void* data, size_t length);
    int32_t icicle_write_memory(icicle_emulator*, uint64_t address, const void* data, size_t length);
    uint32_t icicle_add_syscall_hook(icicle_emulator*, void (*callback)(void*), void* data);
    void icicle_remove_syscall_hook(icicle_emulator*, uint32_t id);
    size_t icicle_read_register(icicle_emulator*, int reg, void* data, size_t length);
    size_t icicle_write_register(icicle_emulator*, int reg, const void* data, size_t length);
    void icicle_start(icicle_emulator*);
    void icicle_destroy_emulator(icicle_emulator*);
}

namespace icicle
{
    namespace
    {
        void ice(const bool result, const std::string_view error)
        {
            if (!result)
            {
                throw std::runtime_error(std::string(error));
            }
        }
    }

    class icicle_x64_emulator : public x64_emulator
    {
      public:
        icicle_x64_emulator()
            : emu_(icicle_create_emulator())
        {
            if (!this->emu_)
            {
                throw std::runtime_error("Failed to create icicle emulator instance");
            }
        }

        ~icicle_x64_emulator() override
        {
            if (this->emu_)
            {
                icicle_destroy_emulator(this->emu_);
                this->emu_ = nullptr;
            }
        }

        void start(const uint64_t start, const uint64_t end, std::chrono::nanoseconds timeout,
                   const size_t count) override
        {
            if (timeout.count() < 0)
            {
                timeout = {};
            }

            icicle_start(this->emu_);
        }

        void stop() override
        {
        }

        void load_gdt(const pointer_type address, const uint32_t limit) override
        {
            struct gdtr
            {
                uint32_t padding{};
                uint32_t limit{};
                uint64_t address{};
            };

            const gdtr entry{.limit = limit, .address = address};
            static_assert(sizeof(gdtr) - offsetof(gdtr, limit) == 12);

            this->write_register(x64_register::gdtr, &entry.limit, 12);
        }

        void set_segment_base(const x64_register base, const pointer_type value) override
        {
            switch (base)
            {
            case x64_register::fs:
            case x64_register::fs_base:
                this->reg(x64_register::fs_base, value);
                break;
            case x64_register::gs:
            case x64_register::gs_base:
                this->reg(x64_register::gs_base, value);
                break;
            default:
                break;
            }
        }

        size_t write_raw_register(const int reg, const void* value, const size_t size) override
        {
            return icicle_write_register(this->emu_, reg, value, size);
        }

        size_t read_raw_register(const int reg, void* value, const size_t size) override
        {
            return icicle_read_register(this->emu_, reg, value, size);
        }

        void map_mmio(const uint64_t address, const size_t size, mmio_read_callback read_cb,
                      mmio_write_callback write_cb) override
        {
            this->map_memory(address, size, memory_permission::read_write);
            // throw std::runtime_error("Not implemented");
        }

        void map_memory(const uint64_t address, const size_t size, memory_permission permissions) override
        {
            const auto res = icicle_map_memory(this->emu_, address, size, static_cast<uint8_t>(permissions));
            ice(res, "Failed to map memory");
        }

        void unmap_memory(const uint64_t address, const size_t size) override
        {
            const auto res = icicle_unmap_memory(this->emu_, address, size);
            ice(res, "Failed to unmap memory");
        }

        bool try_read_memory(const uint64_t address, void* data, const size_t size) const override
        {
            return icicle_read_memory(this->emu_, address, data, size);
        }

        void read_memory(const uint64_t address, void* data, const size_t size) const override
        {
            const auto res = this->try_read_memory(address, data, size);
            ice(res, "Failed to read memory");
        }

        void write_memory(const uint64_t address, const void* data, const size_t size) override
        {
            const auto res = icicle_write_memory(this->emu_, address, data, size);
            ice(res, "Failed to write memory");
        }

        void apply_memory_protection(const uint64_t address, const size_t size, memory_permission permissions) override
        {
            const auto res = icicle_protect_memory(this->emu_, address, size, static_cast<uint8_t>(permissions));
            ice(res, "Failed to apply permissions");
        }

        emulator_hook* hook_instruction(int instruction_type, instruction_hook_callback callback) override
        {
            if (static_cast<x64_hookable_instructions>(instruction_type) != x64_hookable_instructions::syscall)
            {
                return nullptr;
            }

            auto callback_store = std::make_unique<std::function<void()>>([c = std::move(callback)] {
                (void)c(); //
            });

            const auto invoker = +[](void* cb) {
                (*static_cast<std::function<void()>*>(cb))(); //
            };

            const auto id = icicle_add_syscall_hook(this->emu_, invoker, callback_store.get());
            this->syscall_hooks_[id] = std::move(callback_store);

            return reinterpret_cast<emulator_hook*>(static_cast<size_t>(id));
        }

        emulator_hook* hook_basic_block(basic_block_hook_callback callback) override
        {
            return nullptr;
            // throw std::runtime_error("Not implemented");
        }

        emulator_hook* hook_edge_generation(edge_generation_hook_callback callback) override
        {
            return nullptr;
            // throw std::runtime_error("Not implemented");
        }

        emulator_hook* hook_interrupt(interrupt_hook_callback callback) override
        {
            return nullptr;
            // throw std::runtime_error("Not implemented");
        }

        emulator_hook* hook_memory_violation(uint64_t address, size_t size,
                                             memory_violation_hook_callback callback) override
        {
            return nullptr;
            // throw std::runtime_error("Not implemented");
        }

        emulator_hook* hook_memory_access(const uint64_t address, const size_t size, const memory_operation filter,
                                          complex_memory_hook_callback callback) override
        {
            if (filter == memory_permission::none)
            {
                return nullptr;
            }

            return nullptr;
            // throw std::runtime_error("Not implemented");
        }

        void delete_hook(emulator_hook* hook) override
        {
            const auto id = static_cast<uint32_t>(reinterpret_cast<size_t>(hook));
            const auto entry = this->syscall_hooks_.find(id);
            if (entry == this->syscall_hooks_.end())
            {
                return;
            }

            icicle_remove_syscall_hook(this->emu_, id);
            this->syscall_hooks_.erase(entry);
        }

        void serialize_state(utils::buffer_serializer& buffer, const bool is_snapshot) const override
        {
            // throw std::runtime_error("Not implemented");
        }

        void deserialize_state(utils::buffer_deserializer& buffer, const bool is_snapshot) override
        {
            // throw std::runtime_error("Not implemented");
        }

        std::vector<std::byte> save_registers() override
        {
            // throw std::runtime_error("Not implemented");
            return {};
        }

        void restore_registers(const std::vector<std::byte>& register_data) override
        {
            // throw std::runtime_error("Not implemented");
        }

        bool has_violation() const override
        {
            return false;
        }

      private:
        using syscall_hook_storage = std::unique_ptr<std::function<void()>>;
        std::unordered_map<uint32_t, syscall_hook_storage> syscall_hooks_{};
        icicle_emulator* emu_{};
    };

    std::unique_ptr<x64_emulator> create_x64_emulator()
    {
        return std::make_unique<icicle_x64_emulator>();
    }
}
