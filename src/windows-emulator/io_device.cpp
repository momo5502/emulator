#include "io_device.hpp"
#include "devices/afd_endpoint.hpp"

namespace
{
	template <typename Traits>
	struct dummy_device : stateless_device<Traits>
	{
		NTSTATUS io_control(windows_emulator&, const io_device_context<Traits>&) override
		{
			return STATUS_SUCCESS;
		}
	};
}

std::unique_ptr<io_device<EmulatorTraits<Emu64>>> create_device64(const std::u16string_view device)
{
	if (device == u"CNG"
		|| device == u"KsecDD"
		|| device == u"DeviceApi\\CMApi"
		|| device == u"ConDrv\\Server")
	{
		return std::make_unique<dummy_device<EmulatorTraits<Emu64>>>();
	}

	if (device == u"Afd\\Endpoint")
	{
		return create_afd_endpoint64();
	}

	throw std::runtime_error("Unsupported device: " + std::string(device.begin(), device.end()));
}
