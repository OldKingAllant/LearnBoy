#include "../../../include/datatransfer/out/UdpSerial.h"

#include "../../../include/datatransfer/Serial.h"

#include <Poco/Net/NetException.h>

namespace GameboyEmu::DataTransfer {
	UdpSerial::UdpSerial() :
		m_connected(false),
		m_clock_type(0), m_out(),
		m_ex_thread(), m_ip(""),
		m_port(-1), m_socket_in(), 
		m_socket_ex(),
		m_sa_in(), m_sa_ex(),
		m_stop(false)
	{}

	void UdpSerial::SetOut(byte out) {
		m_out = out;
	}

	void UdpSerial::StartThread() {
		m_stop.store(false);

		m_ex_thread = std::thread([this]() {
			while (!m_stop) {
				char data[16] = {};

				int numbytes = m_socket_ex.receiveFrom(data, 16, m_sa_in);

				if (numbytes != 16 || data[1] != 0) {
					TerminationType term = (TerminationType)
						data[1];

					if (term == TerminationType::internal) {
						data[1] = 2;
						m_socket_ex.sendTo(data, 16, m_sa_ex);
					}
					else {
						m_stop.store(true);

						int port = m_socket_in.address().port();

						Poco::Net::SocketAddress sa("127.0.0.1", port);

						data[1] = 1;

						m_socket_ex.sendTo(data, 16, sa);
					}
				}
				else {
					if (!m_stop) {
						byte thbyte = data[0];

						data[0] = m_out.load();

						m_socket_ex.sendTo(data, 16, m_sa_in);

						m_serial->RequestInterrupt(thbyte);
					}
				}
			}
		});
	}

	bool UdpSerial::Send(byte& in) {
		if (!m_connected)
			return false;

		if (m_stop) {
			m_connected = false;
			CloseConnection();
			return false;
		}
		
		char data[16] = {};

		bool ret = true;

		if (m_clock_type == 1) {
			data[0] = m_out.load();

			m_socket_in.sendTo(data, 16, m_sa_ex);

			m_socket_in.receiveFrom(data, 16, m_sa_ex);

			if (data[1] != 0) {
				ret = false;
				CloseConnection();
			}
			else {
				in = data[0];
			}

		}

		return ret;
	}

	void UdpSerial::Listen(std::string const& on) {
		if (m_stop) {
			CloseConnection();
		}

		if (m_connected)
			return;

		bool r = true;
		int port = 0;

		try {
			port = std::stoi(on);
		}
		catch (std::runtime_error const&) {
			r = false;
		}

		if (!r)
			return;

		Poco::Net::SocketAddress sa(port);

		m_connected = true;

		try {
			//Bind main socket waiting for
			//connection
			m_socket_in.bind(sa);

			//m_socket_in.setBlocking(true);

			byte test = 0;

			//Wait for signature flag
			m_socket_in.receiveFrom(&test, 1, m_sa_in);

			if (test != 111) {
				m_connected = false;
			}
			else {
				//Send data from second
				//socket for external clock
				//and wait for response
				m_socket_ex.sendTo(&test, 1, m_sa_in);

				//Receive data from second socket
				m_socket_ex.receiveFrom(&test, 1, m_sa_ex);

				this->StartThread();
			}
		}
		catch (Poco::Net::NetException const&) {
			m_connected = false;
		}
	}

	void UdpSerial::Connect(std::string const& to) {
		if (m_stop) {
			CloseConnection();
		}

		if (m_connected)
			return;

		auto colon = to.find(':');

		if (colon == std::string::npos)
			return;

		std::string address = to.substr(0, colon);
		std::string port_s = to.substr(colon + 1, to.size() - colon - 1);
		
		bool r = true;
		int port = 0;

		try {
			port = std::stoi(port_s);
		}
		catch (std::runtime_error const&) {
			r = false;
		}

		if (!r)
			return;
		

		Poco::Net::IPAddress addr;

		r = Poco::Net::IPAddress::tryParse(address, addr);

		if (!r)
			return;

		Poco::Net::SocketAddress sa(address, port);

		m_connected = true;

		try {
			//m_socket_in.connect(sa);
			m_sa_in = sa;

			//m_socket_in.setBlocking(true);

			byte test = 111;

			m_socket_in.sendTo(&test, 1, m_sa_in);

			m_socket_in.receiveFrom(&test, 1, m_sa_ex);

			m_socket_ex.sendTo(&test, 1, m_sa_ex);

			this->StartThread();
		}
		catch (Poco::Net::NetException const&) {
			m_connected = false;
		}
	}

	void UdpSerial::CloseConnection() {
		if (m_connected) {
			m_stop.store(true);

			auto port = m_socket_ex.address().port();
			Poco::Net::SocketAddress sa("127.0.0.1", port);

			char data[16] = { 0, 1 };

			m_socket_in.sendTo(data, 16, sa);
		}
		
		if (m_ex_thread.joinable())
			m_ex_thread.join();

		if (m_connected) {
			m_connected.store(false);

			m_socket_in.close();
			m_socket_ex.close();
		}
	}

	void UdpSerial::SetClockType(byte type) {
		m_clock_type = type;
	}

	bool UdpSerial::Connected() const {
		return m_connected;
	}

	UdpSerial::~UdpSerial() {
		CloseConnection();
	}
}