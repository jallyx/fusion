#include "Base.h"

enum EA {
	A, B, C
	D,
	E
}

struct SA {
	int32 i;
}

table TA {
	required char c;
	required bool b;
	required float f1,f2;
	required double d;
	required string s;
	required int8 s8;
	required uint8 u8;
	required int16 s16;
	required uint16 u16;
	required int32 s32;
	required uint32 u32;
	required int64 s64;
	required uint64 u64;
	required EA ea1, ea2;
	required SA sa1, sa2;
	required int32[] i32l;
	required SA[] sal;
	required EA[] eal;
	required map<string,int64> si64m;
	required map<int32,SA> i32sam;
	required map<int64,EA> i64eam;
	optional char c1,c2;
	optional float f6;
} key=c, tblname=ta

struct SAB : TA {
	map<string,SA> ssam;
}