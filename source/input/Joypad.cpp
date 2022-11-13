#include "../../include/input/Joypad.h"

#include "../../include/memory/Memory.h"

namespace GameboyEmu::Input {
	Joypad::Joypad() : m_mutex(),
		m_ctx{}, m_mem(nullptr) {
		m_ctx.m_select_actions = 1;
		m_ctx.m_select_direction = 1;

		m_ctx.m_status[0][0] = 0x1;
		m_ctx.m_status[0][1] = 0x1;
		m_ctx.m_status[0][2] = 0x1;
		m_ctx.m_status[0][3] = 0x1;

		m_ctx.m_status[1][0] = 0x1;
		m_ctx.m_status[1][1] = 0x1;
		m_ctx.m_status[1][2] = 0x1;
		m_ctx.m_status[1][3] = 0x1;
	}

	void Joypad::Write(byte val) {
		byte action = GET_BIT(val, 5);
		byte direction = GET_BIT(val, 4);

		std::scoped_lock<std::mutex> lk(m_mutex);

		m_ctx.m_select_actions = action;
		m_ctx.m_select_direction = direction;
	}

	byte Joypad::Read() {
		std::scoped_lock<std::mutex> lk(m_mutex);

		byte* correct_table = nullptr;

		if (m_ctx.m_select_actions
			&& m_ctx.m_select_direction) {
			return 0xFF;
		}

		//actions (Start, ...) are in the
		//first table
		if (!m_ctx.m_select_actions) {
			correct_table = m_ctx.m_status[0];
		}

		if(!m_ctx.m_select_direction) {
			correct_table = m_ctx.m_status[1];
		} 

		byte ret = 0;

		ret |= (m_ctx.m_select_actions << 5);
		ret |= (m_ctx.m_select_direction << 4);

		ret |= (correct_table[3] << 3);
		ret |= (correct_table[2] << 2);
		ret |= (correct_table[1] << 1);
		ret |= correct_table[0];
		
		return ret;
	}

	void Joypad::RequestInterrupt() {
		byte ir = m_mem->Read(0xFF0F);

		JOYPAD_BIT_SET(ir);

		m_mem->Write(0xFF0F, ir);
	}

	void Joypad::SetMemory(Mem::Memory* mem) {
		m_mem = mem;
	}

#define LOCK std::scoped_lock<std::mutex> lk(m_mutex);


	/// ////

	void Joypad::SetStart() {
		LOCK;

		m_ctx.m_status[0][3] = 0x0;

		RequestInterrupt();
	}

	void Joypad::UnsetStart() {
		LOCK;

		m_ctx.m_status[0][3] = 0x1;
	}

	void Joypad::SetSelect() {
		LOCK;

		m_ctx.m_status[0][2] = 0x0;

		RequestInterrupt();
	}

	void Joypad::UnsetSelect() {
		LOCK;

		m_ctx.m_status[0][2] = 0x1;
	}

	void Joypad::SetB() {
		LOCK;

		m_ctx.m_status[0][1] = 0x0;

		RequestInterrupt();
	}

	void Joypad::UnsetB() {
		LOCK;

		m_ctx.m_status[0][1] = 0x1;
	}

	void Joypad::SetA() {
		LOCK;

		m_ctx.m_status[0][0] = 0x0;

		RequestInterrupt();
	}

	void Joypad::UnsetA() {
		LOCK;

		m_ctx.m_status[0][0] = 0x1;
	}

	/// ////

	void Joypad::SetDown() {
		LOCK;

		m_ctx.m_status[1][3] = 0x0;

		RequestInterrupt();
	}

	void Joypad::UnsetDown() {
		LOCK;

		m_ctx.m_status[1][3] = 0x1;
	}

	void Joypad::SetUp() {
		LOCK;

		m_ctx.m_status[1][2] = 0x0;

		RequestInterrupt();
	}

	void Joypad::UnsetUp() {
		LOCK;

		m_ctx.m_status[1][2] = 0x1;
	}

	void Joypad::SetLeft() {
		LOCK;

		m_ctx.m_status[1][1] = 0x0;

		RequestInterrupt();
	}

	void Joypad::UnsetLeft() {
		LOCK;

		m_ctx.m_status[1][1] = 0x1;
	}

	void Joypad::SetRight() {
		LOCK;

		m_ctx.m_status[1][0] = 0x0;

		RequestInterrupt();
	}

	void Joypad::UnsetRight() {
		LOCK;

		m_ctx.m_status[1][0] = 0x1;
	}

	/// ////
}