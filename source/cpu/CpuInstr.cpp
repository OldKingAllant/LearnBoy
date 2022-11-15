#include "../../include/cpu/CpuInstr.h"
#include "../../include/cpu/Disasm.h"
#include "../../include/state/EmulatorState.h"

namespace normalInstructions = GameboyEmu::CPU::normalInstructions;
namespace cbInstructions = GameboyEmu::CPU::cbInstructions;

#define LD16_REG(reg) \
    word value = mem->Read(ctx.ip++); \
    state->Sync(1); \
    value |= (mem->Read(ctx.ip++) << 8); \
    state->Sync(1); \
    reg = value;

#define LD8_REG_H(reg) \
    byte value = mem->Read(ctx.ip++); \
    state->Sync(1); \
    reg = (value << 8) | (reg & 0xFF);

#define LD_A16_8(reg) \
    word address = mem->Read(ctx.ip++); \
    state->Sync(1); \
    address |= (mem->Read(ctx.ip++) << 8); \
    state->Sync(1); \
    mem->Write(address, reg); \
    state->Sync(1);

#define LDH_A8_REG(reg) \
    byte address = mem->Read(ctx.ip++); \
    state->Sync(1); \
    mem->Write(0xFF00 + address, reg); \
    state->Sync(1);

#define LDH_REG_A8(reg) \
    byte address = mem->Read(ctx.ip++); \
    state->Sync(1); \
    byte val = mem->Read(0xFF00 + address); \
	state->Sync(1); \
    reg = (val << 8) | (reg & 0xFF);

#define LD8H_REG_MEM(reg, from) \
	byte low = mem->Read(from); \
    state->Sync(1); \
    reg = (low << 8) | (reg & 0xFF);

#define LD8L_REG_MEM(reg) \
	byte low = mem->Read(ctx.ip++); \
	state->Sync(1); \
	reg = (reg & 0xFF00) | low;

#define LD8_MEM_REG(to, from) \
	mem->Write(to, from); \
	state->Sync(1);

#define LDL_REG_MEM(to, from) \
	byte low = mem->Read(from); \
    state->Sync(1); \
    to = (to & 0xFF00) | low;

#define PUSH16_REG(reg) \
	mem->Write(ctx.sp - 1, (reg >> 8) & 0xFF); \
    state->Sync(1); \
	mem->Write(ctx.sp - 2, reg & 0xFF); \
    state->Sync(1); \
	ctx.sp -= 2; \
    state->Sync(1);

#define POP16_REG(reg) \
    reg = mem->Read(ctx.sp); \
    state->Sync(1); \
    reg |= (mem->Read(ctx.sp + 1) << 8); \
    state->Sync(1); \
    ctx.sp += 2;

#define OR8_REG(reg) \
    byte res = ((ctx.af >> 8) & 0xFF) | reg; \
    byte z = (res == 0); \
    ctx.af = (res << 8) | (ctx.af & 0xFF);

#define LDH_REG_REG(to, from) \
	mem->Write(0xFF00 + to, from); \
	state->Sync(1); 

#define INCL(reg) \
	byte low = reg & 0xFF; \
	byte carry = ((ctx.af & 0xFF) >> 4) & 1; \
	byte h = ((byte)(low & 0xF) + 1) > 0xF; \
	low++; \
	byte zero = (low == 0); \
    reg = (reg & 0xFF00) | low;

#define LD_HL_REG(reg) \
	mem->Write(ctx.hl, reg); \
	state->Sync(1); 

#define CP(value) \
	byte a = (ctx.af & 0xFF00) >> 8; \
	byte res = a - value; \
	byte z = (res == 0); \
	byte c = (value > a); \
	byte h = ((int)(a & 0xF) \
	- (int)(val & 0xF)) < 0;

//Z 0 H C
#define ADD(value) \
	byte a = (ctx.af & 0xFF00) >> 8; \
	byte res = a + value; \
	byte z = (res == 0); \
	byte h = ((byte)(a & 0xF) + (byte)(value & 0xF)) > 0xF; \
	byte c = ((unsigned)a + (unsigned)value) > 0xFF; \
	ctx.af = (res << 8) | (ctx.af & 0xFF);

#define AND(value) \
	byte a = (ctx.af & 0xFF00) >> 8; \
	byte res = a & value; \
	byte z = (res == 0); \
	ctx.af = (res << 8) | (ctx.af & 0xFF);

#define XOR(value) \
	byte a = (ctx.af & 0xFF00) >> 8; \
	byte res = a ^ value; \
	byte z = (res == 0); \
	ctx.af = (res << 8) | (ctx.af & 0xFF);

#define ADC(value) \
	byte a = (ctx.af & 0xFF00) >> 8; \
	byte carry = (ctx.af >> 4) & 1; \
	byte res = a + value + carry; \
	byte z = (res == 0); \
	byte h = ((byte)(a & 0xF) + (byte)(value & 0xF) + carry) > 0xF; \
	byte c = ((unsigned)a + (unsigned)value + carry) > 0xFF; \
	ctx.af = (res << 8) | (ctx.af & 0xFF);



inline void stacktrace_push(
	GameboyEmu::State::EmulatorState* state,
	word callee, word dest, word ret
) {
	if (
		state->IsDebugging()
		&& state->StacktraceEnabled()) {
		
		byte page = 0;

		if (callee <= 0x7FFF) {
			page = state->GetCard()
				->GetCurrentBank(callee);
		}

		state->add_stacktrace_entry(
			callee, dest, ret, page
		);
	}
}

inline void stacktrace_pop(
	GameboyEmu::State::EmulatorState* state,
	word ret
) {
	if (
		state->IsDebugging()
		&& state->StacktraceEnabled()) {
		state->remove_stacktrace_entry(ret);
	}
}

INSTRUCTION(00, {
	return normalInstructions::cycles[0x00];
});


INSTRUCTION(C3, {
	word newip = 0;
	
	newip = mem->Read(ctx.ip++);

	state->Sync(1);

	newip |= (mem->Read(ctx.ip) << 8);

	state->Sync(1);

	ctx.ip = newip;

	state->Sync(1);

	return normalInstructions::cycles[0xC3];
});

INSTRUCTION(AF, {
byte a = GET_HIGH(ctx.af);

a ^= a;

SET_HIGH(ctx.af, a);

byte zero_flag = (a == 0);

SET_FLAGS(zero_flag, 0, 0, 0, ctx.af);

return normalInstructions::cycles[0xAF];
});

INSTRUCTION(21, {
	LD16_REG(ctx.hl)
	return normalInstructions::cycles[0x21];
});

INSTRUCTION(F3, {
	ctx.enableInt = false;

	if (ctx.ei_delay > 0) {
		ctx.ei_delay = 0;
	}

	return normalInstructions::cycles[0xF3];
});

INSTRUCTION(31, {
	LD16_REG(ctx.sp);
return normalInstructions::cycles[0x31];
});

INSTRUCTION(EA, {
	LD_A16_8(GET_HIGH(ctx.af));
return normalInstructions::cycles[0xEA];
});

INSTRUCTION(3E, {
	LD8_REG_H(ctx.af);
return normalInstructions::cycles[0x3E];
});

INSTRUCTION(E0, {
	LDH_A8_REG(GET_HIGH(ctx.af));
return normalInstructions::cycles[0xE0];
});

INSTRUCTION(CD, {
	word newip = mem->Read(ctx.ip++);

	state->Sync(1);

	newip |= (mem->Read(ctx.ip++) << 8);

	state->Sync(1);

	word ipcopy = ctx.ip;

	mem->Write(ctx.sp - 1, GET_HIGH(ipcopy));

	state->Sync(1);

	mem->Write(ctx.sp - 2, GET_LOW(ipcopy));

	state->Sync(1);

	ctx.sp -= 2;

	ctx.ip = newip;

	state->Sync(1);

	if (state->IsDebugging() && 
		state->StacktraceEnabled()) {
		stacktrace_push(
			state, ipcopy - 3, newip,
			ipcopy
		);
	}

	return normalInstructions::cycles[0xCD];
});

INSTRUCTION(7D, {
	SET_HIGH(ctx.af, GET_LOW(ctx.hl));
return normalInstructions::cycles[0x7D];
});

INSTRUCTION(7C, {
	SET_HIGH(ctx.af, GET_HIGH(ctx.hl));
return normalInstructions::cycles[0x7C];
});

INSTRUCTION(18, {
	signed char diff = (signed char)mem->Read(ctx.ip++);
	state->Sync(1);
	ctx.ip = (signed short)ctx.ip + diff;
	state->Sync(1);

	return normalInstructions::cycles[0x18];
});

INSTRUCTION(C9, {
	word oldip = mem->Read(ctx.sp);
	state->Sync(1);
	oldip |= (mem->Read(ctx.sp + 1) << 8);
	state->Sync(1);
	ctx.ip = oldip;
	state->Sync(1);

	ctx.sp += 2;

	if (state->IsDebugging() &&
		state->StacktraceEnabled()) {
		stacktrace_pop(state, oldip);
	}

	return normalInstructions::cycles[0xC9];
});

INSTRUCTION(E5, {
	PUSH16_REG(ctx.hl);

	return normalInstructions::cycles[0xE5];
});

INSTRUCTION(E1, {
	POP16_REG(ctx.hl);

return normalInstructions::cycles[0xE1];
});

INSTRUCTION(F5, {
	PUSH16_REG(ctx.af);

return normalInstructions::cycles[0xF5];
});

INSTRUCTION(23, {
	ctx.hl++;

	state->Sync(1);

return normalInstructions::cycles[0x23];
});

INSTRUCTION(2A, {
	LD8H_REG_MEM(ctx.af, ctx.hl);

	ctx.hl++;

	return normalInstructions::cycles[0x2A];
});

INSTRUCTION(F1, {
	POP16_REG(ctx.af);

ctx.af = (ctx.af & 0xF0) | (ctx.af & 0xFF00);

return normalInstructions::cycles[0xF1];
});

INSTRUCTION(C5, {
	PUSH16_REG(ctx.bc);

return normalInstructions::cycles[0xC5];
});

INSTRUCTION(01, {
	LD16_REG(ctx.bc);

return normalInstructions::cycles[0x01];
});

INSTRUCTION(03, {
	ctx.bc++;

	state->Sync(1);

return normalInstructions::cycles[0x03];
});

INSTRUCTION(78, {
	SET_HIGH(ctx.af, GET_HIGH(ctx.bc));

	return normalInstructions::cycles[0x78];
});

INSTRUCTION(B1, {
	OR8_REG(GET_LOW(ctx.bc));

	SET_FLAGS(z, 0, 0, 0, ctx.af);
	return normalInstructions::cycles[0xB1];
});

INSTRUCTION(28, {
	byte total_cycles = normalInstructions::cycles[0x28];

	signed char diff = (signed char)mem->Read(ctx.ip++);
	state->Sync(1);

	if (ZERO_FLAG(ctx.af)) {
		total_cycles += 4;
		ctx.ip = ctx.ip + diff;
		state->Sync(1);
	}

	return total_cycles;
});

INSTRUCTION(F0, {
	LDH_REG_A8(ctx.af);

return normalInstructions::cycles[0xF0];
});

INSTRUCTION(FE, {
	byte val = mem->Read(ctx.ip++);

	state->Sync(1);

	CP(val);

	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0xFE];
});

INSTRUCTION(20, {
	byte total_cycles = normalInstructions::cycles[0x20];

	signed char diff = (signed char)mem->Read(ctx.ip++);
	state->Sync(1);

	if ((ZERO_FLAG(ctx.af)) == 0) {
		total_cycles += 4;
		ctx.ip = ctx.ip + diff;
		state->Sync(1);
	}

	return total_cycles;
});

INSTRUCTION(C1, {
	POP16_REG(ctx.bc);

return normalInstructions::cycles[0xC1];
});

INSTRUCTION(32, {
	LD8_MEM_REG(ctx.hl, GET_HIGH(ctx.af));

	ctx.hl--;

	return normalInstructions::cycles[0x32];
});

INSTRUCTION(0E, {
	LD8L_REG_MEM(ctx.bc);

return normalInstructions::cycles[0x0E];
});

INSTRUCTION(E2, {
	LDH_REG_REG(GET_LOW(ctx.bc), GET_HIGH(ctx.af));

return normalInstructions::cycles[0xE2];
});

INSTRUCTION(0C, {
	INCL(ctx.bc);

	SET_FLAGS(zero, 0, h, carry, ctx.af);

	return normalInstructions::cycles[0x0C];
});

INSTRUCTION(77, {
	LD_HL_REG(GET_HIGH(ctx.af));

	return normalInstructions::cycles[0x77];
});

INSTRUCTION(11, {
	LD16_REG(ctx.de);

return normalInstructions::cycles[0x11];
});

INSTRUCTION(1A, {
	LD8H_REG_MEM(ctx.af, ctx.de);

	return normalInstructions::cycles[0x1A];
});

INSTRUCTION(13, {
	ctx.de++;

state->Sync(1);

return normalInstructions::cycles[0x13];
});

INSTRUCTION(7B, {
	SET_HIGH(ctx.af, GET_LOW(ctx.de));

return normalInstructions::cycles[0x78];
});

INSTRUCTION(4F, {
	SET_LOW(ctx.bc, GET_HIGH(ctx.af));

	return normalInstructions::cycles[0x4F];
});

INSTRUCTION(06, {
	LD8_REG_H(ctx.bc);

return normalInstructions::cycles[0x06];
});

#define DECH(reg) \
	byte high = (reg & 0xFF00) >> 8; \
	byte c = ((ctx.af & 0xFF) >> 4) & 1; \
	byte h = ((byte)((high - 1) & 0xF) == 0xF); \
	high--; \
	reg = (high << 8) | (reg & 0xFF); \
	byte z = (high == 0); 

#define DECL(reg) \
	byte low = (reg & 0xFF); \
	byte c = ((ctx.af & 0xFF) >> 4) & 1; \
	byte h = ((byte)((low - 1) & 0xF) == 0xF); \
	low--; \
	reg = (reg & 0xFF00) | low; \
	byte z = (low == 0); 

#define INCH(reg) \
	byte high = (reg & 0xFF00) >> 8; \
	byte carry = ((ctx.af & 0xFF) >> 4) & 1; \
	byte h = ((byte)(high & 0xF) + 1) > 0xF; \
	high++; \
	byte zero = (high == 0); \
    reg = (high << 8) | (reg & 0xFF);

INSTRUCTION(05, {
	DECH(ctx.bc);

	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x05];
});

INSTRUCTION(22, {
	LD_HL_REG(GET_HIGH(ctx.af));

ctx.hl++;

return normalInstructions::cycles[0x22];
});

INSTRUCTION(3D, {
	DECH(ctx.af);

	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x05];
});

INSTRUCTION(0D, {
	DECL(ctx.bc);

SET_FLAGS(z, 1, h, c, ctx.af);

return normalInstructions::cycles[0x0D];
});

INSTRUCTION(2E, {
	LD8L_REG_MEM(ctx.hl);

	return normalInstructions::cycles[0x2E];
});

INSTRUCTION(67, {
	SET_HIGH(ctx.hl, GET_HIGH(ctx.af));

return normalInstructions::cycles[0x67];
});


INSTRUCTION(57, {
	SET_HIGH(ctx.de, GET_HIGH(ctx.af));

return normalInstructions::cycles[0x57];
});

INSTRUCTION(04, {
	INCH(ctx.bc);

SET_FLAGS(zero, 0, h, carry, ctx.af);

return normalInstructions::cycles[0x04];
});

INSTRUCTION(1E, {
	LD8L_REG_MEM(ctx.de);

return normalInstructions::cycles[0x1E];
});

INSTRUCTION(1D, {
	DECL(ctx.de);

	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x1D];
});

INSTRUCTION(24, {
	INCH(ctx.hl);

SET_FLAGS(zero, 0, h, carry, ctx.af);

return normalInstructions::cycles[0x24];
});

#define SUB(reg) \
	byte a = (ctx.af >> 8) & 0xFF; \
	byte z = (a - reg) == 0; \
	byte h = ((reg & 0xF) > (a & 0xF)); \
	byte c = (reg > a); \
	a -= reg; \
	ctx.af = (a << 8) | (a & 0xFF);

INSTRUCTION(90, {
	SUB(GET_HIGH(ctx.bc));

SET_FLAGS(z, 1, h, c, ctx.af);

return normalInstructions::cycles[0x90];
});

INSTRUCTION(15, {
	DECH(ctx.de);

SET_FLAGS(z, 1, h, c, ctx.af);

return normalInstructions::cycles[0x15];
});

INSTRUCTION(16, {
	LD8_REG_H(ctx.de);

	return normalInstructions::cycles[0x16];
});

INSTRUCTION(BE, {
	byte val = mem->Read(ctx.hl);

	state->Sync(1);

	CP(val);

	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0xBE];
});

INSTRUCTION(86, {
	byte val = mem->Read(ctx.hl);

	state->Sync(1);

	ADD(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x86];
});

INSTRUCTION(FA, {
	byte lowb = mem->Read(ctx.ip++);

	state->Sync(1);

	byte high = mem->Read(ctx.ip++);

	state->Sync(1);

	word address = (high << 8) | lowb;

	LD8H_REG_MEM(ctx.af, address);

	return normalInstructions::cycles[0xFA];
});

INSTRUCTION(E6, {
	byte val = mem->Read(ctx.ip++);

	state->Sync(1);

	AND(val);

	SET_FLAGS(z, 0, 1, 0, ctx.af);

	return normalInstructions::cycles[0xE6];
});

INSTRUCTION(C4, {
	byte normalCycles = normalInstructions::cycles[0xC4];

	word newip = mem->Read(ctx.ip++);

	state->Sync(1);

	newip |= (mem->Read(ctx.ip++) << 8);

	state->Sync(1);

	if ((ZERO_FLAG(ctx.af)) == 0) {
		normalCycles += 12;

		word ipcopy = ctx.ip;

		mem->Write(ctx.sp - 1, GET_HIGH(ipcopy));

		state->Sync(1);

		mem->Write(ctx.sp - 2, GET_LOW(ipcopy));

		state->Sync(1);

		ctx.sp -= 2;

		ctx.ip = newip;

		state->Sync(1);

		if (state->IsDebugging() &&
			state->StacktraceEnabled()) {
			stacktrace_push(
				state, ipcopy - 3, newip,
				ipcopy
			);
		}
	}

	return normalCycles;
});

INSTRUCTION(2C, {
	INCL(ctx.hl);

	ctx.af = (ctx.af & 0xFF00) | (zero << 7) | (h << 5) | (carry << 4);

	return normalInstructions::cycles[0x2C];
});

INSTRUCTION(A9, {
	byte val = GET_LOW(ctx.bc);

	XOR(val);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return normalInstructions::cycles[0xA9];
});

INSTRUCTION(C6, {
	byte val = mem->Read(ctx.ip++);

	state->Sync(1);

	ADD(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0xC6];
});

INSTRUCTION(D6, {
	byte val = mem->Read(ctx.ip++);

	state->Sync(1);

	SUB(val);

	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0xD6];
});

INSTRUCTION(B7, {
	byte val = GET_HIGH(ctx.af);

	OR8_REG(val);

	SET_FLAGS(z, 0, 0, 0, ctx.af);
	return normalInstructions::cycles[0xB7];
});

INSTRUCTION(D5, {
	PUSH16_REG(ctx.de);

	return normalInstructions::cycles[0xD5];
});

INSTRUCTION(2D, {
	DECL(ctx.hl);

SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x2D];
});


INSTRUCTION(AE, {
	byte val = mem->Read(ctx.hl);

	state->Sync(1);

	XOR(val);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return normalInstructions::cycles[0xAE];
});

INSTRUCTION(46, {
	LD8H_REG_MEM(ctx.bc, ctx.hl);

	return normalInstructions::cycles[0x46];
});

INSTRUCTION(4E, {
	LDL_REG_MEM(ctx.bc, ctx.hl);

	return normalInstructions::cycles[0x4E];
});

INSTRUCTION(56, {
	LD8H_REG_MEM(ctx.de, ctx.hl);

	return normalInstructions::cycles[0x4E];
});


INSTRUCTION(26, {
	LD8_REG_H(ctx.hl);

	return normalInstructions::cycles[0x26];
});

INSTRUCTION(30, {
	byte Cycles = normalInstructions::cycles[0x30];

	signed char diff = (signed char)mem->Read(ctx.ip++);
	state->Sync(1);

	if ((CARRY_FLAG(ctx.af)) == 0) {
		Cycles += 4;
		ctx.ip = ctx.ip + diff;
		state->Sync(1);
	}

	return Cycles;
});

INSTRUCTION(25, {
	DECH(ctx.hl);

	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x25];
});

INSTRUCTION(5F, {
	SET_LOW(ctx.de, GET_HIGH(ctx.af));

	return normalInstructions::cycles[0x5F];
});

INSTRUCTION(EE, {
	byte val = mem->Read(ctx.ip++);

	state->Sync(1);

	XOR(val);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return normalInstructions::cycles[0xEE];
});

INSTRUCTION(47, {
	SET_HIGH(ctx.bc, GET_HIGH(ctx.af));

	return normalInstructions::cycles[0x47];
});

INSTRUCTION(79, {
	SET_HIGH(ctx.af, GET_LOW(ctx.bc));

	return normalInstructions::cycles[0x79];
});

INSTRUCTION(7A, {
	SET_HIGH(ctx.af, GET_HIGH(ctx.de));

	return normalInstructions::cycles[0x7A];
});

INSTRUCTION(72, {
	byte val = GET_HIGH(ctx.de);

	LD8_MEM_REG(ctx.hl, val);

	return normalInstructions::cycles[0x72];
});

INSTRUCTION(71, {
	byte val = GET_LOW(ctx.bc);

	LD8_MEM_REG(ctx.hl, val);

	return normalInstructions::cycles[0x71];
});

INSTRUCTION(70, {
	byte val = GET_HIGH(ctx.bc);

	LD8_MEM_REG(ctx.hl, val);

	return normalInstructions::cycles[0x70];
});

INSTRUCTION(D1, {
	POP16_REG(ctx.de);

	return normalInstructions::cycles[0xD1];
});

INSTRUCTION(CE, {
	byte val = mem->Read(ctx.ip++);

	state->Sync(1);

	ADC(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0xCE];
});

INSTRUCTION(D0, {
	byte cycles = normalInstructions::cycles[0xD0];

	state->Sync(1);

	if ((CARRY_FLAG(ctx.af)) == 0) {
		cycles += 12;

		word oldip = mem->Read(ctx.sp);
		state->Sync(1);
		oldip |= (mem->Read(ctx.sp + 1) << 8);
		state->Sync(1);
		ctx.ip = oldip;
		state->Sync(1);

		ctx.sp += 2;

		if (state->IsDebugging() &&
			state->StacktraceEnabled()) {
			stacktrace_pop(
				state, oldip
			);
		}
	}

	return cycles;
});

INSTRUCTION(C0, {
	byte cycles = normalInstructions::cycles[0xC0];

	state->Sync(1);

	if ((ZERO_FLAG(ctx.af)) == 0) {
		cycles += 12;

		word oldip = mem->Read(ctx.sp);
		state->Sync(1);
		oldip |= (mem->Read(ctx.sp + 1) << 8);
		state->Sync(1);
		ctx.ip = oldip;
		state->Sync(1);

		ctx.sp += 2;

		if (state->IsDebugging() &&
			state->StacktraceEnabled()) {
			stacktrace_pop(
				state, oldip
			);
		}
	}

	return cycles;
});

INSTRUCTION(C8, {
	byte cycles = normalInstructions::cycles[0xC8];

	state->Sync(1);

	if ((ZERO_FLAG(ctx.af)) == 1) {
		cycles += 12;

		word oldip = mem->Read(ctx.sp);
		state->Sync(1);
		oldip |= (mem->Read(ctx.sp + 1) << 8);
		state->Sync(1);
		ctx.ip = oldip;
		state->Sync(1);

		ctx.sp += 2;

		if (state->IsDebugging() &&
			state->StacktraceEnabled()) {
			stacktrace_pop(
				state, oldip
			);
		}
	}

	return cycles;
});

INSTRUCTION(D8, {
	byte cycles = normalInstructions::cycles[0xD8];

	state->Sync(1);

	if ((CARRY_FLAG(ctx.af)) == 1) {
		cycles += 12;

		word oldip = mem->Read(ctx.sp);
		state->Sync(1);
		oldip |= (mem->Read(ctx.sp + 1) << 8);
		state->Sync(1);
		ctx.ip = oldip;
		state->Sync(1);

		ctx.sp += 2;

		if (state->IsDebugging() &&
			state->StacktraceEnabled()) {
			stacktrace_pop(
				state, oldip
			);
		}
	}

	return cycles;
});

INSTRUCTION(B6, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	OR8_REG(val);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return normalInstructions::cycles[0xB6];
});

/*INSTRUCTION(35, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	byte c = ((ctx.af & 0xFF) >> 4) & 1; \
	byte h = ((byte)((val - 1) & 0xF) == 0xF); \
	val--; \
	byte z = (val == 0);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x35];
});*/

INSTRUCTION(6E, {
	LDL_REG_MEM(ctx.hl, ctx.hl);

	return normalInstructions::cycles[0x6E];
});

INSTRUCTION(6F, {
	SET_LOW(ctx.hl, GET_HIGH(ctx.af));

	return normalInstructions::cycles[0x6F];
});

//- 0 H C
#define ADD_HL(reg) \
	byte z = ((ctx.af >> 7) & 1); \
	byte h = ((unsigned)(ctx.hl & 0xFFF) + (unsigned)(reg & 0xFFF)) > 0xFFF; \
	byte c = ((unsigned)ctx.hl + (unsigned)reg) > 0xFFFF; \
	ctx.hl = ctx.hl + reg; \
	state->Sync(1);

INSTRUCTION(29, {
	ADD_HL(ctx.hl);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x29];
});

INSTRUCTION(E9, {
	ctx.ip = ctx.hl;

	return normalInstructions::cycles[0xE9];
});

INSTRUCTION(3C, {
	INCH(ctx.af);

	SET_FLAGS(zero, 0, h, carry, ctx.af);

	return normalInstructions::cycles[0x3C];
});

INSTRUCTION(F6, {
	byte val = mem->Read(ctx.ip++);
	state->Sync(1);

	OR8_REG(val);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return normalInstructions::cycles[0xF6];
});

INSTRUCTION(C2, {
	byte cycles = normalInstructions::cycles[0xC2];
	
	byte low = mem->Read(ctx.ip++);
	state->Sync(1);

	byte high = mem->Read(ctx.ip++);
	state->Sync(1);

	if ((ZERO_FLAG(ctx.af)) == 0) {
		cycles += 4;

		ctx.ip = (high << 8) | low;

		state->Sync(1);
	}

	return cycles;
});

INSTRUCTION(91, {
	byte val = GET_LOW(ctx.bc);

	SUB(val);

	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x91];
});

INSTRUCTION(81, {
	byte val = GET_LOW(ctx.bc);

	ADD(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x81];
});

//0 0 H C
INSTRUCTION(F8, {
	byte r = mem->Read(ctx.ip++);
	state->Sync(1);

	word hl = ctx.hl;

	byte c = ((unsigned)(ctx.sp & 0xFF) + (unsigned)r) > 0xFF;
	byte h = ((byte)(ctx.sp & 0xF) + (byte)(r & 0xF)) > 0xF;

	ctx.hl = ctx.sp + (signed char)r;

	state->Sync(1);

	SET_FLAGS(0, 0, h, c, ctx.af);

	return normalInstructions::cycles[0xF8];
});

INSTRUCTION(62, {
	SET_HIGH(ctx.hl, GET_HIGH(ctx.de));

	return normalInstructions::cycles[0x62];
});

INSTRUCTION(6B, {
	SET_LOW(ctx.hl, GET_LOW(ctx.de));

	return normalInstructions::cycles[0x6B];
});

INSTRUCTION(12, {
	byte val = GET_HIGH(ctx.af);

	LD8_MEM_REG(ctx.de, val);

	return normalInstructions::cycles[0x12];
});

INSTRUCTION(1C, {
	INCL(ctx.de);

	SET_FLAGS(zero, 0, h, carry, ctx.af);

	return normalInstructions::cycles[0x1C];
});

INSTRUCTION(14, {
	INCH(ctx.de);

	SET_FLAGS(zero, 0, h, carry, ctx.af);

	return normalInstructions::cycles[0x14];
});

INSTRUCTION(BB, {
	byte val = GET_LOW(ctx.de);

	CP(val);

	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0xBB];
});

INSTRUCTION(7E, {
	LD8H_REG_MEM(ctx.af, ctx.hl);

	return normalInstructions::cycles[0x7E];
});

INSTRUCTION(38, {
	byte cycles = normalInstructions::cycles[0x38];

	signed char diff = (signed char)mem->Read(ctx.ip++);
	state->Sync(1);

	if ((CARRY_FLAG(ctx.af)) == 1) {
		cycles += 4;

		ctx.ip = ctx.ip + diff;

		state->Sync(1);
	}

	return cycles;
});

INSTRUCTION(F9, {
	ctx.sp = ctx.hl;

	state->Sync(1);

	return normalInstructions::cycles[0x7E];
});

byte instruction_27(GameboyEmu::CPU::CpuContext& ctx, GameboyEmu::Mem::Memory* mem, GameboyEmu::State::EmulatorState* state) {
	int a = GET_HIGH(ctx.af);

	byte s = SIGN_FLAG(ctx.af);

	byte c = CARRY_FLAG(ctx.af);

	byte h = HALF_FLAG(ctx.af);

	//behaviour changes depending
	//on the last executed operation
	// 1) Addition
	// 2) Subtraction
	if (!s) {
		//Addition
		if (h || ((a & 0xF) > 0x9)) {
			a += 0x6;
		}

		if (c || (a > 0x9F)) {
			a += 0x60;
		}
	}
	else {
		//Subtraction
		if (h) {
			a = (a - 0x6) & 0xFF;
		}

		if (c) {
			a -= 0x60;
		}
	}

	if ((a & 0x100) == 0x100) {
		c = 1;
	}

	a &= 0xFF;

	byte z = (a == 0);

	SET_HIGH(ctx.af, a);

	SET_FLAGS(z, s, 0, c, ctx.af);

	return normalInstructions::cycles[0x27];
}

INSTRUCTION(2F, {
	byte a = GET_HIGH(ctx.af);

	a = ~a;

	SET_HIGH(ctx.af, a);

	byte z = ZERO_FLAG(ctx.af);
	byte c = CARRY_FLAG(ctx.af);

	SET_FLAGS(z, 1, 1, c, ctx.af);

	return normalInstructions::cycles[0x2F];
});

INSTRUCTION(BA, {
	byte val = GET_HIGH(ctx.de);

	CP(val);
	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0xBA];
});

INSTRUCTION(B9, {
	byte val = GET_LOW(ctx.bc);

	CP(val);
	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0xB9];
});

INSTRUCTION(B8, {
	byte val = GET_HIGH(ctx.bc);

	CP(val);
	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0xB8];
});

INSTRUCTION(FB, {
	if (!ctx.enableInt) {
		ctx.ei_delay = 2;
	}

	return normalInstructions::cycles[0xFB];
});

INSTRUCTION(CA, {
	byte cycles = normalInstructions::cycles[0xCA];

	byte low = mem->Read(ctx.ip++);
	state->Sync(1);

	byte high = mem->Read(ctx.ip++);
	state->Sync(1);

	if ((ZERO_FLAG(ctx.af)) == 1) {
		cycles += 4;

		ctx.ip = (high << 8) | low;

		state->Sync(1);
	}

	return cycles;
});

INSTRUCTION(76, {
	ctx.halted = true;

	if (!ctx.enableInt) {
		byte ie = mem->Read(0xFFFF);
		byte ir = mem->Read(0xFF0F);

		if ((ie & ir) != 0) {
			ctx.haltBug = true;
		}
	}

	return normalInstructions::cycles[0x76];
});

INSTRUCTION(07, {
	//RLCA
	byte a = GET_HIGH(ctx.af);

	byte c = (a >> 7) & 1;

	a <<= 1;

	a |= c;

	SET_HIGH(ctx.af, a);

	byte z = (a == 0);

	SET_FLAGS(0, 0, 0, c, ctx.af);

	return normalInstructions::cycles[0x07];
});

INSTRUCTION(5D, {
	SET_LOW(ctx.de, GET_LOW(ctx.hl));

return normalInstructions::cycles[0x5D];
});

INSTRUCTION(73, {
	byte val = GET_LOW(ctx.de);

	LD8_MEM_REG(ctx.hl, val);

	return normalInstructions::cycles[0x73];
});

INSTRUCTION(5E, {
	LDL_REG_MEM(ctx.de, ctx.hl);

	return normalInstructions::cycles[0x5E];
});

INSTRUCTION(08, {
	byte low = mem->Read(ctx.ip++);
	state->Sync(1);

	byte high = mem->Read(ctx.ip++);
	state->Sync(1);

	word address = (high << 8) | low;

	byte b1 = GET_LOW(ctx.sp);
	byte b2 = GET_HIGH(ctx.sp);

	mem->Write(address, b1);
	state->Sync(1);

	mem->Write(address + 1, b2);
	state->Sync(1);

	return normalInstructions::cycles[0x08];
});

INSTRUCTION(66, {
	LD8H_REG_MEM(ctx.hl, ctx.hl);

	return normalInstructions::cycles[0x66];
});

INSTRUCTION(33, {
	ctx.sp++;

	state->Sync(1);

	return normalInstructions::cycles[0x33];
});

INSTRUCTION(AD, {
	byte val = GET_LOW(ctx.hl);

	XOR(val);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return normalInstructions::cycles[0xAD];
	});

INSTRUCTION(B0, {
	byte val = GET_HIGH(ctx.bc);

	OR8_REG(val);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return normalInstructions::cycles[0xB0];
	});

INSTRUCTION(3B, {
	ctx.sp--;

	state->Sync(1);

	return normalInstructions::cycles[0x3B];
	});

#define ADD_HL2(val) \
	byte z = (ctx.af >> 7) & 1; \
	byte h = ((unsigned)(ctx.hl & 0xFFF) \
		+ (unsigned)(val & 0xFFF)) > 0xFFF; \
	byte c = ((unsigned)ctx.hl + (unsigned)val) > 0xFFFF; \
	ctx.hl += val; \
	state->Sync(1);

INSTRUCTION(39, {
	byte z = ZERO_FLAG(ctx.af);
	byte h = ((unsigned)(ctx.hl & 0xFFF)
		+ (unsigned)(ctx.sp & 0xFFF)) > 0xFFF;
	byte c = ((unsigned)ctx.hl + (unsigned)ctx.sp) > 0xFFFF;

	ctx.hl += ctx.sp;

	state->Sync(1);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x39];
	});

INSTRUCTION(E8, {
	byte r8 = mem->Read(ctx.ip++);

	state->Sync(1);

	byte h = ((byte)(ctx.sp & 0xF)
		+ (byte)(r8 & 0xF)) > 0xF;
	byte c = ((unsigned)(ctx.sp & 0xFF) 
		+ (unsigned)r8) > 0xFF;

	ctx.sp = ctx.sp + (signed char)r8;

	SET_FLAGS(0, 0, h, c, ctx.af);

	state->Sync(1);
	state->Sync(1);

	return normalInstructions::cycles[0xE8];
	});

INSTRUCTION(36, {
	byte val = mem->Read(ctx.ip++);
	state->Sync(1);

	LD8_MEM_REG(ctx.hl, val);
	
	return normalInstructions::cycles[0x36];
	});

#define SBC(v) \
	byte a = (ctx.af >> 8) & 0xFF; \
	byte old_carry = (ctx.af >> 4) & 1; \
	byte c = (a < v + old_carry); \
	byte h =  ((int)(a & 0xF) - \
		(int)(v & 0xF) - old_carry) < 0;\
	byte res = a - v - old_carry; \
	byte z = (res == 0); \
	ctx.af = (res << 8) | (ctx.af & 0xFF);

INSTRUCTION(DE, {
	//SBC A, d8
	byte val = mem->Read(ctx.ip++);

	state->Sync(1);

	SBC(val);

	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0xDE];
	});

INSTRUCTION(0B, {
	//DEC BC
	ctx.bc--;

	state->Sync(1);

	return normalInstructions::cycles[0x0B];
	});

INSTRUCTION(1B, {
	//DEC DE
	ctx.de--;

	state->Sync(1);

	return normalInstructions::cycles[0x1B];
	});

INSTRUCTION(2B, {
	//DEC DE
	ctx.hl--;

	state->Sync(1);

	return normalInstructions::cycles[0x2B];
	});

INSTRUCTION(09, {
	word val = ctx.bc;

	ADD_HL2(val);
	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x09];
	});

INSTRUCTION(19, {
	word val = ctx.de;

	ADD_HL2(val);
	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x19];
	});

INSTRUCTION(40, {
	//LD B, B
	//Do nothing
	return normalInstructions::cycles[0x40];
	});

INSTRUCTION(41, {
	//LD B, C
	byte c = GET_LOW(ctx.bc);
	SET_HIGH(ctx.bc, c);

	return normalInstructions::cycles[0x41];
	});

INSTRUCTION(42, {
	//LD B, D
	byte d = GET_HIGH(ctx.de);
	SET_HIGH(ctx.bc, d);

	return normalInstructions::cycles[0x42];
	});

INSTRUCTION(43, {
	//LD B, E
	byte e = GET_LOW(ctx.de);
	SET_HIGH(ctx.bc, e);

	return normalInstructions::cycles[0x43];
	});

INSTRUCTION(44, {
	//LD B, H
	byte h = GET_HIGH(ctx.hl);
	SET_HIGH(ctx.bc, h);

	return normalInstructions::cycles[0x44];
	});

INSTRUCTION(45, {
	//LD B, L
	byte l = GET_LOW(ctx.hl);
	SET_HIGH(ctx.bc, l);

	return normalInstructions::cycles[0x45];
	});

INSTRUCTION(48, {
	//LD C, B
	byte b = GET_HIGH(ctx.bc);
	SET_LOW(ctx.bc, b);

	return normalInstructions::cycles[0x48];
	});

INSTRUCTION(49, {
	//LD C, C
	//Do nothing

	return normalInstructions::cycles[0x49];
	});

INSTRUCTION(4A, {
	//LD C, D
	byte d = GET_HIGH(ctx.de);
	SET_LOW(ctx.bc, d);

	return normalInstructions::cycles[0x4A];
	});

INSTRUCTION(4B, {
	//LD C, E
	byte e = GET_LOW(ctx.de);
	SET_LOW(ctx.bc, e);

	return normalInstructions::cycles[0x4B];
	});

INSTRUCTION(4C, {
	//LD C, H
	byte h = GET_HIGH(ctx.hl);
	SET_LOW(ctx.bc, h);

	return normalInstructions::cycles[0x4C];
	});

INSTRUCTION(4D, {
	//LD C, L
	byte l = GET_LOW(ctx.hl);
	SET_LOW(ctx.bc, l);

	return normalInstructions::cycles[0x4D];
	});

INSTRUCTION(50, {
	//LD D, B
	byte b = GET_HIGH(ctx.bc);

	SET_HIGH(ctx.de, b);

	return normalInstructions::cycles[0x50];
	});

INSTRUCTION(51, {
	//LD D, C
	byte c = GET_LOW(ctx.bc);
	SET_HIGH(ctx.de, c);

	return normalInstructions::cycles[0x51];
	});

INSTRUCTION(52, {
	//LD D, D
	//Do nothing

	return normalInstructions::cycles[0x52];
	});

INSTRUCTION(53, {
	//LD D, E
	byte e = GET_LOW(ctx.de);
	SET_HIGH(ctx.de, e);

	return normalInstructions::cycles[0x53];
	});

INSTRUCTION(54, {
	//LD D, H
	byte h = GET_HIGH(ctx.hl);
	SET_HIGH(ctx.de, h);

	return normalInstructions::cycles[0x54];
	});

INSTRUCTION(55, {
	//LD D, L
	byte l = GET_LOW(ctx.hl);
	SET_HIGH(ctx.de, l);

	return normalInstructions::cycles[0x55];
	});

INSTRUCTION(58, {
	//LD E, B
	byte b = GET_HIGH(ctx.bc);
	SET_LOW(ctx.de, b);

	return normalInstructions::cycles[0x58];
	});

INSTRUCTION(59, {
	//LD E, C
	byte c = GET_LOW(ctx.bc);
	SET_LOW(ctx.de, c);

	return normalInstructions::cycles[0x59];
	});

INSTRUCTION(5A, {
	//LD E, D
	byte d = GET_HIGH(ctx.de);
	SET_LOW(ctx.de, d);

	return normalInstructions::cycles[0x5A];
	});

INSTRUCTION(5B, {
	//LD E, E
	//Do nothing

	return normalInstructions::cycles[0x5B];
	});

INSTRUCTION(5C, {
	//LD E, H
	byte h = GET_HIGH(ctx.hl);
	SET_LOW(ctx.de, h);

	return normalInstructions::cycles[0x5C];
	});

INSTRUCTION(60, {
	//LD H, B
	byte b = GET_HIGH(ctx.bc);

	SET_HIGH(ctx.hl, b);

	return normalInstructions::cycles[0x60];
	});

INSTRUCTION(61, {
	//LD H, C
	byte c = GET_LOW(ctx.bc);
	SET_HIGH(ctx.hl, c);

	return normalInstructions::cycles[0x61];
	});

INSTRUCTION(63, {
	//LD H, E
	byte e = GET_LOW(ctx.de);
	SET_HIGH(ctx.hl, e);

	return normalInstructions::cycles[0x63];
	});

INSTRUCTION(64, {
	//LD H, H
	//Do nothing
	return normalInstructions::cycles[0x64];
	});

INSTRUCTION(65, {
	//LD H, L
	byte l = GET_LOW(ctx.hl);
	SET_HIGH(ctx.hl, l);

	return normalInstructions::cycles[0x65];
	});

INSTRUCTION(68, {
	//LD L, B
	byte b = GET_HIGH(ctx.bc);
	SET_LOW(ctx.hl, b);

	return normalInstructions::cycles[0x68];
	});

INSTRUCTION(69, {
	//LD L, C
	byte c = GET_LOW(ctx.bc);
	SET_LOW(ctx.hl, c);

	return normalInstructions::cycles[0x69];
	});

INSTRUCTION(6A, {
	//LD H, D
	byte d = GET_HIGH(ctx.de);
	SET_LOW(ctx.hl, d);

	return normalInstructions::cycles[0x6A];
	});

INSTRUCTION(6C, {
	//LD L, H
	byte h = GET_HIGH(ctx.hl);
	SET_LOW(ctx.hl, h);

	return normalInstructions::cycles[0x6C];
	});

INSTRUCTION(6D, {
	//LD L, L
	//Do nothing

	return normalInstructions::cycles[0x6D];
	});

INSTRUCTION(74, {
	//LD (HL), H
	byte val = GET_HIGH(ctx.hl);

	LD8_MEM_REG(ctx.hl, val);
	
	return normalInstructions::cycles[0x74];
	});

INSTRUCTION(75, {
	//LD (HL), L
	byte val = GET_LOW(ctx.hl);

	LD8_MEM_REG(ctx.hl, val);

	return normalInstructions::cycles[0x75];
	});

INSTRUCTION(7F, {
	//LD A, A
	//Do nothing

	return normalInstructions::cycles[0x7F];
	});

INSTRUCTION(D2, {
	byte cycles = normalInstructions::cycles[0xD2];

	byte low = mem->Read(ctx.ip++);
	state->Sync(1);

	byte high = mem->Read(ctx.ip++);
	state->Sync(1);

	word address = (high << 8) | low;

	if ((CARRY_FLAG(ctx.af)) == 0) {
		cycles += 4;

		ctx.ip = address;

		state->Sync(1);
	}

	return cycles;
	});

INSTRUCTION(DA, {
	byte cycles = normalInstructions::cycles[0xDA];

	byte low = mem->Read(ctx.ip++);
	state->Sync(1);

	byte high = mem->Read(ctx.ip++);
	state->Sync(1);

	word address = (high << 8) | low;

	if ((CARRY_FLAG(ctx.af)) == 1) {
		cycles += 4;

		ctx.ip = address;

		state->Sync(1);
	}

	return cycles;
	});

INSTRUCTION(CC, {
	byte normalCycles = normalInstructions::cycles[0xCC];

	word newip = mem->Read(ctx.ip++);

	state->Sync(1);

	newip |= (mem->Read(ctx.ip++) << 8);

	state->Sync(1);

	if ((ZERO_FLAG(ctx.af)) == 1) {
		normalCycles += 12;

		word ipcopy = ctx.ip;

		mem->Write(ctx.sp - 1, GET_HIGH(ipcopy));

		state->Sync(1);

		mem->Write(ctx.sp - 2, GET_LOW(ipcopy));

		state->Sync(1);

		ctx.sp -= 2;

		ctx.ip = newip;

		state->Sync(1);

		if (state->IsDebugging() &&
			state->StacktraceEnabled()) {
			stacktrace_push(
				state, ipcopy - 3, newip,
				ipcopy
			);
		}
	}

	return normalCycles;
	});

INSTRUCTION(D4, {
	byte normalCycles = normalInstructions::cycles[0xD4];

	word newip = mem->Read(ctx.ip++);

	state->Sync(1);

	newip |= (mem->Read(ctx.ip++) << 8);

	state->Sync(1);

	if ((CARRY_FLAG(ctx.af)) == 0) {
		normalCycles += 12;

		word ipcopy = ctx.ip;

		mem->Write(ctx.sp - 1, GET_HIGH(ipcopy));

		state->Sync(1);

		mem->Write(ctx.sp - 2, GET_LOW(ipcopy));

		state->Sync(1);

		ctx.sp -= 2;

		ctx.ip = newip;

		state->Sync(1);

		if (state->IsDebugging() &&
			state->StacktraceEnabled()) {
			stacktrace_push(
				state, ipcopy - 3, newip,
				ipcopy
			);
		}
	}

	return normalCycles;
	});

INSTRUCTION(DC, {
	byte normalCycles = normalInstructions::cycles[0xDC];

	word newip = mem->Read(ctx.ip++);

	state->Sync(1);

	newip |= (mem->Read(ctx.ip++) << 8);

	state->Sync(1);

	if ((CARRY_FLAG(ctx.af)) == 1) {
		normalCycles += 12;

		word ipcopy = ctx.ip;

		mem->Write(ctx.sp - 1, GET_HIGH(ipcopy));

		state->Sync(1);

		mem->Write(ctx.sp - 2, GET_LOW(ipcopy));

		state->Sync(1);

		ctx.sp -= 2;

		ctx.ip = newip;

		state->Sync(1);

		if (state->IsDebugging() &&
			state->StacktraceEnabled()) {
			stacktrace_push(
				state, ipcopy - 3, newip,
				ipcopy
			);
		}
	}

	return normalCycles;
	});

INSTRUCTION(D9, {
	word oldip = mem->Read(ctx.sp);
	state->Sync(1);
	oldip |= (mem->Read(ctx.sp + 1) << 8);
	state->Sync(1);
	ctx.ip = oldip;
	state->Sync(1);

	ctx.sp += 2;

	if (!ctx.enableInt) {
		ctx.ei_delay = 2;
	}

	if (state->IsDebugging() &&
		state->StacktraceEnabled()) {
		stacktrace_pop(
			state, oldip
		);
	}

	return normalInstructions::cycles[0xD9];
	});

#define RST(handler) \
	mem->Write(ctx.sp - 1, (ctx.ip >> 8) & 0xFF); \
	state->Sync(1); \
	mem->Write(ctx.sp - 2, ctx.ip & 0xFF);\
	ctx.sp -= 2; \
	state->Sync(1); \
	ctx.ip = handler; \
	state->Sync(1);

INSTRUCTION(C7, {
	if (state->IsDebugging() &&
		state->StacktraceEnabled()) {
		stacktrace_push(
			state, ctx.ip - 1, 0x0000,
			ctx.ip
		);
	}

	RST(0x0000);

	return normalInstructions::cycles[0xC7];
});

INSTRUCTION(D7, {
	if (state->IsDebugging() &&
		state->StacktraceEnabled()) {
		stacktrace_push(
			state, ctx.ip - 1, 0x0010,
			ctx.ip
		);
	}

	RST(0x0010);

	return normalInstructions::cycles[0xD7];
	});

INSTRUCTION(E7, {
	if (state->IsDebugging() &&
		state->StacktraceEnabled()) {
		stacktrace_push(
			state, ctx.ip - 1, 0x0020,
			ctx.ip
		);
	}

	RST(0x0020);

	return normalInstructions::cycles[0xE7];
	});

INSTRUCTION(F7, {
	if (state->IsDebugging() &&
		state->StacktraceEnabled()) {
		stacktrace_push(
			state, ctx.ip - 1, 0x0030,
			ctx.ip
		);
	}

	RST(0x0030);

	return normalInstructions::cycles[0xF7];
	});

INSTRUCTION(CF, {
	if (state->IsDebugging() &&
		state->StacktraceEnabled()) {
		stacktrace_push(
			state, ctx.ip - 1, 0x0008,
			ctx.ip
		);
	}

	RST(0x0008);

	return normalInstructions::cycles[0xCF];
	});

INSTRUCTION(DF, {
	if (state->IsDebugging() &&
		state->StacktraceEnabled()) {
		stacktrace_push(
			state, ctx.ip - 1, 0x0018,
			ctx.ip
		);
	}

	RST(0x0018);

	return normalInstructions::cycles[0xDF];
	});

INSTRUCTION(EF, {
	if (state->IsDebugging() &&
		state->StacktraceEnabled()) {
		stacktrace_push(
			state, ctx.ip - 1, 0x0028,
			ctx.ip
		);
	}

	RST(0x0028);

	return normalInstructions::cycles[0xEF];
	});

INSTRUCTION(FF, {
	if (state->IsDebugging() &&
		state->StacktraceEnabled()) {
		stacktrace_push(
			state, ctx.ip - 1, 0x0038,
			ctx.ip
		);
	}

	RST(0x0038);

	return normalInstructions::cycles[0xFF];
	});

INSTRUCTION(F2, {
	byte c = GET_LOW(ctx.bc);

	byte val = mem->Read(0xFF00 + c);
	state->Sync(1);

	SET_HIGH(ctx.af, val);

	return normalInstructions::cycles[0xF2];
	});

INSTRUCTION(37, {
	byte z = ZERO_FLAG(ctx.af);

	SET_FLAGS(z, 0, 0, 1, ctx.af);

	return normalInstructions::cycles[0x37];
	});

INSTRUCTION(3F, {
	byte z = ZERO_FLAG(ctx.af);
	byte c = !(CARRY_FLAG(ctx.af));

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return normalInstructions::cycles[0x3F];
	});

INSTRUCTION(B2, {
	byte val = GET_HIGH(ctx.de);

	OR8_REG(val);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return normalInstructions::cycles[0xB2];
	});

INSTRUCTION(B3, {
	byte val = GET_LOW(ctx.de);

	OR8_REG(val);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return normalInstructions::cycles[0xB3];
	});

INSTRUCTION(B4, {
	byte val = GET_HIGH(ctx.hl);

	OR8_REG(val);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return normalInstructions::cycles[0xB4];
	});

INSTRUCTION(B5, {
	byte val = GET_LOW(ctx.hl);

	OR8_REG(val);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return normalInstructions::cycles[0xB5];
	});

INSTRUCTION(BC, {
	byte val = GET_HIGH(ctx.hl);

	CP(val);

	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0xBC];
	});

INSTRUCTION(BD, {
	byte val = GET_LOW(ctx.hl);

	CP(val);

	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0xBD];
	});

INSTRUCTION(BF, {
	byte val = GET_HIGH(ctx.af);

	CP(val);

	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0xBF];
	});

INSTRUCTION(80, {
	byte val = GET_HIGH(ctx.bc);

	ADD(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x80];
	});

INSTRUCTION(82, {
	byte val = GET_HIGH(ctx.de);

	ADD(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x82];
	});

INSTRUCTION(83, {
	byte val = GET_LOW(ctx.de);

	ADD(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x83];
	});

INSTRUCTION(84, {
	byte val = GET_HIGH(ctx.hl);

	ADD(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x84];
	});

INSTRUCTION(85, {
	byte val = GET_LOW(ctx.hl);

	ADD(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x85];
	});

INSTRUCTION(87, {
	byte val = GET_HIGH(ctx.af);

	ADD(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x87];
	});

INSTRUCTION(88, {
	byte val = GET_HIGH(ctx.bc);

	ADC(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x88];
	});

INSTRUCTION(89, {
	byte val = GET_LOW(ctx.bc);

	ADC(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x89];
	});

INSTRUCTION(8A, {
	byte val = GET_HIGH(ctx.de);

	ADC(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x8A];
	});

INSTRUCTION(8B, {
	byte val = GET_LOW(ctx.de);

	ADC(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x8B];
	});

INSTRUCTION(8C, {
	byte val = GET_HIGH(ctx.hl);

	ADC(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x8C];
	});

INSTRUCTION(8D, {
	byte val = GET_LOW(ctx.hl);

	ADC(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x8D];
	});

INSTRUCTION(8F, {
	byte val = GET_HIGH(ctx.af);

	ADC(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x8F];
	});

INSTRUCTION(92, {
	byte val = GET_HIGH(ctx.de);

	SUB(val);
	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x92];
	});

INSTRUCTION(93, {
	byte val = GET_LOW(ctx.de);

	SUB(val);
	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x93];
	});

INSTRUCTION(94, {
	byte val = GET_HIGH(ctx.hl);

	SUB(val);
	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x94];
	});

INSTRUCTION(95, {
	byte val = GET_LOW(ctx.hl);

	SUB(val);
	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x95];
	});

INSTRUCTION(96, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	SUB(val);
	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x96];
	});

INSTRUCTION(97, {
	byte val = GET_HIGH(ctx.af);

	SUB(val);
	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x97];
	});

INSTRUCTION(98, {
	byte val = GET_HIGH(ctx.bc);

	SBC(val);
	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x98];
	});

INSTRUCTION(99, {
	byte val = GET_LOW(ctx.bc);

	SBC(val);
	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x99];
	});

INSTRUCTION(9A, {
	byte val = GET_HIGH(ctx.de);

	SBC(val);
	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x9A];
	});

INSTRUCTION(9B, {
	byte val = GET_LOW(ctx.de);

	SBC(val);
	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x9B];
	});

INSTRUCTION(9C, {
	byte val = GET_HIGH(ctx.hl);

	SBC(val);
	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x9C];
	});

INSTRUCTION(9D, {
	byte val = GET_LOW(ctx.hl);

	SBC(val);
	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x9D];
	});

INSTRUCTION(9E, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	SBC(val);
	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x9E];
	});

INSTRUCTION(9F, {
	byte val = GET_HIGH(ctx.af);

	SBC(val);
	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x9F];
	});

INSTRUCTION(A0, {
	byte val = GET_HIGH(ctx.bc);

	AND(val);
	SET_FLAGS(z, 0, 1, 0, ctx.af);

	return normalInstructions::cycles[0xA0];
	});

INSTRUCTION(A1, {
	byte val = GET_LOW(ctx.bc);

	AND(val);
	SET_FLAGS(z, 0, 1, 0, ctx.af);

	return normalInstructions::cycles[0xA1];
	});

INSTRUCTION(A2, {
	byte val = GET_HIGH(ctx.de);

	AND(val);
	SET_FLAGS(z, 0, 1, 0, ctx.af);

	return normalInstructions::cycles[0xA2];
	});

INSTRUCTION(A3, {
	byte val = GET_LOW(ctx.de);

	AND(val);
	SET_FLAGS(z, 0, 1, 0, ctx.af);

	return normalInstructions::cycles[0xA3];
	});

INSTRUCTION(A4, {
	byte val = GET_HIGH(ctx.hl);

	AND(val);
	SET_FLAGS(z, 0, 1, 0, ctx.af);

	return normalInstructions::cycles[0xA4];
	});

INSTRUCTION(A5, {
	byte val = GET_LOW(ctx.hl);

	AND(val);
	SET_FLAGS(z, 0, 1, 0, ctx.af);

	return normalInstructions::cycles[0xA5];
	});

INSTRUCTION(A6, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	AND(val);
	SET_FLAGS(z, 0, 1, 0, ctx.af);

	return normalInstructions::cycles[0xA6];
	});

INSTRUCTION(A7, {
	byte val = GET_HIGH(ctx.af);

	AND(val);
	SET_FLAGS(z, 0, 1, 0, ctx.af);

	return normalInstructions::cycles[0xA7];
	});

INSTRUCTION(A8, {
	byte val = GET_HIGH(ctx.bc);

	XOR(val);
	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return normalInstructions::cycles[0xA8];
	});

INSTRUCTION(AA, {
	byte val = GET_HIGH(ctx.de);

	XOR(val);
	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return normalInstructions::cycles[0xAA];
	});

INSTRUCTION(AB, {
	byte val = GET_LOW(ctx.de);

	XOR(val);
	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return normalInstructions::cycles[0xAB];
	});

INSTRUCTION(AC, {
	byte val = GET_HIGH(ctx.hl);

	XOR(val);
	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return normalInstructions::cycles[0xAC];
	});

INSTRUCTION(0A, {
	byte val = mem->Read(ctx.bc);
	state->Sync(1);

	SET_HIGH(ctx.af, val);

	return normalInstructions::cycles[0x0A];
	});

INSTRUCTION(02, {
	byte val = GET_HIGH(ctx.af);

	mem->Write(ctx.bc, val);
	state->Sync(1);

	return normalInstructions::cycles[0x02];
	});

INSTRUCTION(3A, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	ctx.hl--;

	SET_HIGH(ctx.af, val);

	return normalInstructions::cycles[0x3A];
	});

INSTRUCTION(8E, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	ADC(val);

	SET_FLAGS(z, 0, h, c, ctx.af);

	return normalInstructions::cycles[0x8E];
	});

INSTRUCTION(34, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	INCL(val);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	SET_FLAGS(zero, 0, h, carry, ctx.af);

	return normalInstructions::cycles[0x34];
	});

INSTRUCTION(35, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	DECL(val);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	SET_FLAGS(z, 1, h, c, ctx.af);

	return normalInstructions::cycles[0x35];
	});

/*

INSTRUCTION(, {

});


*/



using jmp_type = byte(*)(GameboyEmu::CPU::CpuContext&, GameboyEmu::Mem::Memory*, GameboyEmu::State::EmulatorState*);

jmp_type* cb_table = nullptr;

#define CB_INSTRUCTION(code, body) byte cbinstruction_##code(GameboyEmu::CPU::CpuContext& ctx, GameboyEmu::Mem::Memory* mem, GameboyEmu::State::EmulatorState* state) body
#define CB_INT_PTR(code) &cbinstruction_##code

#define TEST_BIT(reg, bitnum) \
    byte bit = (reg >> bitnum) & 1; \
	byte c_flag = ((ctx.af & 0xFF) >> 4) & 1; \
	byte z_flag = (bit == 0);

#define RLLOW(reg) \
	byte low = (reg & 0xFF); \
	byte cflag = ((ctx.af & 0xFF) >> 4) & 1; \
    byte carry = (low >> 7) & 1; \
	low <<= 1; \
	low |= cflag; \
	reg = (reg & 0xFF00) | low; \
	byte zero = (low == 0);

#define RLHIGH(reg) \
    byte high = (reg & 0xFF00) >> 8; \
	byte cflag = ((ctx.af & 0xFF) >> 4) & 1; \
	byte carry = (high >> 7) & 1; \
	high <<= 1; \
	high |= cflag; \
	reg = (high << 8) | (reg & 0xFF); \
	byte zero = (high == 0);

//Z 0 0 C
#define SRLH(reg) \
	byte high = (reg & 0xFF00) >> 8; \
	byte c = high & 1; \
	high >>= 1; \
	high &= ~(1 << 7); \
	byte z = (high == 0); \
	reg = (high << 8) | (reg & 0xFF);

#define SRLL(reg) \
	byte low = (reg & 0xFF); \
	byte c = low & 1; \
	low >>= 1; \
	low &= ~(1 << 7); \
	byte z = (low == 0); \
	reg = (reg & 0xFF00) | low;

//Z 0 0 C
#define RRH(reg) \
	byte high = (reg & 0xFF00) >> 8; \
	byte cflag = ((ctx.af & 0xFF) >> 4) & 1; \
	byte c = (high & 1); \
	high >>= 1; \
	high |= (cflag << 7); \
	byte z = (high == 0); \
	reg = (high << 8) | (reg & 0xFF);

#define RRL(reg) \
	byte low = (reg & 0xFF); \
	byte cflag = ((ctx.af & 0xFF) >> 4) & 1; \
	byte c = (low & 1); \
	low >>= 1; \
	low |= (cflag << 7); \
	byte z = (low == 0); \
	reg = (reg & 0xFF00) | low;

#define SWAPL(reg) \
	byte val = (reg & 0xFF); \
	val = ((val & 0xF0) >> 4) | ((val & 0x0F) << 4); \
	reg = (reg & 0xFF00) | val; \
	byte z = (val == 0);

#define SWAPH(reg) \
	byte val = (reg & 0xFF00) >> 8; \
	val = ((val & 0xF0) >> 4) | ((val & 0x0F) << 4); \
	reg = (val << 8) | (reg & 0xFF); \
	byte z = (val == 0);

#define RRCH(reg) \
	byte val = (reg & 0xFF00) >> 8; \
	byte c = (val & 1); \
	val >>= 1; \
	val |= (c << 7); \
	byte z = (val == 0); \
	reg = (val << 8) | (reg & 0xFF);

#define RRCL(reg) \
	byte val = (reg & 0xFF); \
	byte c = (val & 1); \
	val >>= 1; \
	val |= (c << 7); \
	byte z = (val == 0); \
	reg = (reg & 0xFF00) | val;

#define RLCH(reg) \
	byte val = (reg & 0xFF00) >> 8; \
	byte c = (val >> 7) & 1; \
	val <<= 1; \
	val |= c; \
	byte z = (val == 0); \
	reg = (val << 8) | (reg & 0xFF);

#define RLCL(reg) \
	byte val = (reg & 0xFF); \
	byte c = (val >> 7) & 1; \
	val <<= 1; \
	val |= c; \
	byte z = (val == 0); \
	reg = (reg & 0xFF00) | val;

#define SLAH(reg) \
	byte val = (reg & 0xFF00) >> 8; \
	byte c = (val >> 7) & 1; \
	val <<= 1; \
	byte z = (val == 0); \
	reg = (val << 8) | (reg & 0xFF);

#define SLAL(reg) \
	byte val = (reg & 0xFF); \
	byte c = (val >> 7) & 1; \
	val <<= 1; \
	byte z = (val == 0); \
	reg = (reg & 0xFF00) | val;

#define SRAH(reg) \
	byte val = (reg & 0xFF00) >> 8; \
	byte c = val & 1; \
	byte msb = (val >> 7) & 1; \
	val >>= 1; \
	val |= (msb << 7); \
	byte z = (val == 0); \
	reg = (val << 8) | (reg & 0xFF);

#define SRAL(reg) \
	byte val = (reg & 0xFF); \
	byte c = (val & 1); \
	byte msb = (val >> 7) & 1; \
	val >>= 1; \
	val |= (msb << 7); \
	byte z = (val == 0); \
	reg = (reg & 0xFF00) | val;

INSTRUCTION(17, {
	//RLA
	byte high = (ctx.af & 0xFF00) >> 8;
	byte cflag = ((ctx.af & 0xFF) >> 4) & 1;
	byte carry = (high >> 7) & 1;
	high <<= 1;
	high |= cflag;
	ctx.af = (high << 8) | (ctx.af & 0xFF);

	SET_FLAGS(0, 0, 0, carry, ctx.af);

	return normalInstructions::cycles[0x17];
});

INSTRUCTION(0F, {
	RRCH(ctx.af);

	SET_FLAGS(0, 0, 0, c, ctx.af);

	return normalInstructions::cycles[0x0F];
	});

INSTRUCTION(1F, {
	//RRA
	RRH(ctx.af);

	SET_FLAGS(0, 0, 0, c, ctx.af);

	return normalInstructions::cycles[0x1F];
});

CB_INSTRUCTION(19, {
	//RR C
	RRL(ctx.bc);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x19];
});

CB_INSTRUCTION(1A, {
	//RR D
	RRH(ctx.de);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x1A];
});

CB_INSTRUCTION(1B, {
	//RR E
	RRL(ctx.de);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x1B];
});

CB_INSTRUCTION(00, {
	RLCH(ctx.bc);

	SET_FLAGS(z, 0, 0, c, ctx.af);
	
	return cbInstructions::cycles[0x00];
	});

CB_INSTRUCTION(01, {
	RLCL(ctx.bc);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x01];
	});

CB_INSTRUCTION(02, {
	RLCH(ctx.de);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x02];
	});

CB_INSTRUCTION(03, {
	RLCL(ctx.de);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x03];
	});

CB_INSTRUCTION(04, {
	RLCH(ctx.hl);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x04];
	});

CB_INSTRUCTION(05, {
	RLCL(ctx.hl);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x05];
	});

CB_INSTRUCTION(06, {
	word low = mem->Read(ctx.hl);
	state->Sync(1);

	RLCL(low);

	mem->Write(ctx.hl, GET_LOW(low));
	state->Sync(1);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x06];
	});

CB_INSTRUCTION(07, {
	RLCH(ctx.af);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x07];
	});

CB_INSTRUCTION(08, {
	RRCH(ctx.bc);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x08];
	});

CB_INSTRUCTION(09, {
	RRCL(ctx.bc);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x09];
	});

CB_INSTRUCTION(0A, {
	RRCH(ctx.de);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x0A];
	});

CB_INSTRUCTION(0B, {
	RRCL(ctx.de);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x0B];
	});

CB_INSTRUCTION(0C, {
	RRCH(ctx.hl);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x0C];
	});

CB_INSTRUCTION(0D, {
	RRCL(ctx.hl);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x0D];
	});

CB_INSTRUCTION(0E, {
	word low = mem->Read(ctx.hl);
	state->Sync(1);

	RRCL(low);

	mem->Write(ctx.hl, GET_LOW(low));
	state->Sync(1);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x0E];
	});

CB_INSTRUCTION(0F, {
	RRCH(ctx.af);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x0F];
	});

CB_INSTRUCTION(10, {
	//RL B
	RLHIGH(ctx.bc);

	SET_FLAGS(zero, 0, 0, carry, ctx.af);

	return cbInstructions::cycles[0x10];
	});

CB_INSTRUCTION(11, {
	//RL C
	RLLOW(ctx.bc);

	SET_FLAGS(zero, 0, 0, carry, ctx.af);

	return cbInstructions::cycles[0x11];
	});

CB_INSTRUCTION(12, {
	//RL D
	RLHIGH(ctx.de);

	SET_FLAGS(zero, 0, 0, carry, ctx.af);

	return cbInstructions::cycles[0x12];
	});

CB_INSTRUCTION(13, {
	//RL E
	RLLOW(ctx.de);

	SET_FLAGS(zero, 0, 0, carry, ctx.af);

	return cbInstructions::cycles[0x13];
	});

CB_INSTRUCTION(14, {
	//RL H
	RLHIGH(ctx.hl);

	SET_FLAGS(zero, 0, 0, carry, ctx.af);

	return cbInstructions::cycles[0x14];
	});

CB_INSTRUCTION(15, {
	//RL L
	RLLOW(ctx.hl);

	SET_FLAGS(zero, 0, 0, carry, ctx.af);

	return cbInstructions::cycles[0x15];
	});

CB_INSTRUCTION(16, {
	//RL (HL)
	word l = mem->Read(ctx.hl);
	state->Sync(1);

	RLLOW(l);

	mem->Write(ctx.hl, GET_LOW(l));
	state->Sync(1);

	SET_FLAGS(zero, 0, 0, carry, ctx.af);

	return cbInstructions::cycles[0x16];
	});

CB_INSTRUCTION(17, {
	//RL A
	RLHIGH(ctx.af);

	SET_FLAGS(zero, 0, 0, carry, ctx.af);

	return cbInstructions::cycles[0x17];
	});

CB_INSTRUCTION(18, {
	//RR B
	RRH(ctx.bc);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x18];
	});

CB_INSTRUCTION(1C, {
	//RR H
	RRH(ctx.hl);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x1C];
	});

CB_INSTRUCTION(1D, {
	//RR L
	RRL(ctx.hl);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x1D];
	});

CB_INSTRUCTION(1E, {
	//RR (HL)
	word l = mem->Read(ctx.hl);
	state->Sync(1);

	RRL(l);

	mem->Write(ctx.hl, GET_LOW(l));
	state->Sync(1);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x1E];
	});

CB_INSTRUCTION(1F, {
	//RR A
	RRH(ctx.af);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x1F];
	});

CB_INSTRUCTION(20, {
	//SLA 
	SLAH(ctx.bc);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x20];
	});

CB_INSTRUCTION(21, {
	//SLA 
	SLAL(ctx.bc);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x21];
	});

CB_INSTRUCTION(22, {
	//SLA 
	SLAH(ctx.de);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x22];
	});

CB_INSTRUCTION(23, {
	//SLA 
	SLAL(ctx.de);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x23];
	});

CB_INSTRUCTION(24, {
	//SLA 
	SLAH(ctx.hl);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x24];
	});

CB_INSTRUCTION(25, {
	//SLA 
	SLAL(ctx.hl);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x25];
	});

CB_INSTRUCTION(26, {
	//SLA 
	word l = mem->Read(ctx.hl);
	state->Sync(1);

	SLAL(l);

	mem->Write(ctx.hl, GET_LOW(l));
	state->Sync(1);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x26];
	});

CB_INSTRUCTION(27, {
	//SLA 
	SLAH(ctx.af);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x27];
	});

CB_INSTRUCTION(28, {
	//SLA 
	SRAH(ctx.bc);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x28];
	});

CB_INSTRUCTION(29, {
	//SLA 
	SRAL(ctx.bc);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x29];
	});

CB_INSTRUCTION(2A, {
	//SLA 
	SRAH(ctx.de);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x2A];
	});

CB_INSTRUCTION(2B, {
	//SLA 
	SRAL(ctx.de);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x2B];
	});

CB_INSTRUCTION(2C, {
	//SLA 
	SRAH(ctx.hl);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x2C];
	});

CB_INSTRUCTION(2D, {
	//SLA 
	SRAL(ctx.hl);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x2D];
	});

CB_INSTRUCTION(2E, {
	//SLA 
	word l = mem->Read(ctx.hl);
	state->Sync(1);

	SRAL(l);

	mem->Write(ctx.hl, GET_LOW(l));
	state->Sync(1);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x2E];
	});

CB_INSTRUCTION(2F, {
	//SLA 
	SRAH(ctx.af);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x2F];
	});

CB_INSTRUCTION(30, {
	SWAPH(ctx.bc);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return cbInstructions::cycles[0x30];
	});

CB_INSTRUCTION(31, {
	SWAPL(ctx.bc);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return cbInstructions::cycles[0x31];
	});

CB_INSTRUCTION(32, {
	SWAPH(ctx.de);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return cbInstructions::cycles[0x32];
	});

CB_INSTRUCTION(33, {
	SWAPL(ctx.de);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return cbInstructions::cycles[0x33];
	});

CB_INSTRUCTION(34, {
	SWAPH(ctx.hl);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return cbInstructions::cycles[0x34];
	});

CB_INSTRUCTION(35, {
	SWAPL(ctx.hl);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return cbInstructions::cycles[0x35];
	});

CB_INSTRUCTION(36, {
	word l = mem->Read(ctx.hl);
	state->Sync(1);

	SWAPL(l);

	mem->Write(ctx.hl, GET_LOW(l));
	state->Sync(1);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return cbInstructions::cycles[0x36];
	});

CB_INSTRUCTION(37, {
	SWAPH(ctx.af);

	SET_FLAGS(z, 0, 0, 0, ctx.af);

	return cbInstructions::cycles[0x37];
	});


CB_INSTRUCTION(38, {
	SRLH(ctx.bc);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x38];
	});

CB_INSTRUCTION(39, {
	SRLL(ctx.bc);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x39];
	});

CB_INSTRUCTION(3A, {
	SRLH(ctx.de);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x3A];
	});

CB_INSTRUCTION(3B, {
	SRLL(ctx.de);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x3B];
	});

CB_INSTRUCTION(3C, {
	SRLH(ctx.hl);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x3C];
	});

CB_INSTRUCTION(3D, {
	SRLL(ctx.hl);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x3D];
	});

CB_INSTRUCTION(3E, {
	word l = mem->Read(ctx.hl);
	state->Sync(1);

	SRLL(l);

	mem->Write(ctx.hl, GET_LOW(l));
	state->Sync(1);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x3E];
	});

CB_INSTRUCTION(3F, {
	SRLH(ctx.af);

	SET_FLAGS(z, 0, 0, c, ctx.af);

	return cbInstructions::cycles[0x3F];
	});

CB_INSTRUCTION(40, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.bc), 0);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x40];
	});

CB_INSTRUCTION(41, {
	//BIT 0
	TEST_BIT(GET_LOW(ctx.bc), 0);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x41];
	});

CB_INSTRUCTION(42, {
	//BIT 0
	TEST_BIT(GET_HIGH(ctx.de), 0);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x42];
	});

CB_INSTRUCTION(43, {
	//BIT 0, B
	TEST_BIT(GET_LOW(ctx.de), 0);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x43];
	});

CB_INSTRUCTION(44, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.hl), 0);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x44];
	});

CB_INSTRUCTION(45, {
	//BIT 0, B
	TEST_BIT(GET_LOW(ctx.hl), 0);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x45];
	});

CB_INSTRUCTION(46, {
	//BIT 0, B
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	TEST_BIT(val, 0);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x46];
	});

CB_INSTRUCTION(47, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.af), 0);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x47];
	});

CB_INSTRUCTION(48, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.bc), 1);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x48];
	});

CB_INSTRUCTION(49, {
	//BIT 0
	TEST_BIT(GET_LOW(ctx.bc), 1);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x49];
	});

CB_INSTRUCTION(4A, {
	//BIT 0
	TEST_BIT(GET_HIGH(ctx.de), 1);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x4A];
	});

CB_INSTRUCTION(4B, {
	//BIT 0, B
	TEST_BIT(GET_LOW(ctx.de), 1);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x4B];
	});

CB_INSTRUCTION(4C, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.hl), 1);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x4C];
	});

CB_INSTRUCTION(4D, {
	//BIT 0, B
	TEST_BIT(GET_LOW(ctx.hl), 1);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x4D];
	});

CB_INSTRUCTION(4E, {
	//BIT 0, B
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	TEST_BIT(val, 1);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x4E];
	});

CB_INSTRUCTION(4F, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.af), 1);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x4F];
	});

CB_INSTRUCTION(50, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.bc), 2);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x50];
	});

CB_INSTRUCTION(51, {
	//BIT 0
	TEST_BIT(GET_LOW(ctx.bc), 2);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x51];
	});

CB_INSTRUCTION(52, {
	//BIT 0
	TEST_BIT(GET_HIGH(ctx.de), 2);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x52];
	});

CB_INSTRUCTION(53, {
	//BIT 0, B
	TEST_BIT(GET_LOW(ctx.de), 2);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x53];
	});

CB_INSTRUCTION(54, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.hl), 2);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x54];
	});

CB_INSTRUCTION(55, {
	//BIT 0, B
	TEST_BIT(GET_LOW(ctx.hl), 2);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x55];
	});

CB_INSTRUCTION(56, {
	//BIT 0, B
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	TEST_BIT(val, 2);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x56];
	});

CB_INSTRUCTION(57, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.af), 2);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x57];
	});

CB_INSTRUCTION(58, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.bc), 3);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x58];
	});

CB_INSTRUCTION(59, {
	//BIT 0
	TEST_BIT(GET_LOW(ctx.bc), 3);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x59];
	});

CB_INSTRUCTION(5A, {
	//BIT 0
	TEST_BIT(GET_HIGH(ctx.de), 3);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x5A];
	});

CB_INSTRUCTION(5B, {
	//BIT 0, B
	TEST_BIT(GET_LOW(ctx.de), 3);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x5B];
	});

CB_INSTRUCTION(5C, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.hl), 3);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x5C];
	});

CB_INSTRUCTION(5D, {
	//BIT 0, B
	TEST_BIT(GET_LOW(ctx.hl), 3);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x5D];
	});

CB_INSTRUCTION(5E, {
	//BIT 0, B
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	TEST_BIT(val, 3);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x5E];
	});

CB_INSTRUCTION(5F, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.af), 3);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x5F];
	});

CB_INSTRUCTION(60, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.bc), 4);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x60];
	});

CB_INSTRUCTION(61, {
	//BIT 0
	TEST_BIT(GET_LOW(ctx.bc), 4);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x61];
	});

CB_INSTRUCTION(62, {
	//BIT 0
	TEST_BIT(GET_HIGH(ctx.de), 4);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x62];
	});

CB_INSTRUCTION(63, {
	//BIT 0, B
	TEST_BIT(GET_LOW(ctx.de), 4);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x63];
	});

CB_INSTRUCTION(64, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.hl), 4);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x64];
	});

CB_INSTRUCTION(65, {
	//BIT 0, B
	TEST_BIT(GET_LOW(ctx.hl), 4);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x65];
	});

CB_INSTRUCTION(66, {
	//BIT 0, B
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	TEST_BIT(val, 4);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x66];
	});

CB_INSTRUCTION(67, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.af), 4);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x67];
	});

CB_INSTRUCTION(68, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.bc), 5);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x68];
	});

CB_INSTRUCTION(69, {
	//BIT 0
	TEST_BIT(GET_LOW(ctx.bc), 5);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x69];
	});

CB_INSTRUCTION(6A, {
	//BIT 0
	TEST_BIT(GET_HIGH(ctx.de), 5);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x6A];
	});

CB_INSTRUCTION(6B, {
	//BIT 0, B
	TEST_BIT(GET_LOW(ctx.de), 5);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x6B];
	});

CB_INSTRUCTION(6C, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.hl), 5);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x6C];
	});

CB_INSTRUCTION(6D, {
	//BIT 0, B
	TEST_BIT(GET_LOW(ctx.hl), 5);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x6D];
	});

CB_INSTRUCTION(6E, {
	//BIT 0, B
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	TEST_BIT(val, 5);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x6E];
	});

CB_INSTRUCTION(6F, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.af), 5);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x6F];
	});

CB_INSTRUCTION(70, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.bc), 6);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x70];
	});

CB_INSTRUCTION(71, {
	//BIT 0
	TEST_BIT(GET_LOW(ctx.bc), 6);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x71];
	});

CB_INSTRUCTION(72, {
	//BIT 0
	TEST_BIT(GET_HIGH(ctx.de), 6);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x72];
	});

CB_INSTRUCTION(73, {
	//BIT 0, B
	TEST_BIT(GET_LOW(ctx.de), 6);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x73];
	});

CB_INSTRUCTION(74, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.hl), 6);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x74];
	});

CB_INSTRUCTION(75, {
	//BIT 0, B
	TEST_BIT(GET_LOW(ctx.hl), 6);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x75];
	});

CB_INSTRUCTION(76, {
	//BIT 0, B
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	TEST_BIT(val, 6);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x76];
	});

CB_INSTRUCTION(77, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.af), 6);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x77];
	});

CB_INSTRUCTION(78, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.bc), 7);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x78];
	});

CB_INSTRUCTION(79, {
	//BIT 0
	TEST_BIT(GET_LOW(ctx.bc), 7);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x79];
	});

CB_INSTRUCTION(7A, {
	//BIT 0
	TEST_BIT(GET_HIGH(ctx.de), 7);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x7A];
	});

CB_INSTRUCTION(7B, {
	//BIT 0, B
	TEST_BIT(GET_LOW(ctx.de), 7);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x7B];
	});

CB_INSTRUCTION(7C, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.hl), 7);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x7C];
	});

CB_INSTRUCTION(7D, {
	//BIT 0, B
	TEST_BIT(GET_LOW(ctx.hl), 7);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x7D];
	});

CB_INSTRUCTION(7E, {
	//BIT 0, B
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	TEST_BIT(val, 7);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x7E];
	});

CB_INSTRUCTION(7F, {
	//BIT 0, B
	TEST_BIT(GET_HIGH(ctx.af), 7);

	SET_FLAGS(z_flag, 0, 1, c_flag, ctx.af);

	return cbInstructions::cycles[0x7F];
	});

#define RESET(reg, bit) \
	reg &= ~(1 << bit);

#define SET(reg, bit) \
	reg |= (1 << bit);

CB_INSTRUCTION(80, {
	byte val = GET_HIGH(ctx.bc);

	RESET(val, 0);

	SET_HIGH(ctx.bc, val);

	return cbInstructions::cycles[0x80];
	});

CB_INSTRUCTION(81, {
	byte val = GET_LOW(ctx.bc);

	RESET(val, 0);

	SET_LOW(ctx.bc, val);

	return cbInstructions::cycles[0x81];
	});

CB_INSTRUCTION(82, {
	byte val = GET_HIGH(ctx.de);

	RESET(val, 0);

	SET_HIGH(ctx.de, val);

	return cbInstructions::cycles[0x82];
	});

CB_INSTRUCTION(83, {
	byte val = GET_LOW(ctx.de);

	RESET(val, 0);

	SET_LOW(ctx.de, val);

	return cbInstructions::cycles[0x83];
	});

CB_INSTRUCTION(84, {
	byte val = GET_HIGH(ctx.hl);

	RESET(val, 0);

	SET_HIGH(ctx.hl, val);

	return cbInstructions::cycles[0x84];
	});

CB_INSTRUCTION(85, {
	byte val = GET_LOW(ctx.hl);

	RESET(val, 0);

	SET_LOW(ctx.hl, val);

	return cbInstructions::cycles[0x85];
	});

CB_INSTRUCTION(86, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	RESET(val, 0);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	return cbInstructions::cycles[0x86];
	});

CB_INSTRUCTION(87, {
	byte val = GET_HIGH(ctx.af);

	RESET(val, 0);

	SET_HIGH(ctx.af, val);

	return cbInstructions::cycles[0x87];
	});

CB_INSTRUCTION(88, {
	byte val = GET_HIGH(ctx.bc);

	RESET(val, 1);

	SET_HIGH(ctx.bc, val);

	return cbInstructions::cycles[0x88];
	});

CB_INSTRUCTION(89, {
	byte val = GET_LOW(ctx.bc);

	RESET(val, 1);

	SET_LOW(ctx.bc, val);

	return cbInstructions::cycles[0x89];
	});

CB_INSTRUCTION(8A, {
	byte val = GET_HIGH(ctx.de);

	RESET(val, 1);

	SET_HIGH(ctx.de, val);

	return cbInstructions::cycles[0x8A];
	});

CB_INSTRUCTION(8B, {
	byte val = GET_LOW(ctx.de);

	RESET(val, 1);

	SET_LOW(ctx.de, val);

	return cbInstructions::cycles[0x8B];
	});

CB_INSTRUCTION(8C, {
	byte val = GET_HIGH(ctx.hl);

	RESET(val, 1);

	SET_HIGH(ctx.hl, val);

	return cbInstructions::cycles[0x8C];
	});

CB_INSTRUCTION(8D, {
	byte val = GET_LOW(ctx.hl);

	RESET(val, 1);

	SET_LOW(ctx.hl, val);

	return cbInstructions::cycles[0x8D];
	});

CB_INSTRUCTION(8E, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	RESET(val, 1);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	return cbInstructions::cycles[0x8E];
	});

CB_INSTRUCTION(8F, {
	byte val = GET_HIGH(ctx.af);

	RESET(val, 1);

	SET_HIGH(ctx.af, val);

	return cbInstructions::cycles[0x8F];
	});

CB_INSTRUCTION(90, {
	byte val = GET_HIGH(ctx.bc);

	RESET(val, 2);

	SET_HIGH(ctx.bc, val);

	return cbInstructions::cycles[0x90];
	});

CB_INSTRUCTION(91, {
	byte val = GET_LOW(ctx.bc);

	RESET(val, 2);

	SET_LOW(ctx.bc, val);

	return cbInstructions::cycles[0x91];
	});

CB_INSTRUCTION(92, {
	byte val = GET_HIGH(ctx.de);

	RESET(val, 2);

	SET_HIGH(ctx.de, val);

	return cbInstructions::cycles[0x92];
	});

CB_INSTRUCTION(93, {
	byte val = GET_LOW(ctx.de);

	RESET(val, 2);

	SET_LOW(ctx.de, val);

	return cbInstructions::cycles[0x93];
	});

CB_INSTRUCTION(94, {
	byte val = GET_HIGH(ctx.hl);

	RESET(val, 2);

	SET_HIGH(ctx.hl, val);

	return cbInstructions::cycles[0x94];
	});

CB_INSTRUCTION(95, {
	byte val = GET_LOW(ctx.hl);

	RESET(val, 2);

	SET_LOW(ctx.hl, val);

	return cbInstructions::cycles[0x95];
	});

CB_INSTRUCTION(96, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	RESET(val, 2);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	return cbInstructions::cycles[0x96];
	});

CB_INSTRUCTION(97, {
	byte val = GET_HIGH(ctx.af);

	RESET(val, 2);

	SET_HIGH(ctx.af, val);

	return cbInstructions::cycles[0x97];
	});

CB_INSTRUCTION(98, {
	byte val = GET_HIGH(ctx.bc);

	RESET(val, 3);

	SET_HIGH(ctx.bc, val);

	return cbInstructions::cycles[0x98];
	});

CB_INSTRUCTION(99, {
	byte val = GET_LOW(ctx.bc);

	RESET(val, 3);

	SET_LOW(ctx.bc, val);

	return cbInstructions::cycles[0x99];
	});

CB_INSTRUCTION(9A, {
	byte val = GET_HIGH(ctx.de);

	RESET(val, 3);

	SET_HIGH(ctx.de, val);

	return cbInstructions::cycles[0x9A];
	});

CB_INSTRUCTION(9B, {
	byte val = GET_LOW(ctx.de);

	RESET(val, 3);

	SET_LOW(ctx.de, val);

	return cbInstructions::cycles[0x9B];
	});

CB_INSTRUCTION(9C, {
	byte val = GET_HIGH(ctx.hl);

	RESET(val, 3);

	SET_HIGH(ctx.hl, val);

	return cbInstructions::cycles[0x9C];
	});

CB_INSTRUCTION(9D, {
	byte val = GET_LOW(ctx.hl);

	RESET(val, 3);

	SET_LOW(ctx.hl, val);

	return cbInstructions::cycles[0x9D];
	});

CB_INSTRUCTION(9E, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	RESET(val, 3);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	return cbInstructions::cycles[0x9E];
	});

CB_INSTRUCTION(9F, {
	byte val = GET_HIGH(ctx.af);

	RESET(val, 3);

	SET_HIGH(ctx.af, val);

	return cbInstructions::cycles[0x9F];
	});

CB_INSTRUCTION(A0, {
	byte val = GET_HIGH(ctx.bc);

	RESET(val, 4);

	SET_HIGH(ctx.bc, val);

	return cbInstructions::cycles[0xA0];
	});

CB_INSTRUCTION(A1, {
	byte val = GET_LOW(ctx.bc);

	RESET(val, 4);

	SET_LOW(ctx.bc, val);

	return cbInstructions::cycles[0xA1];
	});

CB_INSTRUCTION(A2, {
	byte val = GET_HIGH(ctx.de);

	RESET(val, 4);

	SET_HIGH(ctx.de, val);

	return cbInstructions::cycles[0xA2];
	});

CB_INSTRUCTION(A3, {
	byte val = GET_LOW(ctx.de);

	RESET(val, 4);

	SET_LOW(ctx.de, val);

	return cbInstructions::cycles[0xA3];
	});

CB_INSTRUCTION(A4, {
	byte val = GET_HIGH(ctx.hl);

	RESET(val, 4);

	SET_HIGH(ctx.hl, val);

	return cbInstructions::cycles[0xA4];
	});

CB_INSTRUCTION(A5, {
	byte val = GET_LOW(ctx.hl);

	RESET(val, 4);

	SET_LOW(ctx.hl, val);

	return cbInstructions::cycles[0xA5];
	});

CB_INSTRUCTION(A6, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	RESET(val, 4);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	return cbInstructions::cycles[0xA6];
	});

CB_INSTRUCTION(A7, {
	byte val = GET_HIGH(ctx.af);

	RESET(val, 4);

	SET_HIGH(ctx.af, val);

	return cbInstructions::cycles[0xA7];
	});

CB_INSTRUCTION(A8, {
	byte val = GET_HIGH(ctx.bc);

	RESET(val, 5);

	SET_HIGH(ctx.bc, val);

	return cbInstructions::cycles[0xA8];
	});

CB_INSTRUCTION(A9, {
	byte val = GET_LOW(ctx.bc);

	RESET(val, 5);

	SET_LOW(ctx.bc, val);

	return cbInstructions::cycles[0xA9];
	});

CB_INSTRUCTION(AA, {
	byte val = GET_HIGH(ctx.de);

	RESET(val, 5);

	SET_HIGH(ctx.de, val);

	return cbInstructions::cycles[0xAA];
	});

CB_INSTRUCTION(AB, {
	byte val = GET_LOW(ctx.de);

	RESET(val, 5);

	SET_LOW(ctx.de, val);

	return cbInstructions::cycles[0xAB];
	});

CB_INSTRUCTION(AC, {
	byte val = GET_HIGH(ctx.hl);

	RESET(val, 5);

	SET_HIGH(ctx.hl, val);

	return cbInstructions::cycles[0xAC];
	});

CB_INSTRUCTION(AD, {
	byte val = GET_LOW(ctx.hl);

	RESET(val, 5);

	SET_LOW(ctx.hl, val);

	return cbInstructions::cycles[0xAD];
	});

CB_INSTRUCTION(AE, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	RESET(val, 5);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	return cbInstructions::cycles[0xAE];
	});

CB_INSTRUCTION(AF, {
	byte val = GET_HIGH(ctx.af);

	RESET(val, 5);

	SET_HIGH(ctx.af, val);

	return cbInstructions::cycles[0xAF];
	});

CB_INSTRUCTION(B0, {
	byte val = GET_HIGH(ctx.bc);

	RESET(val, 6);

	SET_HIGH(ctx.bc, val);

	return cbInstructions::cycles[0xB0];
	});

CB_INSTRUCTION(B1, {
	byte val = GET_LOW(ctx.bc);

	RESET(val, 6);

	SET_LOW(ctx.bc, val);

	return cbInstructions::cycles[0xB1];
	});

CB_INSTRUCTION(B2, {
	byte val = GET_HIGH(ctx.de);

	RESET(val, 6);

	SET_HIGH(ctx.de, val);

	return cbInstructions::cycles[0xB2];
	});

CB_INSTRUCTION(B3, {
	byte val = GET_LOW(ctx.de);

	RESET(val, 6);

	SET_LOW(ctx.de, val);

	return cbInstructions::cycles[0xB3];
	});

CB_INSTRUCTION(B4, {
	byte val = GET_HIGH(ctx.hl);

	RESET(val, 6);

	SET_HIGH(ctx.hl, val);

	return cbInstructions::cycles[0xB4];
	});

CB_INSTRUCTION(B5, {
	byte val = GET_LOW(ctx.hl);

	RESET(val, 6);

	SET_LOW(ctx.hl, val);

	return cbInstructions::cycles[0xB5];
	});

CB_INSTRUCTION(B6, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	RESET(val, 6);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	return cbInstructions::cycles[0xB6];
	});

CB_INSTRUCTION(B7, {
	byte val = GET_HIGH(ctx.af);

	RESET(val, 6);

	SET_HIGH(ctx.af, val);

	return cbInstructions::cycles[0xB7];
	});

CB_INSTRUCTION(B8, {
	byte val = GET_HIGH(ctx.bc);

	RESET(val, 7);

	SET_HIGH(ctx.bc, val);

	return cbInstructions::cycles[0xB8];
	});

CB_INSTRUCTION(B9, {
	byte val = GET_LOW(ctx.bc);

	RESET(val, 7);

	SET_LOW(ctx.bc, val);

	return cbInstructions::cycles[0xB9];
	});

CB_INSTRUCTION(BA, {
	byte val = GET_HIGH(ctx.de);

	RESET(val, 7);

	SET_HIGH(ctx.de, val);

	return cbInstructions::cycles[0xBA];
	});

CB_INSTRUCTION(BB, {
	byte val = GET_LOW(ctx.de);

	RESET(val, 7);

	SET_LOW(ctx.de, val);

	return cbInstructions::cycles[0xBB];
	});

CB_INSTRUCTION(BC, {
	byte val = GET_HIGH(ctx.hl);

	RESET(val, 7);

	SET_HIGH(ctx.hl, val);

	return cbInstructions::cycles[0xBC];
	});

CB_INSTRUCTION(BD, {
	byte val = GET_LOW(ctx.hl);

	RESET(val, 7);

	SET_LOW(ctx.hl, val);

	return cbInstructions::cycles[0xBD];
	});

CB_INSTRUCTION(BE, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	RESET(val, 7);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	return cbInstructions::cycles[0xBE];
	});

CB_INSTRUCTION(BF, {
	byte val = GET_HIGH(ctx.af);

	RESET(val, 7);

	SET_HIGH(ctx.af, val);

	return cbInstructions::cycles[0xBF];
	});

CB_INSTRUCTION(C0, {
	byte val = GET_HIGH(ctx.bc);

	SET(val, 0);

	SET_HIGH(ctx.bc, val);

	return cbInstructions::cycles[0xC0];
	});

CB_INSTRUCTION(C1, {
	byte val = GET_LOW(ctx.bc);

	SET(val, 0);

	SET_LOW(ctx.bc, val);

	return cbInstructions::cycles[0xC1];
	});

CB_INSTRUCTION(C2, {
	byte val = GET_HIGH(ctx.de);

	SET(val, 0);

	SET_HIGH(ctx.de, val);

	return cbInstructions::cycles[0xC2];
	});

CB_INSTRUCTION(C3, {
	byte val = GET_LOW(ctx.de);

	SET(val, 0);

	SET_LOW(ctx.de, val);

	return cbInstructions::cycles[0xC3];
	});

CB_INSTRUCTION(C4, {
	byte val = GET_HIGH(ctx.hl);

	SET(val, 0);

	SET_HIGH(ctx.hl, val);

	return cbInstructions::cycles[0xC4];
	});

CB_INSTRUCTION(C5, {
	byte val = GET_LOW(ctx.hl);

	SET(val, 0);

	SET_LOW(ctx.hl, val);

	return cbInstructions::cycles[0xC5];
	});

CB_INSTRUCTION(C6, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	SET(val, 0);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	return cbInstructions::cycles[0xC6];
	});

CB_INSTRUCTION(C7, {
	byte val = GET_HIGH(ctx.af);

	SET(val, 0);

	SET_HIGH(ctx.af, val);

	return cbInstructions::cycles[0xC7];
	});

CB_INSTRUCTION(C8, {
	byte val = GET_HIGH(ctx.bc);

	SET(val, 1);

	SET_HIGH(ctx.bc, val);

	return cbInstructions::cycles[0xC8];
	});

CB_INSTRUCTION(C9, {
	byte val = GET_LOW(ctx.bc);

	SET(val, 1);

	SET_LOW(ctx.bc, val);

	return cbInstructions::cycles[0xC9];
	});

CB_INSTRUCTION(CA, {
	byte val = GET_HIGH(ctx.de);

	SET(val, 1);

	SET_HIGH(ctx.de, val);

	return cbInstructions::cycles[0xCA];
	});

CB_INSTRUCTION(CB, {
	byte val = GET_LOW(ctx.de);

	SET(val, 1);

	SET_LOW(ctx.de, val);

	return cbInstructions::cycles[0xCB];
	});

CB_INSTRUCTION(CC, {
	byte val = GET_HIGH(ctx.hl);

	SET(val, 1);

	SET_HIGH(ctx.hl, val);

	return cbInstructions::cycles[0xCC];
	});

CB_INSTRUCTION(CD, {
	byte val = GET_LOW(ctx.hl);

	SET(val, 1);

	SET_LOW(ctx.hl, val);

	return cbInstructions::cycles[0xCD];
	});

CB_INSTRUCTION(CE, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	SET(val, 1);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	return cbInstructions::cycles[0xCE];
	});

CB_INSTRUCTION(CF, {
	byte val = GET_HIGH(ctx.af);

	SET(val, 1);

	SET_HIGH(ctx.af, val);

	return cbInstructions::cycles[0xCF];
	});

CB_INSTRUCTION(D0, {
	byte val = GET_HIGH(ctx.bc);

	SET(val, 2);

	SET_HIGH(ctx.bc, val);

	return cbInstructions::cycles[0xD0];
	});

CB_INSTRUCTION(D1, {
	byte val = GET_LOW(ctx.bc);

	SET(val, 2);

	SET_LOW(ctx.bc, val);

	return cbInstructions::cycles[0xD1];
	});

CB_INSTRUCTION(D2, {
	byte val = GET_HIGH(ctx.de);

	SET(val, 2);

	SET_HIGH(ctx.de, val);

	return cbInstructions::cycles[0xD2];
	});

CB_INSTRUCTION(D3, {
	byte val = GET_LOW(ctx.de);

	SET(val, 2);

	SET_LOW(ctx.de, val);

	return cbInstructions::cycles[0xD3];
	});

CB_INSTRUCTION(D4, {
	byte val = GET_HIGH(ctx.hl);

	SET(val, 2);

	SET_HIGH(ctx.hl, val);

	return cbInstructions::cycles[0xD4];
	});

CB_INSTRUCTION(D5, {
	byte val = GET_LOW(ctx.hl);

	SET(val, 2);

	SET_LOW(ctx.hl, val);

	return cbInstructions::cycles[0xD5];
	});

CB_INSTRUCTION(D6, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	SET(val, 2);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	return cbInstructions::cycles[0xD6];
	});

CB_INSTRUCTION(D7, {
	byte val = GET_HIGH(ctx.af);

	SET(val, 2);

	SET_HIGH(ctx.af, val);

	return cbInstructions::cycles[0xD7];
	});

CB_INSTRUCTION(D8, {
	byte val = GET_HIGH(ctx.bc);

	SET(val, 3);

	SET_HIGH(ctx.bc, val);

	return cbInstructions::cycles[0xD8];
	});

CB_INSTRUCTION(D9, {
	byte val = GET_LOW(ctx.bc);

	SET(val, 3);

	SET_LOW(ctx.bc, val);

	return cbInstructions::cycles[0xD9];
	});

CB_INSTRUCTION(DA, {
	byte val = GET_HIGH(ctx.de);

	SET(val, 3);

	SET_HIGH(ctx.de, val);

	return cbInstructions::cycles[0xDA];
	});

CB_INSTRUCTION(DB, {
	byte val = GET_LOW(ctx.de);

	SET(val, 3);

	SET_LOW(ctx.de, val);

	return cbInstructions::cycles[0xDB];
	});

CB_INSTRUCTION(DC, {
	byte val = GET_HIGH(ctx.hl);

	SET(val, 3);

	SET_HIGH(ctx.hl, val);

	return cbInstructions::cycles[0xDC];
	});

CB_INSTRUCTION(DD, {
	byte val = GET_LOW(ctx.hl);

	SET(val, 3);

	SET_LOW(ctx.hl, val);

	return cbInstructions::cycles[0xDD];
	});

CB_INSTRUCTION(DE, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	SET(val, 3);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	return cbInstructions::cycles[0xDE];
	});

CB_INSTRUCTION(DF, {
	byte val = GET_HIGH(ctx.af);

	SET(val, 3);

	SET_HIGH(ctx.af, val);

	return cbInstructions::cycles[0xDF];
	});

CB_INSTRUCTION(E0, {
	byte val = GET_HIGH(ctx.bc);

	SET(val, 4);

	SET_HIGH(ctx.bc, val);

	return cbInstructions::cycles[0xE0];
	});

CB_INSTRUCTION(E1, {
	byte val = GET_LOW(ctx.bc);

	SET(val, 4);

	SET_LOW(ctx.bc, val);

	return cbInstructions::cycles[0xE1];
	});

CB_INSTRUCTION(E2, {
	byte val = GET_HIGH(ctx.de);

	SET(val, 4);

	SET_HIGH(ctx.de, val);

	return cbInstructions::cycles[0xE2];
	});

CB_INSTRUCTION(E3, {
	byte val = GET_LOW(ctx.de);

	SET(val, 4);

	SET_LOW(ctx.de, val);

	return cbInstructions::cycles[0xE3];
	});

CB_INSTRUCTION(E4, {
	byte val = GET_HIGH(ctx.hl);

	SET(val, 4);

	SET_HIGH(ctx.hl, val);

	return cbInstructions::cycles[0xE4];
	});

CB_INSTRUCTION(E5, {
	byte val = GET_LOW(ctx.hl);

	SET(val, 4);

	SET_LOW(ctx.hl, val);

	return cbInstructions::cycles[0xE5];
	});

CB_INSTRUCTION(E6, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	SET(val, 4);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	return cbInstructions::cycles[0xE6];
	});

CB_INSTRUCTION(E7, {
	byte val = GET_HIGH(ctx.af);

	SET(val, 4);

	SET_HIGH(ctx.af, val);

	return cbInstructions::cycles[0xE7];
	});

CB_INSTRUCTION(E8, {
	byte val = GET_HIGH(ctx.bc);

	SET(val, 5);

	SET_HIGH(ctx.bc, val);

	return cbInstructions::cycles[0xE8];
	});

CB_INSTRUCTION(E9, {
	byte val = GET_LOW(ctx.bc);

	SET(val, 5);

	SET_LOW(ctx.bc, val);

	return cbInstructions::cycles[0xE9];
	});

CB_INSTRUCTION(EA, {
	byte val = GET_HIGH(ctx.de);

	SET(val, 5);

	SET_HIGH(ctx.de, val);

	return cbInstructions::cycles[0xEA];
	});

CB_INSTRUCTION(EB, {
	byte val = GET_LOW(ctx.de);

	SET(val, 5);

	SET_LOW(ctx.de, val);

	return cbInstructions::cycles[0xEB];
	});

CB_INSTRUCTION(EC, {
	byte val = GET_HIGH(ctx.hl);

	SET(val, 5);

	SET_HIGH(ctx.hl, val);

	return cbInstructions::cycles[0xEC];
	});

CB_INSTRUCTION(ED, {
	byte val = GET_LOW(ctx.hl);

	SET(val, 5);

	SET_LOW(ctx.hl, val);

	return cbInstructions::cycles[0xED];
	});

CB_INSTRUCTION(EE, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	SET(val, 5);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	return cbInstructions::cycles[0xEE];
	});

CB_INSTRUCTION(EF, {
	byte val = GET_HIGH(ctx.af);

	SET(val, 5);

	SET_HIGH(ctx.af, val);

	return cbInstructions::cycles[0xEF];
	});

CB_INSTRUCTION(F0, {
	byte val = GET_HIGH(ctx.bc);

	SET(val, 6);

	SET_HIGH(ctx.bc, val);

	return cbInstructions::cycles[0xF0];
	});

CB_INSTRUCTION(F1, {
	byte val = GET_LOW(ctx.bc);

	SET(val, 6);

	SET_LOW(ctx.bc, val);

	return cbInstructions::cycles[0xF1];
	});

CB_INSTRUCTION(F2, {
	byte val = GET_HIGH(ctx.de);

	SET(val, 6);

	SET_HIGH(ctx.de, val);

	return cbInstructions::cycles[0xF2];
	});

CB_INSTRUCTION(F3, {
	byte val = GET_LOW(ctx.de);

	SET(val, 6);

	SET_LOW(ctx.de, val);

	return cbInstructions::cycles[0xF3];
	});

CB_INSTRUCTION(F4, {
	byte val = GET_HIGH(ctx.hl);

	SET(val, 6);

	SET_HIGH(ctx.hl, val);

	return cbInstructions::cycles[0xF4];
	});

CB_INSTRUCTION(F5, {
	byte val = GET_LOW(ctx.hl);

	SET(val, 6);

	SET_LOW(ctx.hl, val);

	return cbInstructions::cycles[0xF5];
	});

CB_INSTRUCTION(F6, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	SET(val, 6);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	return cbInstructions::cycles[0xF6];
	});

CB_INSTRUCTION(F7, {
	byte val = GET_HIGH(ctx.af);

	SET(val, 6);

	SET_HIGH(ctx.af, val);

	return cbInstructions::cycles[0xF7];
	});

CB_INSTRUCTION(F8, {
	byte val = GET_HIGH(ctx.bc);

	SET(val, 7);

	SET_HIGH(ctx.bc, val);

	return cbInstructions::cycles[0xF8];
	});

CB_INSTRUCTION(F9, {
	byte val = GET_LOW(ctx.bc);

	SET(val, 7);

	SET_LOW(ctx.bc, val);

	return cbInstructions::cycles[0xF9];
	});

CB_INSTRUCTION(FA, {
	byte val = GET_HIGH(ctx.de);

	SET(val, 7);

	SET_HIGH(ctx.de, val);

	return cbInstructions::cycles[0xFA];
	});

CB_INSTRUCTION(FB, {
	byte val = GET_LOW(ctx.de);

	SET(val, 7);

	SET_LOW(ctx.de, val);

	return cbInstructions::cycles[0xFB];
	});

CB_INSTRUCTION(FC, {
	byte val = GET_HIGH(ctx.hl);

	SET(val, 7);

	SET_HIGH(ctx.hl, val);

	return cbInstructions::cycles[0xFC];
	});

CB_INSTRUCTION(FD, {
	byte val = GET_LOW(ctx.hl);

	SET(val, 7);

	SET_LOW(ctx.hl, val);

	return cbInstructions::cycles[0xFD];
	});

CB_INSTRUCTION(FE, {
	byte val = mem->Read(ctx.hl);
	state->Sync(1);

	SET(val, 7);

	mem->Write(ctx.hl, val);
	state->Sync(1);

	return cbInstructions::cycles[0xFE];
	});

CB_INSTRUCTION(FF, {
	byte val = GET_HIGH(ctx.af);

	SET(val, 7);

	SET_HIGH(ctx.af, val);

	return cbInstructions::cycles[0xFF];
	});


void init_cb_instructions() {
	cb_table = new jmp_type[256];

	std::fill_n(cb_table, 256, nullptr);

	cb_table[0x11] = CB_INT_PTR(11);
	cb_table[0x00] = CB_INT_PTR(00);
	cb_table[0x01] = CB_INT_PTR(01);
	cb_table[0x02] = CB_INT_PTR(02);
	cb_table[0x03] = CB_INT_PTR(03);
	cb_table[0x04] = CB_INT_PTR(04);
	cb_table[0x05] = CB_INT_PTR(05);
	cb_table[0x06] = CB_INT_PTR(06);
	cb_table[0x07] = CB_INT_PTR(07);
	cb_table[0x08] = CB_INT_PTR(08);
	cb_table[0x09] = CB_INT_PTR(09);
	cb_table[0x0A] = CB_INT_PTR(0A);
	cb_table[0x0B] = CB_INT_PTR(0B);
	cb_table[0x0C] = CB_INT_PTR(0C);
	cb_table[0x0D] = CB_INT_PTR(0D);
	cb_table[0x0E] = CB_INT_PTR(0E);
	cb_table[0x0F] = CB_INT_PTR(0F);
	cb_table[0x10] = CB_INT_PTR(10);
	cb_table[0x12] = CB_INT_PTR(12);
	cb_table[0x13] = CB_INT_PTR(13);
	cb_table[0x14] = CB_INT_PTR(14);
	cb_table[0x15] = CB_INT_PTR(15);
	cb_table[0x16] = CB_INT_PTR(16);
	cb_table[0x17] = CB_INT_PTR(17);
	cb_table[0x18] = CB_INT_PTR(18);
	cb_table[0x19] = CB_INT_PTR(19);
	cb_table[0x1A] = CB_INT_PTR(1A);
	cb_table[0x1B] = CB_INT_PTR(1B);
	cb_table[0x1C] = CB_INT_PTR(1C);
	cb_table[0x1D] = CB_INT_PTR(1D);
	cb_table[0x1E] = CB_INT_PTR(1E);
	cb_table[0x1F] = CB_INT_PTR(1F);
	cb_table[0x20] = CB_INT_PTR(20);
	cb_table[0x21] = CB_INT_PTR(21);
	cb_table[0x22] = CB_INT_PTR(22);
	cb_table[0x23] = CB_INT_PTR(23);
	cb_table[0x24] = CB_INT_PTR(24);
	cb_table[0x25] = CB_INT_PTR(25);
	cb_table[0x26] = CB_INT_PTR(26);
	cb_table[0x27] = CB_INT_PTR(27);
	cb_table[0x28] = CB_INT_PTR(28);
	cb_table[0x29] = CB_INT_PTR(29);
	cb_table[0x2A] = CB_INT_PTR(2A);
	cb_table[0x2B] = CB_INT_PTR(2B);
	cb_table[0x2C] = CB_INT_PTR(2C);
	cb_table[0x2D] = CB_INT_PTR(2D);
	cb_table[0x2E] = CB_INT_PTR(2E);
	cb_table[0x2F] = CB_INT_PTR(2F);
	cb_table[0x30] = CB_INT_PTR(30);
	cb_table[0x31] = CB_INT_PTR(31);
	cb_table[0x32] = CB_INT_PTR(32);
	cb_table[0x33] = CB_INT_PTR(33);
	cb_table[0x34] = CB_INT_PTR(34);
	cb_table[0x35] = CB_INT_PTR(35);
	cb_table[0x36] = CB_INT_PTR(36);
	cb_table[0x37] = CB_INT_PTR(37);
	cb_table[0x38] = CB_INT_PTR(38);
	cb_table[0x39] = CB_INT_PTR(39);
	cb_table[0x3A] = CB_INT_PTR(3A);
	cb_table[0x3B] = CB_INT_PTR(3B);
	cb_table[0x3C] = CB_INT_PTR(3C);
	cb_table[0x3D] = CB_INT_PTR(3D);
	cb_table[0x3E] = CB_INT_PTR(3E);
	cb_table[0x3F] = CB_INT_PTR(3F);

	cb_table[0x40] = CB_INT_PTR(40);
	cb_table[0x41] = CB_INT_PTR(41);
	cb_table[0x42] = CB_INT_PTR(42);
	cb_table[0x43] = CB_INT_PTR(43);
	cb_table[0x44] = CB_INT_PTR(44);
	cb_table[0x45] = CB_INT_PTR(45);
	cb_table[0x46] = CB_INT_PTR(46);
	cb_table[0x47] = CB_INT_PTR(47);
	cb_table[0x48] = CB_INT_PTR(48);
	cb_table[0x49] = CB_INT_PTR(49);
	cb_table[0x4A] = CB_INT_PTR(4A);
	cb_table[0x4B] = CB_INT_PTR(4B);
	cb_table[0x4C] = CB_INT_PTR(4C);
	cb_table[0x4D] = CB_INT_PTR(4D);
	cb_table[0x4E] = CB_INT_PTR(4E);
	cb_table[0x4F] = CB_INT_PTR(4F);

	cb_table[0x50] = CB_INT_PTR(50);
	cb_table[0x51] = CB_INT_PTR(51);
	cb_table[0x52] = CB_INT_PTR(52);
	cb_table[0x53] = CB_INT_PTR(53);
	cb_table[0x54] = CB_INT_PTR(54);
	cb_table[0x55] = CB_INT_PTR(55);
	cb_table[0x56] = CB_INT_PTR(56);
	cb_table[0x57] = CB_INT_PTR(57);
	cb_table[0x58] = CB_INT_PTR(58);
	cb_table[0x59] = CB_INT_PTR(59);
	cb_table[0x5A] = CB_INT_PTR(5A);
	cb_table[0x5B] = CB_INT_PTR(5B);
	cb_table[0x5C] = CB_INT_PTR(5C);
	cb_table[0x5D] = CB_INT_PTR(5D);
	cb_table[0x5E] = CB_INT_PTR(5E);
	cb_table[0x5F] = CB_INT_PTR(5F);

	cb_table[0x60] = CB_INT_PTR(60);
	cb_table[0x61] = CB_INT_PTR(61);
	cb_table[0x62] = CB_INT_PTR(62);
	cb_table[0x63] = CB_INT_PTR(63);
	cb_table[0x64] = CB_INT_PTR(64);
	cb_table[0x65] = CB_INT_PTR(65);
	cb_table[0x66] = CB_INT_PTR(66);
	cb_table[0x67] = CB_INT_PTR(67);
	cb_table[0x68] = CB_INT_PTR(68);
	cb_table[0x69] = CB_INT_PTR(69);
	cb_table[0x6A] = CB_INT_PTR(6A);
	cb_table[0x6B] = CB_INT_PTR(6B);
	cb_table[0x6C] = CB_INT_PTR(6C);
	cb_table[0x6D] = CB_INT_PTR(6D);
	cb_table[0x6E] = CB_INT_PTR(6E);
	cb_table[0x6F] = CB_INT_PTR(6F);

	cb_table[0x70] = CB_INT_PTR(70);
	cb_table[0x71] = CB_INT_PTR(71);
	cb_table[0x72] = CB_INT_PTR(72);
	cb_table[0x73] = CB_INT_PTR(73);
	cb_table[0x74] = CB_INT_PTR(74);
	cb_table[0x75] = CB_INT_PTR(75);
	cb_table[0x76] = CB_INT_PTR(76);
	cb_table[0x77] = CB_INT_PTR(77);
	cb_table[0x78] = CB_INT_PTR(78);
	cb_table[0x79] = CB_INT_PTR(79);
	cb_table[0x7A] = CB_INT_PTR(7A);
	cb_table[0x7B] = CB_INT_PTR(7B);
	cb_table[0x7C] = CB_INT_PTR(7C);
	cb_table[0x7D] = CB_INT_PTR(7D);
	cb_table[0x7E] = CB_INT_PTR(7E);
	cb_table[0x7F] = CB_INT_PTR(7F);

	cb_table[0x80] = CB_INT_PTR(80);
	cb_table[0x81] = CB_INT_PTR(81);
	cb_table[0x82] = CB_INT_PTR(82);
	cb_table[0x83] = CB_INT_PTR(83);
	cb_table[0x84] = CB_INT_PTR(84);
	cb_table[0x85] = CB_INT_PTR(85);
	cb_table[0x86] = CB_INT_PTR(86);
	cb_table[0x87] = CB_INT_PTR(87);
	cb_table[0x88] = CB_INT_PTR(88);
	cb_table[0x89] = CB_INT_PTR(89);
	cb_table[0x8A] = CB_INT_PTR(8A);
	cb_table[0x8B] = CB_INT_PTR(8B);
	cb_table[0x8C] = CB_INT_PTR(8C);
	cb_table[0x8D] = CB_INT_PTR(8D);
	cb_table[0x8E] = CB_INT_PTR(8E);
	cb_table[0x8F] = CB_INT_PTR(8F);

	cb_table[0x90] = CB_INT_PTR(90);
	cb_table[0x91] = CB_INT_PTR(91);
	cb_table[0x92] = CB_INT_PTR(92);
	cb_table[0x93] = CB_INT_PTR(93);
	cb_table[0x94] = CB_INT_PTR(94);
	cb_table[0x95] = CB_INT_PTR(95);
	cb_table[0x96] = CB_INT_PTR(96);
	cb_table[0x97] = CB_INT_PTR(97);
	cb_table[0x98] = CB_INT_PTR(98);
	cb_table[0x99] = CB_INT_PTR(99);
	cb_table[0x9A] = CB_INT_PTR(9A);
	cb_table[0x9B] = CB_INT_PTR(9B);
	cb_table[0x9C] = CB_INT_PTR(9C);
	cb_table[0x9D] = CB_INT_PTR(9D);
	cb_table[0x9E] = CB_INT_PTR(9E);
	cb_table[0x9F] = CB_INT_PTR(9F);

	cb_table[0xA0] = CB_INT_PTR(A0);
	cb_table[0xA1] = CB_INT_PTR(A1);
	cb_table[0xA2] = CB_INT_PTR(A2);
	cb_table[0xA3] = CB_INT_PTR(A3);
	cb_table[0xA4] = CB_INT_PTR(A4);
	cb_table[0xA5] = CB_INT_PTR(A5);
	cb_table[0xA6] = CB_INT_PTR(A6);
	cb_table[0xA7] = CB_INT_PTR(A7);
	cb_table[0xA8] = CB_INT_PTR(A8);
	cb_table[0xA9] = CB_INT_PTR(A9);
	cb_table[0xAA] = CB_INT_PTR(AA);
	cb_table[0xAB] = CB_INT_PTR(AB);
	cb_table[0xAC] = CB_INT_PTR(AC);
	cb_table[0xAD] = CB_INT_PTR(AD);
	cb_table[0xAE] = CB_INT_PTR(AE);
	cb_table[0xAF] = CB_INT_PTR(AF);

	cb_table[0xB0] = CB_INT_PTR(B0);
	cb_table[0xB1] = CB_INT_PTR(B1);
	cb_table[0xB2] = CB_INT_PTR(B2);
	cb_table[0xB3] = CB_INT_PTR(B3);
	cb_table[0xB4] = CB_INT_PTR(B4);
	cb_table[0xB5] = CB_INT_PTR(B5);
	cb_table[0xB6] = CB_INT_PTR(B6);
	cb_table[0xB7] = CB_INT_PTR(B7);
	cb_table[0xB8] = CB_INT_PTR(B8);
	cb_table[0xB9] = CB_INT_PTR(B9);
	cb_table[0xBA] = CB_INT_PTR(BA);
	cb_table[0xBB] = CB_INT_PTR(BB);
	cb_table[0xBC] = CB_INT_PTR(BC);
	cb_table[0xBD] = CB_INT_PTR(BD);
	cb_table[0xBE] = CB_INT_PTR(BE);
	cb_table[0xBF] = CB_INT_PTR(BF);

	cb_table[0xC0] = CB_INT_PTR(C0);
	cb_table[0xC1] = CB_INT_PTR(C1);
	cb_table[0xC2] = CB_INT_PTR(C2);
	cb_table[0xC3] = CB_INT_PTR(C3);
	cb_table[0xC4] = CB_INT_PTR(C4);
	cb_table[0xC5] = CB_INT_PTR(C5);
	cb_table[0xC6] = CB_INT_PTR(C6);
	cb_table[0xC7] = CB_INT_PTR(C7);
	cb_table[0xC8] = CB_INT_PTR(C8);
	cb_table[0xC9] = CB_INT_PTR(C9);
	cb_table[0xCA] = CB_INT_PTR(CA);
	cb_table[0xCB] = CB_INT_PTR(CB);
	cb_table[0xCC] = CB_INT_PTR(CC);
	cb_table[0xCD] = CB_INT_PTR(CD);
	cb_table[0xCE] = CB_INT_PTR(CE);
	cb_table[0xCF] = CB_INT_PTR(CF);

	cb_table[0xD0] = CB_INT_PTR(D0);
	cb_table[0xD1] = CB_INT_PTR(D1);
	cb_table[0xD2] = CB_INT_PTR(D2);
	cb_table[0xD3] = CB_INT_PTR(D3);
	cb_table[0xD4] = CB_INT_PTR(D4);
	cb_table[0xD5] = CB_INT_PTR(D5);
	cb_table[0xD6] = CB_INT_PTR(D6);
	cb_table[0xD7] = CB_INT_PTR(D7);
	cb_table[0xD8] = CB_INT_PTR(D8);
	cb_table[0xD9] = CB_INT_PTR(D9);
	cb_table[0xDA] = CB_INT_PTR(DA);
	cb_table[0xDB] = CB_INT_PTR(DB);
	cb_table[0xDC] = CB_INT_PTR(DC);
	cb_table[0xDD] = CB_INT_PTR(DD);
	cb_table[0xDE] = CB_INT_PTR(DE);
	cb_table[0xDF] = CB_INT_PTR(DF);

	cb_table[0xE0] = CB_INT_PTR(E0);
	cb_table[0xE1] = CB_INT_PTR(E1);
	cb_table[0xE2] = CB_INT_PTR(E2);
	cb_table[0xE3] = CB_INT_PTR(E3);
	cb_table[0xE4] = CB_INT_PTR(E4);
	cb_table[0xE5] = CB_INT_PTR(E5);
	cb_table[0xE6] = CB_INT_PTR(E6);
	cb_table[0xE7] = CB_INT_PTR(E7);
	cb_table[0xE8] = CB_INT_PTR(E8);
	cb_table[0xE9] = CB_INT_PTR(E9);
	cb_table[0xEA] = CB_INT_PTR(EA);
	cb_table[0xEB] = CB_INT_PTR(EB);
	cb_table[0xEC] = CB_INT_PTR(EC);
	cb_table[0xED] = CB_INT_PTR(ED);
	cb_table[0xEE] = CB_INT_PTR(EE);
	cb_table[0xEF] = CB_INT_PTR(EF);

	cb_table[0xF0] = CB_INT_PTR(F0);
	cb_table[0xF1] = CB_INT_PTR(F1);
	cb_table[0xF2] = CB_INT_PTR(F2);
	cb_table[0xF3] = CB_INT_PTR(F3);
	cb_table[0xF4] = CB_INT_PTR(F4);
	cb_table[0xF5] = CB_INT_PTR(F5);
	cb_table[0xF6] = CB_INT_PTR(F6);
	cb_table[0xF7] = CB_INT_PTR(F7);
	cb_table[0xF8] = CB_INT_PTR(F8);
	cb_table[0xF9] = CB_INT_PTR(F9);
	cb_table[0xFA] = CB_INT_PTR(FA);
	cb_table[0xFB] = CB_INT_PTR(FB);
	cb_table[0xFC] = CB_INT_PTR(FC);
	cb_table[0xFD] = CB_INT_PTR(FD);
	cb_table[0xFE] = CB_INT_PTR(FE);
	cb_table[0xFF] = CB_INT_PTR(FF);
}

INSTRUCTION(CB, {
	byte nextInstr = mem->Read(ctx.ip++);

	state->Sync(1);

	return (cb_table[nextInstr])(ctx, mem, state);
})