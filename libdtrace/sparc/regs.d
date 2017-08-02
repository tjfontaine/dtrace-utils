/*
 * Oracle Linux DTrace.
 * Copyright © 2012, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

inline int R_G0	= 0;
#pragma D binding "1.0" R_G0
inline int R_G1	= 1;
#pragma D binding "1.0" R_G1
inline int R_G2	= 2;
#pragma D binding "1.0" R_G2
inline int R_G3	= 3;
#pragma D binding "1.0" R_G3
inline int R_G4	= 4;
#pragma D binding "1.0" R_G4
inline int R_G5	= 5;
#pragma D binding "1.0" R_G5
inline int R_G6	= 6;
#pragma D binding "1.0" R_G6
inline int R_G7	= 7;
#pragma D binding "1.0" R_G7

inline int R_O0	= 8;
#pragma D binding "1.0" R_O0
inline int R_O1	= 9;
#pragma D binding "1.0" R_O1
inline int R_O2	= 10;
#pragma D binding "1.0" R_O2
inline int R_O3	= 11;
#pragma D binding "1.0" R_O3
inline int R_O4	= 12;
#pragma D binding "1.0" R_O4
inline int R_O5	= 13;
#pragma D binding "1.0" R_O5
inline int R_O6	= 14;
#pragma D binding "1.0" R_O6
inline int R_O7	= 15;
#pragma D binding "1.0" R_O7

inline int R_L0	= 16;
#pragma D binding "1.0" R_L0
inline int R_L1	= 17;
#pragma D binding "1.0" R_L1
inline int R_L2	= 18;
#pragma D binding "1.0" R_L2
inline int R_L3	= 19;
#pragma D binding "1.0" R_L3
inline int R_L4	= 20;
#pragma D binding "1.0" R_L4
inline int R_L5	= 21;
#pragma D binding "1.0" R_L5
inline int R_L6	= 22;
#pragma D binding "1.0" R_L6
inline int R_L7	= 23;
#pragma D binding "1.0" R_L7

inline int R_I0	= 24;
#pragma D binding "1.0" R_I0
inline int R_I1	= 25;
#pragma D binding "1.0" R_I1
inline int R_I2	= 26;
#pragma D binding "1.0" R_I2
inline int R_I3	= 27;
#pragma D binding "1.0" R_I3
inline int R_I4	= 28;
#pragma D binding "1.0" R_I4
inline int R_I5	= 29;
#pragma D binding "1.0" R_I5
inline int R_I6	= 30;
#pragma D binding "1.0" R_I6
inline int R_I7	= 31;
#pragma D binding "1.0" R_I7

inline int R_CCR = 32;
#pragma D binding "1.0" R_CCR
inline int R_PC = 33;
#pragma D binding "1.0" R_PC
inline int R_nPC = 34;
#pragma D binding "1.0" R_nPC
inline int R_NPC = R_nPC;
#pragma D binding "1.0" R_NPC
inline int R_Y = 35;
#pragma D binding "1.0" R_Y
inline int R_ASI = 36;
#pragma D binding "1.0" R_ASI
inline int R_FPRS = 37;
#pragma D binding "1.0" R_FPRS
inline int R_PS = R_CCR;
#pragma D binding "1.0" R_PS
inline int R_SP = R_O6;
#pragma D binding "1.0" R_SP
inline int R_FP = R_I6;
#pragma D binding "1.0" R_FP
inline int R_R0 = R_O0;
#pragma D binding "1.0" R_R0
inline int R_R1 = R_O1;
#pragma D binding "1.0" R_R1
