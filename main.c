
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>

enum opcode_decode
{
	R = 0x33,
	I = 0x13,
	S = 0x23,
	L = 0x03,
	B = 0x63,
	JALR = 0x67,
	JAL = 0x6F,
	AUIPC = 0x17,
	LUI = 0x37
};

typedef struct
{
	size_t data_mem_size_;
	uint32_t regfile_[32];
	uint32_t pc_;
	uint8_t *instr_mem_;
	uint8_t *data_mem_;
} CPU;

void CPU_open_instruction_mem(CPU *cpu, const char *filename);
void CPU_load_data_mem(CPU *cpu, const char *filename);

//helper functions
int8_t getFunc3(uint32_t instruction);
int8_t getRS1(uint32_t instruction);
int8_t getRS2(uint32_t instruction);
int8_t getRD(uint32_t instruction);
uint8_t getOpCode(uint32_t instruction);
int8_t getFunc7(uint32_t instruction);
int32_t shamt(uint32_t instruction);

//immediate functions
int32_t imm_I(uint32_t instruction);
int32_t imm_S(uint32_t instruction);
int32_t imm_B(uint32_t instruction);
uint32_t imm_U(uint32_t instruction);
int32_t imm_J(uint32_t instruction);

//R-Type functions
void ADD(CPU *cpu, uint32_t instruction);
void SUB(CPU *cpu, uint32_t instruction);
void SLL(CPU *cpu, uint32_t instruction);
void SLT(CPU *cpu, uint32_t instruction);
void SLTU(CPU *cpu, uint32_t instruction);
void XOR(CPU *cpu, uint32_t instruction);
void SRL(CPU *cpu, uint32_t instruction);
void SRA(CPU *cpu, uint32_t instruction);
void OR(CPU *cpu, uint32_t instruction);
void AND(CPU *cpu, uint32_t instruction);

//I-Type functions
void JALR1(CPU *cpu, uint32_t instruction);
void LB(CPU *cpu, uint32_t instruction);
void LH(CPU *cpu, uint32_t instruction);
void LW(CPU *cpu, uint32_t instruction);
void LBU(CPU *cpu, uint32_t instruction);
void LHU(CPU *cpu, uint32_t instruction);
void ADDI(CPU *cpu, uint32_t instruction);
void SLTI(CPU *cpu, uint32_t instruction);
void SLTU(CPU *cpu, uint32_t instruction);
void XORI(CPU *cpu, uint32_t instruction);
void ORI(CPU *cpu, uint32_t instruction);
void ANDI(CPU *cpu, uint32_t instruction);

//S-Type functions
void SB(CPU *cpu, uint32_t instruction);
void SH(CPU *cpu, uint32_t instruction);
void SW(CPU *cpu, uint32_t instruction);

//B_Type function
void BEQ(CPU *cpu, uint32_t instruction);
void BNE(CPU *cpu, uint32_t instruction);
void BLT(CPU *cpu, uint32_t instruction);
void BGE(CPU *cpu, uint32_t instruction);
void BLTU(CPU *cpu, uint32_t instruction);
void BGEU(CPU *cpu, uint32_t instruction);

//U-Type functions
void LUI1(CPU *cpu, uint32_t instruction);
void AUIPC1(CPU *cpu, uint32_t instruction);

//J_Type functions
void JAL1(CPU *cpu, uint32_t instruction);

//shift functions
void SLLI(CPU *cpu, uint32_t instruction);
void SRLI(CPU *cpu, uint32_t instruction);
void SRAI(CPU *cpu, uint32_t instruction);

//initialises the cpu to the values given
CPU *CPU_init(const char *path_to_inst_mem, const char *path_to_data_mem)
{
	CPU *cpu = (CPU *)malloc(sizeof(CPU));
	cpu->data_mem_size_ = 0x400000;
	cpu->pc_ = 0x0;
	CPU_open_instruction_mem(cpu, path_to_inst_mem);
	CPU_load_data_mem(cpu, path_to_data_mem);
	return cpu;
}

void CPU_open_instruction_mem(CPU *cpu, const char *filename)
{
	uint32_t instr_mem_size;
	FILE *input_file = fopen(filename, "r");
	if (!input_file)
	{
		printf("no input\n");
		exit(EXIT_FAILURE);
	}
	struct stat sb;
	if (stat(filename, &sb) == -1)
	{
		printf("error stat\n");
		perror("stat");
		exit(EXIT_FAILURE);
	}
	printf("size of instruction memory: %d Byte\n\n", sb.st_size);
	instr_mem_size = sb.st_size;
	cpu->instr_mem_ = malloc(instr_mem_size);
	fread(cpu->instr_mem_, sb.st_size, 1, input_file);
	fclose(input_file);
	return;
}

void CPU_load_data_mem(CPU *cpu, const char *filename)
{
	FILE *input_file = fopen(filename, "r");
	if (!input_file)
	{
		printf("no input\n");
		exit(EXIT_FAILURE);
	}
	struct stat sb;
	if (stat(filename, &sb) == -1)
	{
		printf("error stat\n");
		perror("stat");
		exit(EXIT_FAILURE);
	}
	printf("read data for data memory: %d Byte\n\n", sb.st_size);

	cpu->data_mem_ = malloc(cpu->data_mem_size_);
	fread(cpu->data_mem_, sb.st_size, 1, input_file);
	fclose(input_file);
	return;
}

/**
 * Instruction fetch Instruction decode, Execute, Memory access, Write back
 */

//implementing functions

int8_t getRS1(uint32_t instruction)
{
	// rs1 in bits 19..15
	return (instruction >> 15) & 0x1f;
};
int8_t getRS2(uint32_t instruction)
{
	// rs2 in bits 24..20
	return (instruction >> 20) & 0x1f;
};

//gives the address of the destination register
int8_t getRD(uint32_t instruction)
{
	// RD is in bits 7...11
	return (instruction >> 7) & 0x1f;
}

uint8_t getOpCode(uint32_t instruction)
{
	uint8_t opcode = (instruction & 0x7f);
	return opcode;
};

int8_t getFunc3(uint32_t instruction)
{
	return (instruction >> 12) & 0x7;
};

int8_t getFunc7(uint32_t instruction)
{
	return (instruction >> 25) & 0x7f;
};

//PROBLEM IS IN SHAMT-- ENDLESS LOOP but why?
int32_t shamt(uint32_t instruction)
{ //shifting 20 bits to the left and ANDing with last 5 bits
	//printf("%d", 9999);

	uint32_t temp_instr = (instruction >> 20) & 0x1f;

	if ((instruction >> 31) == 1)
	{
		return (temp_instr | 0xffffffe0);
	}
	else
	{
		//return (temp_instr & 0x0000001f);
		return (temp_instr | 0x00000000);
	}
}

int32_t imm_I(uint32_t instruction)
{
	//sign extend the bit after shifting
	uint32_t temp_instr = (instruction & 0xfff00000) >> 20;

	//if bit is 1, then 'or' with the temp_instr
	if ((instruction >> 31) == 1)
	{
		return (temp_instr | 0xfffff000);
	}
	else
	{
		return (temp_instr & 0x00000fff);
	}
}


int32_t imm_S(uint32_t instruction)
{
    int32_t S_immediate=(instruction&0x80000000) ? 0xFFFFF000 | (instruction&0xFE000000)>>20 | (instruction&0xF80)>>7
    : (instruction&0xFE000000)>>20 | (instruction&0xF80)>>7;

    return S_immediate;
}


int32_t imm_B(uint32_t instruction)
{
	return ((int32_t)(instruction & 0x80000000) >> 19) | ((instruction & 0x80) << 4) // imm[11]
		   | ((instruction >> 20) & 0x7e0)											 // imm[10:5]
		   | ((instruction >> 7) & 0x1e);											 // imm[4:1]
};

uint32_t imm_U(uint32_t instruction)
{
	return (int32_t)(instruction & 0xfffff000);
}

int32_t imm_J(uint32_t instruction)
{
	//printf("%d", 123);

	// imm[20|10:1|11|19:12] = inst[31|30:21|20|19:12]
	return ((int32_t)(instruction & 0x80000000) >> 11) | (instruction & 0xff000) // imm[19:12]
		   | ((instruction >> 9) & 0x800)										 // imm[11]
		   | ((instruction >> 20) & 0x7fe);										 // imm[10:1]
};

//R Type Instructions
void ADD(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	cpu->regfile_[rd] = cpu->regfile_[rs1] + cpu->regfile_[rs2];
	cpu->pc_ += 0x4;
}

void SUB(CPU *cpu, uint32_t instruction)
{
	//printf("%d", 55555);

	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	cpu->regfile_[rd] = cpu->regfile_[rs1] - cpu->regfile_[rs2];
	cpu->pc_ += 0x4;
}

void SLL(CPU *cpu, uint32_t instruction)
{ //I am temporarily changing uint to int to check if it works (22.08.22)
	//printf("%d", 55555);
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);

	cpu->regfile_[rd] = cpu->regfile_[rs1] << cpu->regfile_[rs2];
	cpu->pc_ += 0x4;
}

void SLT(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	cpu->regfile_[rd] = (int32_t)cpu->regfile_[rs1] < (int32_t)cpu->regfile_[rs2] ? 1 : 0;
	cpu->pc_ += 4;
}

void SLTU(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	cpu->regfile_[rd] = (uint32_t)cpu->regfile_[rs1] < (uint32_t)cpu->regfile_[rs2] ? 1 : 0;
	cpu->pc_ += 4;
}

void XOR(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	cpu->regfile_[rd] = cpu->regfile_[rs1] ^ cpu->regfile_[rs2];
	cpu->pc_ += 0x4;
}

void SRL(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	cpu->regfile_[rd] = cpu->regfile_[rs1] >> cpu->regfile_[rs2];
	cpu->pc_ += 0x4;
}

void SRA(CPU *cpu, uint32_t instruction) // TODO
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	cpu->regfile_[rd] = (int32_t)cpu->regfile_[rs1] >> cpu->regfile_[rs2];
	cpu->pc_ += 0x4;
}

void OR(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	cpu->regfile_[rd] = cpu->regfile_[rs1] | cpu->regfile_[rs2];
	cpu->pc_ += 0x4;
}

void AND(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	cpu->regfile_[rd] = cpu->regfile_[rs1] & cpu->regfile_[rs2];
	cpu->pc_ += 0x4;
}

//I-Type Instruction
void JALR1(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int32_t imm = imm_I(instruction);
	cpu->regfile_[rd] = cpu->pc_ + 0x4;
	cpu->pc_ = (cpu->regfile_[rs1] + ((int32_t)imm));
}

void LB(CPU *cpu, uint32_t instruction) // TODO
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int32_t imm = imm_I(instruction);
	uint8_t tmp = cpu->data_mem_[cpu->regfile_[rs1] + imm];

	//take last 8 bits
	if ((tmp & 0x80) > 1)
	{
		cpu->regfile_[rd] = 0xffffff00 | tmp;
	}
	else
	{
		cpu->regfile_[rd] = 0x000000ff & tmp;
	}

	cpu->pc_ += 0x4;
}

void LH(CPU *cpu, uint32_t instruction) // TODO
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int32_t imm = imm_I(instruction);
	uint16_t tmp = *(uint16_t *)(cpu->regfile_[rs1] + imm + cpu->data_mem_);

	//Take last 16 bits
	if ((tmp & 0x8000) > 1)
	{
		cpu->regfile_[rd] = 0xffff0000 | tmp;
	}
	else
	{
		cpu->regfile_[rd] = 0x0000ffff & tmp;
	}

	cpu->pc_ += 0x4;
}

void LW(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int32_t imm = imm_I(instruction);
	cpu->regfile_[rd] = *(uint32_t *)(cpu->regfile_[rs1] + imm + cpu->data_mem_);
	cpu->pc_ += 0x4;
}

void LBU(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int32_t imm = imm_I(instruction);
	cpu->regfile_[rd] = cpu->data_mem_[cpu->regfile_[rs1] + imm];
	cpu->pc_ += 0x4;
}

void LHU(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int32_t imm = imm_I(instruction);
	cpu->regfile_[rd] = *(uint16_t *)(cpu->regfile_[rs1] + imm + cpu->data_mem_);
	cpu->pc_ += 0x4;
}

void ADDI(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int32_t imm = imm_I(instruction);
	cpu->regfile_[rd] = cpu->regfile_[rs1] + imm;
	cpu->pc_ += 0x4;
}

void SLTI(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int32_t imm = imm_I(instruction);
	cpu->regfile_[rd] = cpu->regfile_[rs1] < imm ? 1 : 0;
	cpu->pc_ += 4;
}

void SLTIU(CPU *cpu, uint32_t instruction)
{
	uint8_t rd = getRD(instruction);
	uint8_t rs1 = getRS1(instruction);
	uint32_t imm = imm_I(instruction);
	cpu->regfile_[rd] = cpu->regfile_[rs1] < imm ? 1 : 0;
	cpu->pc_ += 4;
}

void XORI(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int32_t imm = imm_I(instruction);
	cpu->regfile_[rd] = cpu->regfile_[rs1] ^ imm;
	cpu->pc_ += 0x4;
}

void ORI(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int32_t imm = imm_I(instruction);
	cpu->regfile_[rd] = cpu->regfile_[rs1] | imm;
	cpu->pc_ += 0x4;
}

void ANDI(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int32_t imm = imm_I(instruction);
	cpu->regfile_[rd] = cpu->regfile_[rs1] & imm;
	cpu->pc_ += 0x4;
}

//S-Type Instructions
void SB(CPU *cpu, uint32_t instruction) 
{
	uint8_t rd = getRD(instruction);
	uint8_t rs1 = getRS1(instruction);
	uint8_t rs2 = getRS2(instruction);
	uint32_t imm = imm_S(instruction);

	//Print character for SB
	if ((cpu->regfile_[rs1]  == 0x5000))
	{
		putchar((char)cpu->regfile_[rs2]);
	}

	cpu->data_mem_[cpu->regfile_[rs1] + (int32_t)imm] = (uint8_t)cpu->regfile_[rs2];
	cpu->pc_ += 0x4;
}

void SH(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	int32_t imm = imm_S(instruction);
	*(uint16_t *)(cpu->data_mem_ + cpu->regfile_[rs1] + imm) = (uint16_t)cpu->regfile_[rs2];
	cpu->pc_ += 0x4;
}

void SW(CPU *cpu, uint32_t instruction) //PROBLEM HERE 
{
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	int32_t imm = imm_S(instruction);
	// printf("%d\n", imm);
	// fflush(stdout);

	//print imm s in decimal and then ffslush 
	//pritnf imm word -- 14 immediate 
	*(uint32_t *)(cpu->data_mem_ + cpu->regfile_[rs1] + (int32_t)imm) = (uint32_t)cpu->regfile_[rs2];
	cpu->pc_ += 0x4;
}

//B-Type Instruction
void BEQ(CPU *cpu, uint32_t instruction)
{
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	int32_t imm = imm_B(instruction);

	if (cpu->regfile_[rs1] == cpu->regfile_[rs2])
	{
		cpu->pc_ = cpu->pc_ + (int32_t)imm;
	}
	else
	{
		cpu->pc_ += 0x4;
	}
}

void BNE(CPU *cpu, uint32_t instruction)
{
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	int32_t imm = imm_B(instruction);

	if (cpu->regfile_[rs1] != cpu->regfile_[rs2])
	{
		cpu->pc_ = cpu->pc_ + (int32_t)imm;
	}
	else
	{
		cpu->pc_ += 0x4;
	}
}

void BLT(CPU *cpu, uint32_t instruction)
{
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	int32_t imm = imm_B(instruction);

	if (cpu->regfile_[rs1] < cpu->regfile_[rs2])
	{
		cpu->pc_ = cpu->pc_ + imm;
	}
	else
	{
		cpu->pc_ += 0x4;
	}
}

void BGE(CPU *cpu, uint32_t instruction)
{
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	int32_t imm = imm_B(instruction);

	if ((int32_t)cpu->regfile_[rs1] >= (int32_t)cpu->regfile_[rs2])
	{
		cpu->pc_ = cpu->pc_ + (int32_t)imm;
	}
	else
	{
		cpu->pc_ += 0x4;
	}
}

void BLTU(CPU *cpu, uint32_t instruction)
{
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	int32_t imm = imm_B(instruction);

	if ((uint32_t)cpu->regfile_[rs1] < (uint32_t)cpu->regfile_[rs2])
	{
		cpu->pc_ = cpu->pc_ + imm;
	}
	else
	{
		cpu->pc_ += 0x4;
	}
}

void BGEU(CPU *cpu, uint32_t instruction)
{
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	int32_t imm = imm_B(instruction);

	if ((uint32_t)cpu->regfile_[rs1] >= (uint32_t)cpu->regfile_[rs2])
	{
		cpu->pc_ = cpu->pc_ + imm;
	}
	else
	{
		cpu->pc_ += 0x4;
	}
}

//U_Type Instruction
void LUI1(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int32_t imm = imm_U(instruction);
	cpu->regfile_[rd] = imm;
	cpu->pc_ += 0x4;
}

void AUIPC1(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int32_t imm = imm_U(instruction);
	cpu->regfile_[rd] = cpu->pc_ + imm;
	cpu->pc_ += 0x4;
}

//J-Type Instruction
void JAL1(CPU *cpu, uint32_t instruction)
{
	//printf("%d", 66666);
	int8_t rd = getRD(instruction);
	int32_t imm = imm_J(instruction);
	cpu->regfile_[rd] = cpu->pc_ + 0x4;
	cpu->pc_ = cpu->pc_ + (int32_t)imm;
}

//Shift Instruction
void SLLI(CPU *cpu, uint32_t instruction)
{
	//printf("%d", 00000);
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);
	cpu->regfile_[rd] = cpu->regfile_[rs1] << rs2;
	cpu->pc_ += 0x04;
}

void SRLI(CPU *cpu, uint32_t instruction)
{
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);
	int8_t rs2 = getRS2(instruction);

	cpu->regfile_[rd] = cpu->regfile_[rs1] >> rs2;
	cpu->pc_ += 0x04;
}

void SRAI(CPU *cpu, uint32_t instruction) // TODO
{
	//printf("%d", 66666);
	//I am temporarily changing uint to int to check if it works (22.08.22)
	int32_t imm = shamt(instruction);
	int8_t rd = getRD(instruction);
	int8_t rs1 = getRS1(instruction);

	//######### with a star (vorzeichen)
	cpu->regfile_[rd] = (int32_t)cpu->regfile_[rs1] >> imm;
	cpu->pc_ += 0x04;
}

void CPU_execute(CPU *cpu)
{

	uint32_t instruction = *(uint32_t *)(cpu->instr_mem_ + (cpu->pc_ & 0xFFFFF));
	// TODO

	uint8_t opCode = getOpCode(instruction); //check if I need to do &(address) of instruction
	int8_t func3 = getFunc3(instruction);
	int8_t func7 = getFunc7(instruction);

	switch (opCode)
	{

	case R:
		switch (func3)
		{
		case (0x00):
			switch (func7)
			{
			case (0x00):
				ADD(cpu, instruction);
				break;
			case (0x20):
				SUB(cpu, instruction);
				break;
			}
			break;

		case (0x01):
			SLL(cpu, instruction);
			break;

		case (0x02):
			SLT(cpu, instruction);
			break;
		case (0x03):
			SLTU(cpu, instruction);
			break;
		case (0x04):
			XOR(cpu, instruction);
			break;

		case (0x05):
			switch (func7)
			{
			case (0x00):
				SRL(cpu, instruction);
				break;
			case (0x20):
				SRA(cpu, instruction);
				break;
			}
			break;

		case (0x06):
			OR(cpu, instruction);
			break;
		case (0x07):
			AND(cpu, instruction);
			break;

		default:;
		}
		break;

	case I:
		switch (func3)
		{
		case (0x00):
			ADDI(cpu, instruction);
			break;
		case (0x02):
			SLTI(cpu, instruction);
			break;
		case (0x01):
			SLLI(cpu, instruction);
			break;
		case (0x03):
			SLTIU(cpu, instruction);
			break;
		case (0x04):
			XORI(cpu, instruction);
			break;
		case (0x06):
			ORI(cpu, instruction);
			break;
		case (0x07):
			ANDI(cpu, instruction);
			break;
		case (0x05):
			switch (func7)
			{
			case (0x00):
				SRLI(cpu, instruction);
				break;
			case (0x20):
				SRAI(cpu, instruction);
				break;
			}
			break;
		}
		break;

	case S:
		switch (func3)
		{
		case (0x00):
			SB(cpu, instruction);
			break;
		case (0x01):
			SH(cpu, instruction);
			break;
		case (0x02):
			SW(cpu, instruction);
			break;
		}
		break;

	case L:
		switch (func3)
		{
		case (0x00):
			LB(cpu, instruction);
			break;
		case (0x01):
			LH(cpu, instruction);
			break;
		case (0x02):
			LW(cpu, instruction);
			break;
		case (0x04):
			LBU(cpu, instruction);
			break;
		case (0x05):
			LHU(cpu, instruction);
			break;
		}
		break;

	case B:
		switch (func3)
		{
		case (0x00):
			BEQ(cpu, instruction);
			break;
		case (0x01):
			BNE(cpu, instruction);
			break;
		case (0x04):
			BLT(cpu, instruction);
			break;
		case (0x05):
			BGE(cpu, instruction);
			break;
		case (0x06):
			BLTU(cpu, instruction);
			break;
		case (0x07):
			BGEU(cpu, instruction);
			break;
		}
		break;

	case LUI:
		LUI1(cpu, instruction);
		break;

	case AUIPC:
		AUIPC1(cpu, instruction);
		break;

	case JAL:
		JAL1(cpu, instruction);
		break;

	case JALR:
		JALR1(cpu, instruction);
		break;
	}

	cpu->regfile_[0] = 0;
}

int main(int argc, char *argv[])
{
	printf("C Praktikum\nHU Risc-V  Emulator 2022\n");

	CPU *cpu_inst;

	cpu_inst = CPU_init(argv[1], argv[2]);
	for (uint32_t i = 0; i < 1000000; i++)
	{ // run 70000 cycles
		CPU_execute(cpu_inst);
	}

	printf("\n-----------------------RISC-V program terminate------------------------\nRegfile values:\n");

	//output Regfile
	for (uint32_t i = 0; i <= 31; i++)
	{
		printf("%d: %X\n", i, cpu_inst->regfile_[i]);
	}

	//printf(%)
	fflush(stdout);


	return 0;
}
