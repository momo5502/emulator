#pragma once
#include "emulator_utils.hpp"
#include "handles.hpp"

#include "module/module_manager.hpp"
#include <utils/nt_handle.hpp>

#include <x64_emulator.hpp>

struct event
{
	bool signaled{};
	EVENT_TYPE type{};

	bool is_signaled()
	{
		const auto res = this->signaled;

		if (this->type == SynchronizationEvent)
		{
			this->signaled = false;
		}

		return res;
	}

	void serialize(utils::buffer_serializer& buffer) const
	{
		buffer.write(this->signaled);
		buffer.write(this->type);
	}

	void deserialize(utils::buffer_deserializer& buffer)
	{
		buffer.read(this->signaled);
		buffer.read(this->type);
	}
};

struct file
{
	utils::nt::handle<INVALID_HANDLE_VALUE> handle{};
	std::wstring name{};

	void serialize(utils::buffer_serializer& buffer) const
	{
		buffer.write(this->name);
		// TODO: Serialize handle
	}

	void deserialize(utils::buffer_deserializer& buffer)
	{
		buffer.read(this->name);
		this->handle = INVALID_HANDLE_VALUE;
	}
};

struct semaphore
{
	std::wstring name{};
	volatile uint32_t current_count{};
	uint32_t max_count{};

	void serialize(utils::buffer_serializer& buffer) const
	{
		buffer.write(this->name);
		buffer.write(this->current_count);
		buffer.write(this->max_count);
	}

	void deserialize(utils::buffer_deserializer& buffer)
	{
		buffer.read(this->name);
		buffer.read(this->current_count);
		buffer.read(this->max_count);
	}
};

struct process_context
{
	process_context(x64_emulator& emu)
		: emu(&emu)
		  , teb(emu)
		  , peb(emu)
		  , process_params(emu)
		  , kusd(emu)
		  , module_manager(emu)
		  , gs_segment(emu)
	{
	}

	x64_emulator* emu{};
	uint64_t executed_instructions{0};
	emulator_object<TEB> teb;
	emulator_object<PEB> peb;
	emulator_object<RTL_USER_PROCESS_PARAMETERS> process_params;
	emulator_object<KUSER_SHARED_DATA> kusd;

	module_manager module_manager;

	mapped_module* executable{};
	mapped_module* ntdll{};
	mapped_module* win32u{};

	uint64_t shared_section_size{};

	handle_store<handle_types::event, event> events{};
	handle_store<handle_types::file, file> files{};
	handle_store<handle_types::semaphore, semaphore> semaphores{};
	std::map<uint16_t, std::wstring> atoms{};
	emulator_allocator gs_segment;

	bool verbose{false};

	void serialize(utils::buffer_serializer& buffer) const
	{
		buffer.write(this->executed_instructions);
		buffer.write(this->teb);
		buffer.write(this->peb);
		buffer.write(this->process_params);
		buffer.write(this->kusd);
		buffer.write(this->module_manager);

		buffer.write(this->executable->image_base);
		buffer.write(this->ntdll->image_base);
		buffer.write(this->win32u->image_base);

		buffer.write(this->shared_section_size);
		buffer.write(this->events);
		buffer.write(this->files);
		buffer.write(this->semaphores);
		buffer.write_map(this->atoms);
		buffer.write(this->gs_segment);
	}

	void deserialize(utils::buffer_deserializer& buffer)
	{
		buffer.read(this->executed_instructions);
		buffer.read(this->teb);
		buffer.read(this->peb);
		buffer.read(this->process_params);
		buffer.read(this->kusd);
		buffer.read(this->module_manager);

		const auto executable_base = buffer.read<uint64_t>();
		const auto ntdll_base = buffer.read<uint64_t>();
		const auto win32u_base = buffer.read<uint64_t>();

		this->executable = this->module_manager.find_by_address(executable_base);
		this->ntdll = this->module_manager.find_by_address(ntdll_base);
		this->win32u = this->module_manager.find_by_address(win32u_base);

		buffer.read(this->shared_section_size);
		buffer.read(this->events);
		buffer.read(this->files);
		buffer.read(this->semaphores);
		buffer.read_map(this->atoms);
		buffer.read(this->gs_segment);
	}
};
