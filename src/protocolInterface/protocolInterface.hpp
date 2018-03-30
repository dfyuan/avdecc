/*
* Copyright (C) 2016-2018, L-Acoustics and its contributors

* This file is part of LA_avdecc.

* LA_avdecc is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* LA_avdecc is distributed in the hope that it will be usefu_state,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General Public License
* along with LA_avdecc.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
* @file protocolInterface.hpp
* @author Christophe Calmejane
*/

#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <list>
#include <mutex>
#include <stdexcept>
#include <functional>
#include "la/avdecc/utils.hpp"
#include "la/avdecc/internals/exception.hpp"
#include "la/avdecc/internals/entity.hpp"
#include "la/avdecc/networkInterfaceHelper.hpp"
#include "protocol/protocolAdpdu.hpp"
#include "protocol/protocolAecpdu.hpp"
#include "protocol/protocolAcmpdu.hpp"

namespace la
{
namespace avdecc
{
namespace protocol
{

class ProtocolInterface : public la::avdecc::Subject<ProtocolInterface, std::recursive_mutex>
{
public:
	enum class Error
	{
		NoError = 0,
		TransportError = 1, /**< Transport interface error. This is critical and the interface is no longer usable. */
		Timeout = 2, /**< A timeout occured during the operation. */
		UnknownRemoteEntity = 3, /**< Unknown remote entity. */
		UnknownLocalEntity = 4, /**< Unknown local entity. */
		InvalidEntityType = 5, /**< Invalid entity type for the operation. */
		DuplicateLocalEntityID = 6, /**< The EntityID specified in a LocalEntity is already in use by another local entity. */
		InterfaceNotFound = 7, /**< Specified interface not found. */
		InterfaceInvalid = 8, /**< Specified interface is invalid. */
		InterfaceNotSupported = 9, /**< This protocol interface is not in the list of supported protocol interfaces. */
		MessageNotSupported = 10, /**< This type of message is not supported by this protocol interface. */
		InternalError = 99, /**< Internal error, please report the issue. */
	};

	class Exception final : public la::avdecc::Exception
	{
	public:
		template<class TextType>
		Exception(Error const error, TextType&& text) noexcept : la::avdecc::Exception(std::forward<TextType>(text)), _error(error) {}
		Error getError() const noexcept { return _error; }
	private:
		Error const _error{ Error::NoError };
	};

	using UniquePointer = std::unique_ptr<ProtocolInterface>;
	using AecpCommandResultHandler = std::function<void(la::avdecc::protocol::Aecpdu const* const response, la::avdecc::protocol::ProtocolInterface::Error const error) noexcept>;
	using AcmpCommandResultHandler = std::function<void(la::avdecc::protocol::Acmpdu const* const response, la::avdecc::protocol::ProtocolInterface::Error const error) noexcept>;

	class Observer : public la::avdecc::Observer<ProtocolInterface>
	{
	public:
		/* **** Global notifications **** */
		virtual void onTransportError(la::avdecc::protocol::ProtocolInterface* const /*pi*/) noexcept = 0;
		/* **** Discovery notifications **** */
		virtual void onLocalEntityOnline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::DiscoveredEntity const& /*entity*/) noexcept {}
		virtual void onLocalEntityOffline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept {}
		virtual void onLocalEntityUpdated(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::DiscoveredEntity const& /*entity*/) noexcept {}
		virtual void onRemoteEntityOnline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::DiscoveredEntity const& /*entity*/) noexcept {}
		virtual void onRemoteEntityOffline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept {}
		virtual void onRemoteEntityUpdated(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::DiscoveredEntity const& /*entity*/) noexcept {}
		/* **** AECP notifications **** */
		/** Notification for when an AECP Command destined to *entity* is received. */
		virtual void onAecpCommand(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::LocalEntity const& /*entity*/, la::avdecc::protocol::Aecpdu const& /*aecpdu*/) noexcept {}
		/** Notification for when an unsolicited AECP Response destined to *entity* is received. */
		virtual void onAecpUnsolicitedResponse(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::LocalEntity const& /*entity*/, la::avdecc::protocol::Aecpdu const& /*aecpdu*/) noexcept {}
		/* **** ACMP notifications **** */
		/** Notification for when a sniffed ACMP Command is received (Not destined to *entity*). */
		virtual void onAcmpSniffedCommand(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::LocalEntity const& /*entity*/, la::avdecc::protocol::Acmpdu const& /*acmpdu*/) noexcept {}
		/** Notification for when a sniffed ACMP Response is received (Not destined to *entity*). */
		virtual void onAcmpSniffedResponse(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::LocalEntity const& /*entity*/, la::avdecc::protocol::Acmpdu const& /*acmpdu*/) noexcept {}
	};

	virtual ~ProtocolInterface() noexcept = default;

	/** Returns the Mac Address associated with the network interface name */
	networkInterface::MacAddress const& getMacAddress() const noexcept;

	/** Returns the interface index associated with the network interface name */
	std::uint16_t getInterfaceIndex() const noexcept;

	// Virtual interface
	/** Shuts down the interface, stopping all active communications. This method blocks the current thread until all pending messages are processed. This is automatically called during destructor. */
	virtual void shutdown() noexcept = 0;
	/** Registers a local entity to the interface, allowing it to send and receive messages. */
	virtual Error registerLocalEntity(entity::LocalEntity& entity) noexcept = 0;
	/** Unregisters a local entity from the interface. It won't be able to send or receive messages anymore. */
	virtual Error unregisterLocalEntity(entity::LocalEntity& entity) noexcept = 0;
	/** Enables entity advertising on the network. */
	virtual Error enableEntityAdvertising(entity::LocalEntity const& entity) noexcept = 0;
	/** Disables entity advertising on the network. */
	virtual Error disableEntityAdvertising(entity::LocalEntity& entity) noexcept = 0;
	/** Requests a remote entities discovery. */
	virtual Error discoverRemoteEntities() const noexcept = 0;
	/** Requests a targetted remote entity discovery. */
	virtual Error discoverRemoteEntity(UniqueIdentifier const entityID) const noexcept = 0;
	/** Sends an ADP message directly on the network (not supported by all kinds of ProtocolInterface). */
	virtual Error sendAdpMessage(Adpdu::UniquePointer&& adpdu) const noexcept = 0;
	/** Sends an AECP message directly on the network (not supported by all kinds of ProtocolInterface). */
	virtual Error sendAecpMessage(Aecpdu::UniquePointer&& aecpdu) const noexcept = 0;
	/** Sends an ACMP message directly on the network (not supported by all kinds of ProtocolInterface). */
	virtual Error sendAcmpMessage(Acmpdu::UniquePointer&& acmpdu) const noexcept = 0;
	/** Sends an AECP command message. */
	virtual Error sendAecpCommand(Aecpdu::UniquePointer&& aecpdu, networkInterface::MacAddress const& macAddress, AecpCommandResultHandler const& onResult) const noexcept = 0;
	/** Sends an AECP response message. */
	virtual Error sendAecpResponse(Aecpdu::UniquePointer&& aecpdu, networkInterface::MacAddress const& macAddress) const noexcept = 0;
	/** Sends an ACMP command message. */
	virtual Error sendAcmpCommand(Acmpdu::UniquePointer&& acmpdu, AcmpCommandResultHandler const& onResult) const noexcept = 0;
	/** Sends an ACMP response message. */
	virtual Error sendAcmpResponse(Acmpdu::UniquePointer&& acmpdu) const noexcept = 0;

	/** BasicLockable concept 'lock' method for the whole ProtocolInterface */
	virtual void lock() noexcept = 0;
	/** BasicLockable concept 'unlock' method for the whole ProtocolInterface */
	virtual void unlock() noexcept = 0;

	// Deleted compiler auto-generated methods
	ProtocolInterface(ProtocolInterface&&) = delete;
	ProtocolInterface(ProtocolInterface const&) = delete;
	ProtocolInterface& operator=(ProtocolInterface const&) = delete;
	ProtocolInterface& operator=(ProtocolInterface&&) = delete;

protected:
	/**
	* @brief Create a ProtocolInterface associated with specified network interface name.
	* @details Create a ProtocolInterface associated with specified network interface name, checking the interface actually exists.
	* @param[in] networkInterfaceName The name of the network interface.
	* @note Throws Exception if networkInterfaceName is invalid or inaccessible.
	*/
	ProtocolInterface(std::string const& networkInterfaceName);

	/**
	* @brief Create a ProtocolInterface associated with specified network interface name and MAC address.
	* @details Create a ProtocolInterface associated with specified network interface name and MAC address, not checking if the interface exists.
	* @param[in] networkInterfaceName The name of the network interface.
	* @param[in] macAddress The MAC address associated with the network interface. Cannot be all 0.
	* @note Throws Exception if networkInterfaceName or macAddress is invalid or inaccessible.
	*/
	ProtocolInterface(std::string const& networkInterfaceName, networkInterface::MacAddress const& macAddress);

	std::string const _networkInterfaceName{};

private:
	networkInterface::MacAddress _networkInterfaceMacAddress{};
	std::uint16_t _interfaceIndex{};
};

/* Operator overloads */
constexpr bool operator!(ProtocolInterface::Error const error)
{
	return error == ProtocolInterface::Error::NoError;
}

constexpr ProtocolInterface::Error& operator|=(ProtocolInterface::Error& lhs, ProtocolInterface::Error const rhs)
{
	lhs = static_cast<ProtocolInterface::Error>(static_cast<std::underlying_type_t<ProtocolInterface::Error>>(lhs) | static_cast<std::underlying_type_t<ProtocolInterface::Error>>(rhs));
	return lhs;
}

} // namespace protocol
} // namespace avdecc
} // namespace la
