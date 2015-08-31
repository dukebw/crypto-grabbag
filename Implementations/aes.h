#include "crypt_helper.h"

// TODO(brendan): 128 bits for now -- add 192 and 256 bit versions
#define KEY_LENGTH			4 // NOTE(brendan): 32-bit words
#define COL_COUNT_NB		4
#define ROW_COUNT_NK		KEY_LENGTH
#define NUMBER_OF_ROUNDS	10
#define AES_SUCCESS			0

// TODO(brendan): use struct context instead of global state
global_variable u8 GlobalStateArray[ROW_COUNT_NK*COL_COUNT_NB];
global_variable u32 GlobalKeySchedule[(NUMBER_OF_ROUNDS + 1)*COL_COUNT_NB];

global_variable u8 SBox[] =
{
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5,
    0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0,
    0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC,
    0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A,
    0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0,
    0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B,
    0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85,
    0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,
    0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17,
    0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88,
    0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C,
    0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9,
    0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6,
    0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E,
    0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94,
    0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68,
    0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};

// NOTE(brendan): Rcon[i] contains [x^(i - 1), 0, 0, 0]
global_variable u8 RoundConstant[] =
{
  0x8D, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36, 0x6C, 0xD8, 0xAB, 0x4D, 0x9A,
  0x2F, 0x5E, 0xBC, 0x63, 0xC6, 0x97, 0x35, 0x6A, 0xD4, 0xB3, 0x7D, 0xFA, 0xEF, 0xC5, 0x91, 0x39,
  0x72, 0xE4, 0xD3, 0xBD, 0x61, 0xC2, 0x9F, 0x25, 0x4A, 0x94, 0x33, 0x66, 0xCC, 0x83, 0x1D, 0x3A,
  0x74, 0xE8, 0xCB, 0x8D, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36, 0x6C, 0xD8,
  0xAB, 0x4D, 0x9A, 0x2F, 0x5E, 0xBC, 0x63, 0xC6, 0x97, 0x35, 0x6A, 0xD4, 0xB3, 0x7D, 0xFA, 0xEF,
  0xC5, 0x91, 0x39, 0x72, 0xE4, 0xD3, 0xBD, 0x61, 0xC2, 0x9F, 0x25, 0x4A, 0x94, 0x33, 0x66, 0xCC,
  0x83, 0x1D, 0x3A, 0x74, 0xE8, 0xCB, 0x8D, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B,
  0x36, 0x6C, 0xD8, 0xAB, 0x4D, 0x9A, 0x2F, 0x5E, 0xBC, 0x63, 0xC6, 0x97, 0x35, 0x6A, 0xD4, 0xB3,
  0x7D, 0xFA, 0xEF, 0xC5, 0x91, 0x39, 0x72, 0xE4, 0xD3, 0xBD, 0x61, 0xC2, 0x9F, 0x25, 0x4A, 0x94,
  0x33, 0x66, 0xCC, 0x83, 0x1D, 0x3A, 0x74, 0xE8, 0xCB, 0x8D, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20,
  0x40, 0x80, 0x1B, 0x36, 0x6C, 0xD8, 0xAB, 0x4D, 0x9A, 0x2F, 0x5E, 0xBC, 0x63, 0xC6, 0x97, 0x35,
  0x6A, 0xD4, 0xB3, 0x7D, 0xFA, 0xEF, 0xC5, 0x91, 0x39, 0x72, 0xE4, 0xD3, 0xBD, 0x61, 0xC2, 0x9F,
  0x25, 0x4A, 0x94, 0x33, 0x66, 0xCC, 0x83, 0x1D, 0x3A, 0x74, 0xE8, 0xCB, 0x8D, 0x01, 0x02, 0x04,
  0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36, 0x6C, 0xD8, 0xAB, 0x4D, 0x9A, 0x2F, 0x5E, 0xBC, 0x63,
  0xC6, 0x97, 0x35, 0x6A, 0xD4, 0xB3, 0x7D, 0xFA, 0xEF, 0xC5, 0x91, 0x39, 0x72, 0xE4, 0xD3, 0xBD,
  0x61, 0xC2, 0x9F, 0x25, 0x4A, 0x94, 0x33, 0x66, 0xCC, 0x83, 0x1D, 0x3A, 0x74, 0xE8, 0xCB
};

// TODO(brendan): parallelize (operate on u32)?
internal inline u8
MultiplyByX(u8 ByteValue)
{
	u8 Result;
	if (ByteValue & 0x80)
	{
		// NOTE(brendan): if Result > m(x) reduce mod m(x)
		Result = ((ByteValue << 1) ^ 0x1B);
	}
	else
	{
		Result = (ByteValue << 1);
	}
	return Result;
}

internal inline u8
MultiplyByPowerOfX(u8 ByteValue, u32 Power)
{
	u8 Result = ByteValue;
	Stopif(Power >= 8, return 0xFF, "Power of X too high");
	for (u32 PowerIndex = 0;
		 PowerIndex < Power;
		 ++PowerIndex)
	{
		Result = MultiplyByX(Result);
	}
	return Result;
}

internal inline u8
MixColumnsByteTransform(u8 A, u8 B, u8 C, u8 D)
{
	u8 Result = (MultiplyByX(A) ^ MultiplyByPowerOfX(B, 2) ^ C ^ D);
	return Result;
}

internal inline u32
Word(u8 A, u8 B, u8 C, u8 D)
{
	u32 Result = (A | (B << 8) | (C << 16) | (D << 24));
	return Result;
}

internal inline u8
LowByte(u32 Word)
{
	u8 Result = (Word & 0xFF);
	return Result;
}

internal inline u8
MidLowByte(u32 Word)
{
	u8 Result = ((Word >> 8) & 0xFF);
	return Result;
}

internal inline u8
MidHighByte(u32 Word)
{
	u8 Result = ((Word >> 16) & 0xFF);
	return Result;
}

internal inline u8
HighByte(u32 Word)
{
	u8 Result = ((Word >> 24) & 0xFF);
	return Result;
}

internal void
AddRoundKey(u8 *StateArray, u32 *KeySchedule)
{
	Stopif((StateArray == 0) || (KeySchedule == 0), return, "Null inputs to AddRoundKey()");
	for (u32 RoundKeyIndex = 0;
		 RoundKeyIndex < COL_COUNT_NB;
		 ++RoundKeyIndex)
	{
		u32 Column = Word(StateArray[RoundKeyIndex], StateArray[RoundKeyIndex + ROW_COUNT_NK],
						  StateArray[RoundKeyIndex + 2*ROW_COUNT_NK], StateArray[RoundKeyIndex + 3*ROW_COUNT_NK]);
		Column = Column ^ KeySchedule[RoundKeyIndex];
		StateArray[RoundKeyIndex] = LowByte(Column);
		StateArray[RoundKeyIndex + ROW_COUNT_NK] = MidLowByte(Column);
		StateArray[RoundKeyIndex + 2*ROW_COUNT_NK] = MidHighByte(Column);;
		StateArray[RoundKeyIndex + 3*ROW_COUNT_NK] = HighByte(Column);;
	}
}

internal inline u32
RotateWordLeft(u32 Word, i32 Amount)
{
	u32 Result;
	Amount &= 31;
	Result = ((Word << Amount) | (Word >> (32 - Amount)));
	return Result;
}

internal inline u32
RotateWordRight(u32 Word, i32 Amount)
{
	u32 Result;
	Amount &= 31;
	Result = ((Word >> Amount) | (Word << (32 - Amount)));
	return Result;
}

internal inline void
ShiftRows(u8 *StateArray)
{
	Stopif(StateArray == 0, return, "Null input to SubBytes()");
	// s[r][c] = s[r][c + shift(r, Nb) mod Nb] for 0 < r < 4 and 0 <= c < Nb
	// where shift(n, 4) == n
	for (u32 RowIndex = 1;
		 RowIndex < ROW_COUNT_NK;
		 ++RowIndex)
	{
		u32 *Row = (u32 *)(StateArray + RowIndex*COL_COUNT_NB);
		*Row = (*Row << RowIndex) | (*Row >> (COL_COUNT_NB - RowIndex));
	}
}

internal inline u32
SubstituteWord(u32 Word)
{
	u32 Result;
	Result = SBox[LowByte(Word)];
	Result |= (SBox[MidLowByte(Word)] << 8);
	Result |= (SBox[MidHighByte(Word)] << 16);
	Result |= (SBox[HighByte(Word)] << 24);
	return Result;
}

internal inline void
SubBytes(u8 *StateArray)
{
	Stopif(StateArray == 0, return, "Null input to SubBytes()");
	// 1.	Take the multiplicative inverse in the finite field GF(2^8).
	// 2.	Apply the following affine transformation over GF(2):
	//		b[i] = b[i] ^ b[(i + 4) mod 8] ^ b[(i + 5) mod 8] ^ b[(i + 6) mod 8] ^
	//			   b[(i + 7) mod 8] ^ c[i]
	// 		Where b[i] is the i'th bit of the byte, and c[i] is the i'th bit of a
	//		byte c with value 0x63.
	for (u32 StateByteIndex = 0;
		 StateByteIndex < sizeof(StateArray);
		 ++StateByteIndex)
	{
		StateArray[StateByteIndex] = SBox[StateArray[StateByteIndex]];
	}
}

// NOTE(brendan): INPUT: Sequences of 128 bits. OUTPUT: Same.
internal void
AesEncryptBlock(u8 *Cipher, u8 *Message, u32 MessageLength, u8 *Key, u32 KeyLength)
{
	Stopif((Cipher == 0) || (Message == 0) || (Key == 0), return, "Null input to AesEncrypt()");
	Stopif(MessageLength < COL_COUNT_NB*(NUMBER_OF_ROUNDS + 1), return, "Bad message block size");
	Stopif(KeyLength != 4*KEY_LENGTH, return, "Invalid key length");

	// TODO(brendan): bug in KeyExpansion output for 128-bit example key
	// KeyExpansion(byte key[4*Nk], word w[Nb*(Nr + 1)], Nk)
	memcpy(GlobalKeySchedule, Key, KeyLength);
	for (u32 KeyIndex = ROW_COUNT_NK;
		 KeyIndex < sizeof(GlobalKeySchedule);
		 ++KeyIndex)
	{
		u32 Temp = GlobalKeySchedule[KeyIndex - 1];
		if ((KeyIndex % ROW_COUNT_NK) == 0)
		{
			Temp = SubstituteWord(RotateWordRight(Temp, 8)) ^ (u32)RoundConstant[KeyIndex/ROW_COUNT_NK];
		}
		else if ((ROW_COUNT_NK > 6) && ((KeyIndex % ROW_COUNT_NK) == 4))
		{
			Temp = SubstituteWord(Temp);
		}
		GlobalKeySchedule[KeyIndex] = GlobalKeySchedule[KeyIndex - ROW_COUNT_NK] ^ Temp;
	}

	memcpy(GlobalStateArray, Message, sizeof(GlobalStateArray));

	// AddRoundKey(state, w[0, Nb - 1])
	AddRoundKey(GlobalStateArray, GlobalKeySchedule);

	for (u32 RoundIndex = 0;
		 RoundIndex < NUMBER_OF_ROUNDS;
		 ++RoundIndex)
	{
		SubBytes(GlobalStateArray);
		ShiftRows(GlobalStateArray);

		// MixColumns(state)
		// Columns are considered as four-term polynomials over GF(2^8) and multiplied
		// modulo x^4 + 1 with fixed polynomial a(x) = 3x^3 + x^2 + x + 2.
		for (u32 ColumnIndex = 0;
			 ColumnIndex < COL_COUNT_NB;
			 ++ColumnIndex)
		{
			// NOTE(brendan): Multiplication of polynomials over GF(2^8) by x:
			// x*b(x), where b(x) := b[7]*x^7 + ... + b[0] is obtained by
			// reducing the above result modulo m(x) := x^8 + x^4 + x^3 + x + 1
			// (irreducible polynomial over GF(2^8)). If b[7] == 0, the result is
			// already reduced. If b[7] == 1, the reduction is accomplished by XOR'ing m(x)
			// (subtracting m(x)).
			u8 *Row0 = (GlobalStateArray + ColumnIndex);
			u8 *Row1 = (GlobalStateArray + ColumnIndex + COL_COUNT_NB);
			u8 *Row2 = (GlobalStateArray + ColumnIndex + 2*COL_COUNT_NB);
			u8 *Row3 = (GlobalStateArray + ColumnIndex + 3*COL_COUNT_NB);
			*Row0 = MixColumnsByteTransform(*Row0, *Row1, *Row2, *Row3);
			*Row1 = MixColumnsByteTransform(*Row1, *Row2, *Row3, *Row0);
			*Row2 = MixColumnsByteTransform(*Row2, *Row3, *Row0, *Row1);
			*Row3 = MixColumnsByteTransform(*Row3, *Row0, *Row1, *Row2);
		}
		AddRoundKey(GlobalStateArray, GlobalKeySchedule + RoundIndex*COL_COUNT_NB);
	}
	SubBytes(GlobalStateArray);
	ShiftRows(GlobalStateArray);
	AddRoundKey(GlobalStateArray, GlobalKeySchedule + NUMBER_OF_ROUNDS*COL_COUNT_NB);

	memcpy(Cipher, GlobalStateArray, sizeof(GlobalStateArray));
}