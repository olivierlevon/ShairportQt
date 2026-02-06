#pragma once

#include <string>
#include <cstdint>

namespace Networking
{
#ifdef _WIN32
	using socket_t = uintptr_t;	// matches SOCKET (UINT_PTR) on Windows
#else
	using socket_t = int;
#endif

	constexpr socket_t invalid_socket = ~socket_t{ 0 };

	void		Init();
	socket_t	CreateSocket(bool stream = true, bool bV4 = true) noexcept;
	void		DestroySocket(socket_t sd) noexcept;
	bool		SetSocketBlockingEnabled(socket_t sd, bool blocking) noexcept;
	int			WaitForIncomingData(socket_t sd, unsigned int ms = 0xffffffff) noexcept;
	int			Read(socket_t sd, unsigned char* buffer, unsigned int size) noexcept;
	std::string GetPeerIP(socket_t sd) noexcept;

} // namespace Networking
