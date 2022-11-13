#pragma once

#include "../SerialDevice.h"

#include <thread>
#include <atomic>

#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/StreamSocket.h>

namespace GameboyEmu::DataTransfer {
	class UdpSerial : public SerialDevice {
	public :
		enum class TerminationType {
			internal = 1,
			external = 2
		};

		UdpSerial();

		bool Send(byte& in) override;
		void SetOut(byte out) override;

		void Listen(std::string const& on) override;
		void Connect(std::string const& to) override;
		void CloseConnection() override;

		void SetClockType(byte type) override;

		~UdpSerial() override;

		bool Connected() const override;

		void StartThread();

	private :
		std::atomic_bool m_connected;
		std::atomic_uint8_t m_clock_type;

		std::atomic_uint8_t m_out;

		std::thread m_ex_thread;

		std::string m_ip;
		int m_port;

		Poco::Net::DatagramSocket m_socket_in;
		Poco::Net::SocketAddress m_sa_in;

		Poco::Net::DatagramSocket m_socket_ex;
		Poco::Net::SocketAddress m_sa_ex;

		std::atomic_bool m_stop;
	};
}
