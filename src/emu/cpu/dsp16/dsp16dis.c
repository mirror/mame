#include "emu.h"
#include "dsp16.h"

astring disasmF1Field(const UINT8& F1, const UINT8& D, const UINT8& S)
{
	astring ret = "";
	switch (F1)
	{
		case 0x00: ret.printf("a%d = p, p = x*y", D); break;
		case 0x01: ret.printf("a%d = a%d + p, p = x*y", D, S); break;
		case 0x02: ret.printf("p = x*y"); break;
		case 0x03: ret.printf("a%d = a%d - p, p = x*y", D, S); break;
		case 0x04: ret.printf("a%d = p", D); break;
		case 0x05: ret.printf("a%d = a%d + p", D, S); break;
		case 0x06: ret.printf("NOP"); break;
		case 0x07: ret.printf("a%d = a%d - p", D, S); break;
		case 0x08: ret.printf("a%d = a%d | y", D, S); break;
		case 0x09: ret.printf("a%d = a%d ^ y", D, S); break;
		case 0x0a: ret.printf("a%d & y", S); break;
		case 0x0b: ret.printf("a%d - y", S); break;
		case 0x0c: ret.printf("a%d = y", D); break;
		case 0x0d: ret.printf("a%d = a%d + y", D, S); break;
		case 0x0e: ret.printf("a%d = a%d & y", D, S); break;
		case 0x0f: ret.printf("a%d = a%d - y", D, S); break;

		default: return "UNKNOWN";
	}
	return ret;
}

astring disasmYField(const UINT8& Y)
{
	switch (Y)
	{
		case 0x00: return "*r0";
		case 0x01: return "*r0++";
		case 0x02: return "*r0--";
		case 0x03: return "*r0++j";

		case 0x04: return "*r1";
		case 0x05: return "*r1++";
		case 0x06: return "*r1--";
		case 0x07: return "*r1++j";

		case 0x08: return "*r2";
		case 0x09: return "*r2++";
		case 0x0a: return "*r2--";
		case 0x0b: return "*r2++j";

		case 0x0c: return "*r3";
		case 0x0d: return "*r3++";
		case 0x0e: return "*r3--";
		case 0x0f: return "*r3++j";

		default: return "UNKNOWN";
	}
	// never executed
	//return "";
}

astring disasmZField(const UINT8& Z)
{
	switch (Z)
	{
		case 0x00: return "*r0zp";
		case 0x01: return "*r0pz";
		case 0x02: return "*r0m2";
		case 0x03: return "*r0jk";

		case 0x04: return "*r1zp";
		case 0x05: return "*r1pz";
		case 0x06: return "*r1m2";
		case 0x07: return "*r1jk";

		case 0x08: return "*r2zp";
		case 0x09: return "*r2pz";
		case 0x0a: return "*r2m2";
		case 0x0b: return "*r2jk";

		case 0x0c: return "*r3zp";
		case 0x0d: return "*r3pz";
		case 0x0e: return "*r3m2";
		case 0x0f: return "*r3jk";

		default: return "UNKNOWN";
	}
	// never executed
	//return "";
}

astring disasmF2Field(const UINT8& F2, const UINT8& D, const UINT8& S)
{
	astring ret = "";
	switch (F2)
	{
		case 0x00: ret.printf("a%d = a%d >> 1", D, S); break;
		case 0x01: ret.printf("a%d = a%d << 1", D, S); break;
		case 0x02: ret.printf("a%d = a%d >> 4", D, S); break;
		case 0x03: ret.printf("a%d = a%d << 4", D, S); break;
		case 0x04: ret.printf("a%d = a%d >> 8", D, S); break;
		case 0x05: ret.printf("a%d = a%d << 8", D, S); break;
		case 0x06: ret.printf("a%d = a%d >> 16", D, S); break;
		case 0x07: ret.printf("a%d = a%d << 16", D, S); break;

		case 0x08: ret.printf("a%d = p", D); break;
		case 0x09: ret.printf("a%dh = a%dh + 1", D, S); break;
		case 0x0a: ret.printf("RESERVED"); break;
		case 0x0b: ret.printf("a%d = rnd(a%d)", D, S); break;
		case 0x0c: ret.printf("a%d = y", D); break;
		case 0x0d: ret.printf("a%d = a%d + 1", D, S); break;
		case 0x0e: ret.printf("a%d = a%d", D, S); break;
		case 0x0f: ret.printf("a%d = -a%d", D, S); break;

		default: return "UNKNOWN";
	}
	return ret;
}

astring disasmCONField(const UINT8& CON)
{
	switch (CON)
	{
		case 0x00: return "mi";
		case 0x01: return "pl";
		case 0x02: return "eq";
		case 0x03: return "ne";
		case 0x04: return "lvs";
		case 0x05: return "lvc";
		case 0x06: return "mvs";
		case 0x07: return "mvc";
		case 0x08: return "heads";
		case 0x09: return "tails";
		case 0x0a: return "c0ge";
		case 0x0b: return "c0lt";
		case 0x0c: return "c1ge";
		case 0x0d: return "c1lt";
		case 0x0e: return "true";
		case 0x0f: return "false";
		case 0x10: return "gt";
		case 0x11: return "le";

		default: return "RESERVED";
	}
	// never executed
	//return "";
}

astring disasmBField(const UINT8& B)
{
	switch (B)
	{
		case 0x00: return "return";
		case 0x01: return "ireturn";
		case 0x02: return "goto pt";
		case 0x03: return "call pt";
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07: return "RESERVED";

		default: return "UNKNOWN";
	}
	// never executed
	//return "";
}

astring disasmRImmediateField(const UINT8& R)
{
	switch (R)
	{
		case 0x00: return "j";
		case 0x01: return "k";
		case 0x02: return "rb";
		case 0x03: return "re";
		case 0x04: return "r0";
		case 0x05: return "r1";
		case 0x06: return "r2";
		case 0x07: return "r3";

		default: return "UNKNOWN";
	}
	// never executed
	//return "";
}

astring disasmRField(const UINT8& R)
{
	switch (R)
	{
		case 0x00: return "r0";
		case 0x01: return "r1";
		case 0x02: return "r2";
		case 0x03: return "r3";
		case 0x04: return "j";
		case 0x05: return "k";
		case 0x06: return "rb";
		case 0x07: return "re";
		case 0x08: return "pt";
		case 0x09: return "pr";
		case 0x0a: return "pi";
		case 0x0b: return "i";

		case 0x10: return "x";
		case 0x11: return "y";
		case 0x12: return "yl";
		case 0x13: return "auc";
		case 0x14: return "psw";
		case 0x15: return "c0";
		case 0x16: return "c1";
		case 0x17: return "c2";
		case 0x18: return "sioc";
		case 0x19: return "srta";
		case 0x1a: return "sdx";
		case 0x1b: return "tdms";
		case 0x1c: return "pioc";
		case 0x1d: return "pdx0";
		case 0x1e: return "pdx1";

		default: return "RESERVED";
	}
	// never executed
	//return "";
}

astring disasmIField(const UINT8& I)
{
	switch (I)
	{
		case 0x00: return "r0/j";
		case 0x01: return "r1/k";
		case 0x02: return "r2/rb";
		case 0x03: return "r3/re";

		default: return "UNKNOWN";
	}
	// never executed
	//return "";
}

bool disasmSIField(const UINT8& SI)
{
	switch (SI)
	{
		case 0x00: return 0;    // Not a software interrupt
		case 0x01: return 1;    // Software Interrupt
	}
	return false;
}


CPU_DISASSEMBLE( dsp16a )
{
	UINT8 opSize = 1;
	UINT32 dasmflags = 0;
	UINT16 op  = oprom[0] | (oprom[1] << 8);
	UINT16 op2 = oprom[2] | (oprom[3] << 8);

	// TODO: Test for previous "if CON" instruction and tab the next instruction in?

	const UINT8 opcode = (op >> 11) & 0x1f;
	switch(opcode)
	{
		// Format 1: Multiply/ALU Read/Write Group
		case 0x06:
		{
			// F1, Y
			const UINT8 Y = (op & 0x000f);
			const UINT8 S = (op & 0x0200) >> 9;
			const UINT8 D = (op & 0x0400) >> 10;
			const UINT8 F1 = (op & 0x01e0) >> 5;
			astring yString = disasmYField(Y);
			astring fString = disasmF1Field(F1, D, S);
			sprintf(buffer, "%s, %s", fString.cstr(), yString.cstr());
			break;
		}
		case 0x04: case 0x1c:
		{
			// F1 Y=a0[1] | F1 Y=a1[1]
			const UINT8 Y = (op & 0x000f);
			const UINT8 X = (op & 0x0010) >> 4;
			const UINT8 S = (op & 0x0200) >> 9;
			const UINT8 D = (op & 0x0400) >> 10;
			const UINT8 F1 = (op & 0x01e0) >> 5;
			astring yString = disasmYField(Y);
			astring fString = disasmF1Field(F1, D, S);
			astring aString = (opcode == 0x1c) ? "a0" : "a1";
			astring xString = (X) ? "" : "l";
			sprintf(buffer, "%s = %s%s, %s", yString.cstr(), aString.cstr(), xString.cstr(), fString.cstr());
			break;
		}
		case 0x16:
		{
			// F1, x = Y
			const UINT8 Y = (op & 0x000f);
			const UINT8 S = (op & 0x0200) >> 9;
			const UINT8 D = (op & 0x0400) >> 10;
			const UINT8 F1 = (op & 0x01e0) >> 5;
			astring yString = disasmYField(Y);
			astring fString = disasmF1Field(F1, D, S);
			sprintf(buffer, "%s, x = %s", fString.cstr(), yString.cstr());
			break;
		}
		case 0x17:
		{
			// F1, y[l] = Y
			const UINT8 Y = (op & 0x000f);
			const UINT8 X = (op & 0x0010) >> 4;
			const UINT8 S = (op & 0x0200) >> 9;
			const UINT8 D = (op & 0x0400) >> 10;
			const UINT8 F1 = (op & 0x01e0) >> 5;
			astring yString = disasmYField(Y);
			astring fString = disasmF1Field(F1, D, S);
			astring xString = (X ? "y" : "y1");
			sprintf(buffer, "%s, %s = %s", fString.cstr(), xString.cstr(), yString.cstr());
			break;
		}
		case 0x1f:
		{
			// F1, y = Y, x = *pt++[i]
			const UINT8 Y = (op & 0x000f);
			const UINT8 X = (op & 0x0010) >> 4;
			const UINT8 S = (op & 0x0200) >> 9;
			const UINT8 D = (op & 0x0400) >> 10;
			const UINT8 F1 = (op & 0x01e0) >> 5;
			astring yString = disasmYField(Y);
			astring fString = disasmF1Field(F1, D, S);
			astring xString = (X ? "*pt++i" : "*pt++");
			sprintf(buffer, "%s, y = %s, x = %s", fString.cstr(), yString.cstr(), xString.cstr());
			break;
		}
		case 0x19: case 0x1b:
		{
			// F1, y = a0|1, x = *pt++[i]
			const UINT8 Y = (op & 0x000f);
			const UINT8 X = (op & 0x0010) >> 4;
			const UINT8 S = (op & 0x0200) >> 9;
			const UINT8 D = (op & 0x0400) >> 10;
			const UINT8 F1 = (op & 0x01e0) >> 5;
			astring fString = disasmF1Field(F1, D, S);
			astring xString = (X ? "*pt++i" : "*pt++");
			astring aString = (opcode == 0x19) ? "a0" : "a1";
			sprintf(buffer, "%s, y = %s, x = %s", fString.cstr(), aString.cstr(), xString.cstr());
			if (Y != 0x00) sprintf(buffer, "UNKNOWN");
			break;
		}
		case 0x14:
		{
			// F1, Y = y[1]
			const UINT8 Y = (op & 0x000f);
			const UINT8 X = (op & 0x0010) >> 4;
			const UINT8 S = (op & 0x0200) >> 9;
			const UINT8 D = (op & 0x0400) >> 10;
			const UINT8 F1 = (op & 0x01e0) >> 5;
			astring yString = disasmYField(Y);
			astring xString = (X ? "y" : "y1");
			astring fString = disasmF1Field(F1, D, S);
			sprintf(buffer, "%s, %s = %s", fString.cstr(), yString.cstr(), xString.cstr());
			break;
		}

		// Format 1a: Multiply/ALU Read/Write Group (major typo in docs on p3-51)
		case 0x07:
		{
			// F1, At[1] = Y
			const UINT8 Y = (op & 0x000f);
			const UINT8 S = (op & 0x0200) >> 9;
			const UINT8 aT = (op & 0x0400) >> 10;
			const UINT8 F1 = (op & 0x01e0) >> 5;
			astring yString = disasmYField(Y);
			astring atString = (aT ? "a0" : "a1");
			astring fString = disasmF1Field(F1, aT, S);
			sprintf(buffer, "%s, %s = %s", fString.cstr(), atString.cstr(), yString.cstr());
			break;
		}

		// Format 2: Multiply/ALU Read/Write Group
		case 0x15:
		{
			// F1, Z : y[1]
			const UINT8 Z = (op & 0x000f);
			const UINT8 X = (op & 0x0010) >> 4;
			const UINT8 S = (op & 0x0200) >> 9;
			const UINT8 D = (op & 0x0400) >> 10;
			const UINT8 F1 = (op & 0x01e0) >> 5;
			astring zString = disasmZField(Z);
			astring xString = (X ? "y" : "y1");
			astring fString = disasmF1Field(F1, D, S);
			sprintf(buffer, "%s, %s <=> %s", fString.cstr(), xString.cstr(), zString.cstr());
			break;
		}
		case 0x1d:
		{
			// F1, Z : y, x=*pt++[i]
			const UINT8 Z = (op & 0x000f);
			const UINT8 X = (op & 0x0010) >> 4;
			const UINT8 S = (op & 0x0200) >> 9;
			const UINT8 D = (op & 0x0400) >> 10;
			const UINT8 F1 = (op & 0x01e0) >> 5;
			astring zString = disasmZField(Z);
			astring xString = (X ? "*pt++i" : "*pt++");
			astring fString = disasmF1Field(F1, D, S);
			sprintf(buffer, "%s, %s <=> y, x = %s", fString.cstr(), zString.cstr(), xString.cstr());
			break;
		}

		// Format 2a: Multiply/ALU Read/Write Group
		case 0x05:
		{
			// F1, Z : aT[1]
			const UINT8 Z = (op & 0x000f);
			const UINT8 X = (op & 0x0010) >> 4;
			const UINT8 S = (op & 0x0200) >> 9;
			const UINT8 aT = (op & 0x0400) >> 10;
			const UINT8 F1 = (op & 0x01e0) >> 5;
			astring zString = disasmZField(Z);
			astring atString = (aT ? "a0" : "a1");
			atString += X ? "" : "1";   // TODO: Figure out unclear wording.
			astring fString = disasmF1Field(F1, aT, S);
			sprintf(buffer, "%s, %s <=> %s", fString.cstr(), zString.cstr(), atString.cstr());
			break;
		}

		// Format 3: Special Functions
		case 0x12:
		case 0x13:
		{
			// if|ifc CON F2
			const UINT8 CON = (op & 0x001f);
			const UINT8 S = (op & 0x0200) >> 9;
			const UINT8 D = (op & 0x0400) >> 10;
			const UINT8 F2 = (op & 0x01e0) >> 5;
			astring fString = disasmF2Field(F2, D, S);
			astring conString = disasmCONField(CON);
			if (op & 0x0800) sprintf(buffer,  "if %s : %s", conString.cstr(), fString.cstr());
			else             sprintf(buffer, "ifc %s : %s", conString.cstr(), fString.cstr());
			break;
		}

		// Format 4: Branch Direct Group
		case 0x00: case 0x01:
		{
			// goto JA
			const UINT16 JA = (op & 0x0fff) | (pc & 0xf000);
			sprintf(buffer, "goto 0x%04x", JA);
			break;
		}
		case 0x10: case 0x11:
		{
			// call JA
			const UINT16 JA = (op & 0x0fff) | (pc & 0xf000);
			sprintf(buffer, "call 0x%04x", JA);
			break;
		}

		// Format 5: Branch Indirect Group
		case 0x18:
		{
			// goto B
			const UINT8 B = (op & 0x0700) >> 8;
			astring bString = disasmBField(B);
			sprintf(buffer, "%s", bString.cstr());
			break;
		}

		// Format 6: Contitional Branch Qualifier/Software Interrupt (icall)
		case 0x1a:
		{
			// if CON [goto/call/return]
			const UINT8 CON = (op & 0x001f);
			astring conString = disasmCONField(CON);
			sprintf(buffer, "if %s:", conString.cstr());
			// TODO: Test for invalid ops
			// icall
			if (op == 0xd40e) sprintf(buffer, "icall");
			break;
		}

		// Format 7: Data Move Group
		case 0x09: case 0x0b:
		{
			// R = aS
			const UINT8 R = (op & 0x03f0) >> 4;
			const UINT8 S = (op & 0x1000) >> 12;
			astring rString = disasmRField(R);
			sprintf(buffer, "%s = %s", rString.cstr(), (S ? "a1" : "a0"));
			break;
		}
		case 0x08:
		{
			// aT = R
			const UINT8 R  = (op & 0x03f0) >> 4;
			const UINT8 aT = (op & 0x0400) >> 10;
			astring rString = disasmRField(R);
			sprintf(buffer, "%s = %s", (aT ? "a0" : "a1"), rString.cstr());
			break;
		}
		case 0x0f:
		{
			// R = Y
			const UINT8 Y = (op & 0x000f);
			const UINT8 R = (op & 0x03f0) >> 4;
			astring yString = disasmYField(Y);
			astring rString = disasmRField(R);
			sprintf(buffer, "%s = %s", rString.cstr(), yString.cstr());
			// TODO: Special case the R == [y, y1, or x] case
			break;
		}
		case 0x0c:
		{
			// Y = R
			const UINT8 Y = (op & 0x000f);
			const UINT8 R = (op & 0x03f0) >> 4;
			astring yString = disasmYField(Y);
			astring rString = disasmRField(R);
			// TODO: page 3-31 "special function encoding"
			sprintf(buffer, "%s = %s", yString.cstr(), rString.cstr());
			break;
		}
		case 0x0d:
		{
			// Z : R
			const UINT8 Z = (op & 0x000f);
			const UINT8 R = (op & 0x03f0) >> 4;
			astring zString = disasmZField(Z);
			astring rString = disasmRField(R);
			sprintf(buffer, "%s <=> %s", zString.cstr(), rString.cstr());
			break;
		}

		// Format 8: Data Move (immediate operand - 2 words)
		case 0x0a:
		{
			// R = N
			const UINT8 R = (op & 0x03f0) >> 4;
			astring rString = disasmRField(R);
			sprintf(buffer, "%s = 0x%04x", rString.cstr(), op2);
			opSize = 2;
			break;
		}

		// Format 9: Short Immediate Group
		case 0x02: case 0x03:
		{
			// R = M
			const UINT16 M = (op & 0x01ff);
			const UINT8  R = (op & 0x0e00) >> 9;
			astring rString = disasmRImmediateField(R);
			sprintf(buffer, "%s = 0x%04x", rString.cstr(), M);
			break;
		}

		// Format 10: do - redo
		case 0x0e:
		{
			// do|redo K
			const UINT8 K = (op & 0x007f);
			const UINT8 NI = (op & 0x0780) >> 7;
			sprintf(buffer, "do (next %d inst) %d times", NI, K);
			// TODO: Limits on K & NI
			if (NI == 0x00)
				sprintf(buffer, "redo %d", K);
			break;
		}

		// RESERVED
		case 0x1e:
		{
			sprintf(buffer, "RESERVED");
			break;
		}

		// UNKNOWN
		default:
		{
			sprintf(buffer, "UNKNOWN");
			break;
		}
	}

	return opSize | dasmflags | DASMFLAG_SUPPORTED;
}
