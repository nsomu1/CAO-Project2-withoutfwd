/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "apex_macros.h"

/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
struct BTB *initializeBTB()
{
    struct BTB *btb = (struct BTB *)malloc(sizeof(struct BTB));
    btb->capacity = BTB_SIZE;
    btb->front = btb->size = 0;
    btb->rear = BTB_SIZE - 1;
    btb->array = (CPU_Stage *)malloc(btb->capacity * sizeof(CPU_Stage));
    return btb;
}
void displayBTB(APEX_CPU *cpu)
{
    printf("\n");
    if (cpu->btb->size == 0)
    {
        printf("BTB is EMPTY\n");
    }
    else
    {
        printf("-------\n%s\n-------\n", "BTB:");
        printf("%-5s %-11s %-5s %-5s %-5s\n", "pc", "opcode_str", "bit0", "bit1", "target_address");
        for (int i = 0; i < cpu->btb->size; i++)
        {
            int index = (cpu->btb->front + i) % cpu->btb->capacity;
            printf("%-5d %-11s %-5d %-5d %-5d\n", cpu->btb->array[index].pc, cpu->btb->array[index].opcode_str, cpu->btb->array[index].bits[0], cpu->btb->array[index].bits[1], cpu->btb->array[index].target_address);
        }
    }
    printf("\n");
}
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
    case OPCODE_ADD:
    case OPCODE_SUB:
    case OPCODE_MUL:
    case OPCODE_AND:
    case OPCODE_OR:
    case OPCODE_XOR:
    {
        printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
               stage->rs2);
        break;
    }
    case OPCODE_ADDL:
    case OPCODE_SUBL:
    case OPCODE_JALR:
    {
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
               stage->imm);
        break;
    }

    case OPCODE_MOVC:
    {
        printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
        break;
    }

    case OPCODE_LOAD:
    case OPCODE_LOADP:
    {
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
               stage->imm);
        break;
    }

    case OPCODE_STORE:
    case OPCODE_STOREP:
    {
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
               stage->imm);
        break;
    }
    case OPCODE_CML:
    case OPCODE_JUMP:
    {
        printf("%s,R%d,#%d ", stage->opcode_str, stage->rs1, stage->imm);
        break;
    }
    case OPCODE_CMP:
    {
        printf("%s,R%d,R%d ", stage->opcode_str, stage->rs1, stage->rs2);
        break;
    }

    case OPCODE_BZ:
    case OPCODE_BNZ:
    case OPCODE_BP:
    case OPCODE_BNP:
    case OPCODE_BN:
    case OPCODE_BNN:
    {
        printf("%s,#%d ", stage->opcode_str, stage->imm);
        break;
    }

    case OPCODE_HALT:
    case OPCODE_NOP:
    {
        printf("%s", stage->opcode_str);
        break;
    }
    }
}

/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{
    printf("%-15s: pc(%d) ", name, stage->pc);
    print_instruction(stage);
    printf("\n");
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_reg_file(const APEX_CPU *cpu)
{
    int i;

    printf("----------\n%s\n----------\n", "Registers:");

    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-1d[%-1d] ", i, cpu->regs[i]);
    }

    printf("\n");

    for (i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
    {
        printf("R%-1d[%-1d] ", i, cpu->regs[i]);
    }

    printf("\n");
}
static void
print_data_memory(APEX_CPU *cpu)
{
    int start = 0, count = 0;
    while (start < DATA_MEMORY_SIZE)
    {

        if (cpu->data_memory[start] != 0)
        {
            printf("MEM[%d] = %d\n", start, cpu->data_memory[start]);
            count++;
        }
        start++;
    }
    if (count == 0)
    {
        printf("All the memory values are zeros\n");
    }
}
/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_fetch(APEX_CPU *cpu)
{
    APEX_Instruction *current_ins;

    if (cpu->fetch.has_insn)
    {
        /* This fetches new branch target instruction from next cycle */
        if (cpu->fetch_from_next_cycle == TRUE)
        {
            cpu->fetch_from_next_cycle = FALSE;

            /* Skip this cycle*/
            return;
        }

        /* Store current PC in fetch latch */
        cpu->fetch.pc = cpu->pc;

        /* Index into code memory using this pc and copy all instruction fields
         * into fetch latch  */
        current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
        strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
        cpu->fetch.opcode = current_ins->opcode;
        cpu->fetch.rd = current_ins->rd;
        cpu->fetch.rs1 = current_ins->rs1;
        cpu->fetch.rs2 = current_ins->rs2;
        cpu->fetch.imm = current_ins->imm;

        if (cpu->stall_flag == 0)
        {
            /* Update PC for next instruction */
            cpu->pc += 4;
            /* Copy data from fetch latch to decode latch*/
            cpu->decode = cpu->fetch;
            if (cpu->fetch.opcode == OPCODE_HALT)
            {
                cpu->fetch.has_insn = FALSE;
            }
        }

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Fetch", &cpu->fetch);
        }

        /* Stop fetching new instructions if HALT is fetched */
    }
}

/*
 * Decode Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode(APEX_CPU *cpu)
{
    if (cpu->decode.has_insn)
    {
        /* Read operands from register file based on the instruction type */
        switch (cpu->decode.opcode)
        {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_OR:
        case OPCODE_AND:
        case OPCODE_XOR:
        {

            if (cpu->flag[cpu->decode.rs1] == 0 && cpu->flag[cpu->decode.rs2] == 0)
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                cpu->flag[cpu->decode.rd] = 1;
                cpu->stall_flag = 0;
            }
            else
            {
                cpu->stall_flag = 1;
            }
            break;
        }
        case OPCODE_ADDL:
        case OPCODE_SUBL:
        case OPCODE_LOAD:
        case OPCODE_JALR:
        {

            if (cpu->flag[cpu->decode.rs1] == 0)
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->flag[cpu->decode.rd] = 1;
                cpu->stall_flag = 0;
            }
            else
            {
                cpu->stall_flag = 1;
            }

            break;
        }
        case OPCODE_LOADP:
        {

            if (cpu->flag[cpu->decode.rs1] == 0)
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->flag[cpu->decode.rd] = 1;
                cpu->flag[cpu->decode.rs1] = 1;
                cpu->stall_flag = 0;
            }
            else
            {
                cpu->stall_flag = 1;
            }

            break;
        }
        case OPCODE_CML:
        case OPCODE_JUMP:
        {
            if (cpu->flag[cpu->decode.rs1] == 0)
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->stall_flag = 0;
            }
            else
            {
                cpu->stall_flag = 1;
            }
            break;
        }
        case OPCODE_CMP:
        case OPCODE_STORE:
        {
            if (cpu->flag[cpu->decode.rs1] == 0 && cpu->flag[cpu->decode.rs2] == 0)
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                cpu->stall_flag = 0;
            }
            else
            {
                cpu->stall_flag = 1;
            }
            break;
        }
        case OPCODE_STOREP:
        {
            if (cpu->flag[cpu->decode.rs1] == 0 && cpu->flag[cpu->decode.rs2] == 0)
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                cpu->flag[cpu->decode.rs2] = 1;
                cpu->stall_flag = 0;
            }
            else
            {
                cpu->stall_flag = 1;
            }
            break;
        }
        case OPCODE_MOVC:
        {
            /* MOVC doesn't have register operands */
            cpu->flag[cpu->decode.rd] = 1;
            break;
        }
        }
        switch (cpu->decode.opcode)
        {
        case OPCODE_JALR:
        case OPCODE_JUMP:
        {
            cpu->fetch.has_insn = FALSE;
            break;
        }
        }

        /* Copy data from decode latch to execute latch*/
        if (cpu->stall_flag == 0)
        {
            if (cpu->decode.opcode == OPCODE_BNZ || cpu->decode.opcode == OPCODE_BP)
            {

                int count = 0;
                if (cpu->btb->size != 0)
                {
                    for (int i = 0; i < cpu->btb->size; i++)
                    {
                        int index = (cpu->btb->front + i) % cpu->btb->capacity;
                        {
                            if (cpu->btb->array[index].pc == cpu->decode.pc)
                            {
                                count = 1;
                                if ((cpu->btb->array[index].bits[0] == 1 && cpu->btb->array[index].bits[1] == 1) ||
                                    (cpu->btb->array[index].bits[0] == 1 && cpu->btb->array[index].bits[1] == 0) ||
                                    (cpu->btb->array[index].bits[0] == 0 && cpu->btb->array[index].bits[1] == 1))
                                {
                                    cpu->pc = cpu->btb->array[index].target_address;
                                    cpu->btb->array[index].taken = 1;
                                }
                            }
                        }
                    }
                }
                if (count == 0 && cpu->btb->size == cpu->btb->capacity)
                {
                    cpu->btb->front = (cpu->btb->front + 1) % cpu->btb->capacity;
                    cpu->btb->size--;
                }
                if (count == 0)
                {
                    cpu->btb->rear = (cpu->btb->rear + 1) % cpu->btb->capacity;
                    cpu->btb->array[cpu->btb->rear].pc = cpu->decode.pc;
                    strcpy(cpu->btb->array[cpu->btb->rear].opcode_str, cpu->decode.opcode_str);
                    cpu->btb->array[cpu->btb->rear].target_address = cpu->pc + cpu->decode.imm - 4;
                    cpu->btb->array[cpu->btb->rear].bits[0] = 1;
                    cpu->btb->array[cpu->btb->rear].bits[1] = 1;
                    cpu->btb->array[cpu->btb->rear].taken = 0;
                    cpu->btb->size++;
                }
            }
            if (cpu->decode.opcode == OPCODE_BZ || cpu->decode.opcode == OPCODE_BNP)
            {

                int count = 0;
                if (cpu->btb->size != 0)
                {
                    for (int i = 0; i < cpu->btb->size; i++)
                    {
                        int index = (cpu->btb->front + i) % cpu->btb->capacity;
                        {
                            if (cpu->btb->array[index].pc == cpu->decode.pc)
                            {
                                count = 1;
                                if ((cpu->btb->array[index].bits[0] == 1 && cpu->btb->array[index].bits[1] == 1))
                                {
                                    cpu->pc = cpu->btb->array[index].target_address;
                                    cpu->btb->array[index].taken = 1;
                                }
                            }
                        }
                    }
                }
                if (count == 0 && cpu->btb->size == cpu->btb->capacity)
                {
                    cpu->btb->front = (cpu->btb->front + 1) % cpu->btb->capacity;
                    cpu->btb->size--;
                }

                if (count == 0)
                {
                    cpu->btb->rear = (cpu->btb->rear + 1) % cpu->btb->capacity;
                    cpu->btb->array[cpu->btb->rear].pc = cpu->decode.pc;
                    strcpy(cpu->btb->array[cpu->btb->rear].opcode_str, cpu->decode.opcode_str);
                    cpu->btb->array[cpu->btb->rear].target_address = cpu->pc + cpu->decode.imm - 4;
                    cpu->btb->array[cpu->btb->rear].bits[0] = 0;
                    cpu->btb->array[cpu->btb->rear].bits[1] = 0;
                    cpu->btb->array[cpu->btb->rear].taken = 0;
                    cpu->btb->size++;
                }
            }
            cpu->execute = cpu->decode;
            cpu->decode.has_insn = FALSE;
        }
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Decode/RF", &cpu->decode);
        }
    }
}

/*
 * Execute Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_execute(APEX_CPU *cpu)
{
    if (cpu->execute.has_insn)
    {
        /* Execute logic based on instruction type */
        switch (cpu->execute.opcode)
        {
        case OPCODE_MOVC:
        {
            cpu->execute.result_buffer = cpu->execute.imm;
            break;
        }
        case OPCODE_ADD:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.rs2_value;

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            if (cpu->execute.result_buffer > 0)
            {
                cpu->positive_flag = TRUE;
            }
            else
            {
                cpu->positive_flag = FALSE;
            }
            if (cpu->execute.result_buffer < 0)
            {
                cpu->negative_flag = TRUE;
            }
            else
            {
                cpu->negative_flag = FALSE;
            }
            break;
        }
        case OPCODE_ADDL:
        {

            cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.imm;

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            if (cpu->execute.result_buffer > 0)
            {
                cpu->positive_flag = TRUE;
            }
            else
            {
                cpu->positive_flag = FALSE;
            }
            if (cpu->execute.result_buffer < 0)
            {
                cpu->negative_flag = TRUE;
            }
            else
            {
                cpu->negative_flag = FALSE;
            }
            break;
        }
        case OPCODE_SUB:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.rs2_value;

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            if (cpu->execute.result_buffer > 0)
            {
                cpu->positive_flag = TRUE;
            }
            else
            {
                cpu->positive_flag = FALSE;
            }
            if (cpu->execute.result_buffer < 0)
            {
                cpu->negative_flag = TRUE;
            }
            else
            {
                cpu->negative_flag = FALSE;
            }
            break;
        }
        case OPCODE_SUBL:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.imm;

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            if (cpu->execute.result_buffer > 0)
            {
                cpu->positive_flag = TRUE;
            }
            else
            {
                cpu->positive_flag = FALSE;
            }
            if (cpu->execute.result_buffer < 0)
            {
                cpu->negative_flag = TRUE;
            }
            else
            {
                cpu->negative_flag = FALSE;
            }
            break;
        }
        case OPCODE_MUL:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value * cpu->execute.rs2_value;

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            if (cpu->execute.result_buffer > 0)
            {
                cpu->positive_flag = TRUE;
            }
            else
            {
                cpu->positive_flag = FALSE;
            }
            if (cpu->execute.result_buffer < 0)
            {
                cpu->negative_flag = TRUE;
            }
            else
            {
                cpu->negative_flag = FALSE;
            }
            break;
        }
        case OPCODE_OR:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value | cpu->execute.rs2_value;

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            if (cpu->execute.result_buffer > 0)
            {
                cpu->positive_flag = TRUE;
            }
            else
            {
                cpu->positive_flag = FALSE;
            }
            if (cpu->execute.result_buffer < 0)
            {
                cpu->negative_flag = TRUE;
            }
            else
            {
                cpu->negative_flag = FALSE;
            }
            break;
        }
        case OPCODE_AND:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value & cpu->execute.rs2_value;

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            if (cpu->execute.result_buffer > 0)
            {
                cpu->positive_flag = TRUE;
            }
            else
            {
                cpu->positive_flag = FALSE;
            }
            if (cpu->execute.result_buffer < 0)
            {
                cpu->negative_flag = TRUE;
            }
            else
            {
                cpu->negative_flag = FALSE;
            }
            break;
        }
        case OPCODE_XOR:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value ^ cpu->execute.rs2_value;

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            if (cpu->execute.result_buffer > 0)
            {
                cpu->positive_flag = TRUE;
            }
            else
            {
                cpu->positive_flag = FALSE;
            }
            if (cpu->execute.result_buffer < 0)
            {
                cpu->negative_flag = TRUE;
            }
            else
            {
                cpu->negative_flag = FALSE;
            }
            break;
        }

        case OPCODE_LOAD:
        {
            cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
            break;
        }
        case OPCODE_STORE:
        {
            cpu->execute.memory_address = cpu->execute.rs2_value + cpu->execute.imm;
            break;
        }
        case OPCODE_LOADP:
        {
            cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
            break;
        }
        case OPCODE_STOREP:
        {
            cpu->execute.memory_address = cpu->execute.rs2_value + cpu->execute.imm;
            break;
        }

        case OPCODE_CML:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.imm;
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            if (cpu->execute.result_buffer > 0)
            {
                cpu->positive_flag = TRUE;
            }
            else
            {
                cpu->positive_flag = FALSE;
            }
            if (cpu->execute.result_buffer < 0)
            {
                cpu->negative_flag = TRUE;
            }
            else
            {
                cpu->negative_flag = FALSE;
            }
            break;
        }
        case OPCODE_CMP:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.rs2_value;
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            if (cpu->execute.result_buffer > 0)
            {
                cpu->positive_flag = TRUE;
            }
            else
            {
                cpu->positive_flag = FALSE;
            }
            if (cpu->execute.result_buffer < 0)
            {
                cpu->negative_flag = TRUE;
            }
            else
            {
                cpu->negative_flag = FALSE;
            }
            break;
        }

        case OPCODE_BP:
        {
            if (cpu->positive_flag == TRUE)
            {
                for (int i = 0; i < cpu->btb->size; i++)
                {
                    int index = (cpu->btb->front + i) % cpu->btb->capacity;
                    {
                        if (cpu->btb->array[index].pc == cpu->execute.pc)
                        {
                            cpu->btb->array[index].bits[0] = cpu->btb->array[index].bits[1];
                            cpu->btb->array[index].bits[1] = 1;
                            if (cpu->btb->array[index].taken == 0)
                            {
                                cpu->pc = cpu->execute.pc + cpu->execute.imm;

                                cpu->fetch_from_next_cycle = TRUE;

                                cpu->decode.has_insn = FALSE;

                                cpu->fetch.has_insn = TRUE;

                                cpu->btb->array[index].taken = 0;
                            }
                        }
                    }
                }
            }

            else
            {
                for (int i = 0; i < cpu->btb->size; i++)
                {
                    int index = (cpu->btb->front + i) % cpu->btb->capacity;
                    {
                        if (cpu->btb->array[index].pc == cpu->execute.pc)
                        {
                            cpu->btb->array[index].bits[0] = cpu->btb->array[index].bits[1];
                            cpu->btb->array[index].bits[1] = 0;
                            if (cpu->btb->array[index].taken == 1)
                            {
                                cpu->pc = cpu->execute.pc + 4;

                                cpu->fetch_from_next_cycle = TRUE;

                                cpu->decode.has_insn = FALSE;

                                cpu->fetch.has_insn = TRUE;

                                cpu->btb->array[index].taken = 0;
                            }
                        }
                    }
                }
            }
            break;
        }

        case OPCODE_BNZ:
        {
            if (cpu->zero_flag == FALSE)
            {

                for (int i = 0; i < cpu->btb->size; i++)
                {
                    int index = (cpu->btb->front + i) % cpu->btb->capacity;
                    {
                        if (cpu->btb->array[index].pc == cpu->execute.pc)
                        {
                            cpu->btb->array[index].bits[0] = cpu->btb->array[index].bits[1];
                            cpu->btb->array[index].bits[1] = 1;
                            if (cpu->btb->array[index].taken != 1)
                            {
                                cpu->pc = cpu->execute.pc + cpu->execute.imm;

                                cpu->fetch_from_next_cycle = TRUE;

                                cpu->decode.has_insn = FALSE;

                                cpu->fetch.has_insn = TRUE;

                                cpu->btb->array[index].taken = 0;
                            }
                        }
                    }
                }
            }

            else
            {
                for (int i = 0; i < cpu->btb->size; i++)
                {
                    int index = (cpu->btb->front + i) % cpu->btb->capacity;
                    {
                        if (cpu->btb->array[index].pc == cpu->execute.pc)
                        {
                            cpu->btb->array[index].bits[0] = cpu->btb->array[index].bits[1];
                            cpu->btb->array[index].bits[1] = 0;
                            if (cpu->btb->array[index].taken == 1)
                            {
                                cpu->pc = cpu->execute.pc + 4;

                                cpu->fetch_from_next_cycle = TRUE;

                                cpu->decode.has_insn = FALSE;

                                cpu->fetch.has_insn = TRUE;

                                cpu->btb->array[index].taken = 0;
                            }
                        }
                    }
                }
            }
            break;
        }
        case OPCODE_BZ:
        {
            if (cpu->zero_flag == TRUE)
            {

                for (int i = 0; i < cpu->btb->size; i++)
                {
                    int index = (cpu->btb->front + i) % cpu->btb->capacity;
                    {
                        if (cpu->btb->array[index].pc == cpu->execute.pc)
                        {
                            cpu->btb->array[index].bits[0] = cpu->btb->array[index].bits[1];
                            cpu->btb->array[index].bits[1] = 1;
                            if (cpu->btb->array[index].taken != 1)
                            {
                                cpu->pc = cpu->execute.pc + cpu->execute.imm;

                                cpu->fetch_from_next_cycle = TRUE;

                                cpu->decode.has_insn = FALSE;

                                cpu->fetch.has_insn = TRUE;

                                cpu->btb->array[index].taken = 0;
                            }
                        }
                    }
                }
            }

            else
            {
                for (int i = 0; i < cpu->btb->size; i++)
                {
                    int index = (cpu->btb->front + i) % cpu->btb->capacity;
                    {
                        if (cpu->btb->array[index].pc == cpu->execute.pc)
                        {
                            cpu->btb->array[index].bits[0] = cpu->btb->array[index].bits[1];
                            cpu->btb->array[index].bits[1] = 0;
                            if (cpu->btb->array[index].taken == 1)
                            {
                                cpu->pc = cpu->execute.pc + 4;

                                cpu->fetch_from_next_cycle = TRUE;

                                cpu->decode.has_insn = FALSE;

                                cpu->fetch.has_insn = TRUE;

                                cpu->btb->array[index].taken = 0;
                            }
                        }
                    }
                }
            }
            break;
        }

        case OPCODE_BNP:
        {
            if (cpu->positive_flag == FALSE)
            {
                for (int i = 0; i < cpu->btb->size; i++)
                {
                    int index = (cpu->btb->front + i) % cpu->btb->capacity;
                    {
                        if (cpu->btb->array[index].pc == cpu->execute.pc)
                        {
                            cpu->btb->array[index].bits[0] = cpu->btb->array[index].bits[1];
                            cpu->btb->array[index].bits[1] = 1;
                            if (cpu->btb->array[index].taken != 1)
                            {
                                cpu->pc = cpu->execute.pc + cpu->execute.imm;

                                cpu->fetch_from_next_cycle = TRUE;

                                cpu->decode.has_insn = FALSE;

                                cpu->fetch.has_insn = TRUE;

                                cpu->btb->array[index].taken = 0;
                            }
                        }
                    }
                }
            }

            else
            {
                for (int i = 0; i < cpu->btb->size; i++)
                {
                    int index = (cpu->btb->front + i) % cpu->btb->capacity;
                    {
                        if (cpu->btb->array[index].pc == cpu->execute.pc)
                        {
                            cpu->btb->array[index].bits[0] = cpu->btb->array[index].bits[1];
                            cpu->btb->array[index].bits[1] = 0;
                            if (cpu->btb->array[index].taken == 1)
                            {
                                cpu->pc = cpu->execute.pc + 4;

                                cpu->fetch_from_next_cycle = TRUE;

                                cpu->decode.has_insn = FALSE;

                                cpu->fetch.has_insn = TRUE;

                                cpu->btb->array[index].taken = 0;
                            }
                        }
                    }
                }
            }
            break;
        }
        case OPCODE_BN:
        {
            if (cpu->negative_flag == TRUE)
            {
                cpu->pc = cpu->execute.pc + cpu->execute.imm;

                cpu->fetch_from_next_cycle = TRUE;

                cpu->decode.has_insn = FALSE;

                cpu->fetch.has_insn = TRUE;
            }
            break;
        }

        case OPCODE_BNN:
        {
            if (cpu->negative_flag == FALSE)
            {
                cpu->pc = cpu->execute.pc + cpu->execute.imm;

                cpu->fetch_from_next_cycle = TRUE;

                cpu->decode.has_insn = FALSE;

                cpu->fetch.has_insn = TRUE;
            }
            break;
        }
        case OPCODE_JALR:
        {
            cpu->pc = cpu->execute.rs1_value + cpu->execute.imm;

            cpu->fetch_from_next_cycle = TRUE;

            cpu->decode.has_insn = FALSE;

            cpu->fetch.has_insn = TRUE;
            cpu->execute.result_buffer = cpu->execute.pc + 4;
            break;
        }
        case OPCODE_JUMP:
        {
            cpu->pc = cpu->execute.rs1_value + cpu->execute.imm;

            cpu->fetch_from_next_cycle = TRUE;

            cpu->decode.has_insn = FALSE;

            cpu->fetch.has_insn = TRUE;
            break;
        }
        }

        /* Copy data from execute latch to memory latch*/
        cpu->memory = cpu->execute;
        cpu->execute.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Execute", &cpu->execute);
        }
    }
}

/*
 * Memory Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_memory(APEX_CPU *cpu)
{
    if (cpu->memory.has_insn)
    {
        switch (cpu->memory.opcode)
        {

        case OPCODE_LOAD:
        {
            /* Read from data memory */
            cpu->memory.result_buffer = cpu->data_memory[cpu->memory.memory_address];
            break;
        }
        case OPCODE_STORE:
        {
            /* Read from data memory */
            cpu->data_memory[cpu->memory.memory_address] = cpu->memory.rs1_value;
            break;
        }
        case OPCODE_LOADP:
        {
            /* Read from data memory */
            cpu->memory.result_buffer = cpu->data_memory[cpu->memory.memory_address];
            break;
        }
        case OPCODE_STOREP:
        {
            /* Read from data memory */
            cpu->data_memory[cpu->memory.memory_address] = cpu->memory.rs1_value;
            break;
        }
        }

        /* Copy data from memory latch to writeback latch*/
        cpu->writeback = cpu->memory;
        cpu->memory.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Memory", &cpu->memory);
        }
    }
}

/*
 * Writeback Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static int
APEX_writeback(APEX_CPU *cpu)
{
    if (cpu->writeback.has_insn)
    {

        /* Write result to register file based on instruction type */
        switch (cpu->writeback.opcode)
        {
        case OPCODE_ADD:
        case OPCODE_ADDL:
        case OPCODE_SUB:
        case OPCODE_SUBL:
        case OPCODE_MUL:
        case OPCODE_OR:
        case OPCODE_AND:
        case OPCODE_XOR:
        case OPCODE_LOAD:
        case OPCODE_MOVC:
        case OPCODE_JALR:
        {
            cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
            if ((cpu->writeback.rd == cpu->memory.rd && cpu->memory.has_insn == TRUE) || (cpu->writeback.rd == cpu->execute.rd && cpu->execute.has_insn == TRUE))
            {
                cpu->flag[cpu->writeback.rd] = 1;
            }
            else
            {
                cpu->flag[cpu->writeback.rd] = 0;
            }
            break;
        }
        case OPCODE_LOADP:
        {
            cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
            cpu->regs[cpu->writeback.rs1] = cpu->writeback.rs1_value + 4;
            if ((cpu->writeback.rd == cpu->memory.rd && cpu->memory.has_insn == TRUE) || (cpu->writeback.rd == cpu->execute.rd && cpu->execute.has_insn == TRUE))
            {
                cpu->flag[cpu->writeback.rd] = 1;
            }
            else
            {
                cpu->flag[cpu->writeback.rd] = 0;
            }
            if ((cpu->writeback.rs1 == cpu->memory.rd && cpu->memory.has_insn == TRUE) || (cpu->writeback.rs1 == cpu->execute.rd && cpu->execute.has_insn == TRUE))
            {
                cpu->flag[cpu->writeback.rs1] = 1;
            }
            else
            {
                cpu->flag[cpu->writeback.rs1] = 0;
            }
            break;
        }
        case OPCODE_STOREP:
        {
            cpu->regs[cpu->writeback.rs2] = cpu->writeback.rs2_value + 4;
            if ((cpu->writeback.rd == cpu->memory.rd && cpu->memory.has_insn == TRUE) || (cpu->writeback.rd == cpu->execute.rd && cpu->execute.has_insn == TRUE))
            {
                cpu->flag[cpu->writeback.rd] = 1;
            }
            else
            {
                cpu->flag[cpu->writeback.rd] = 0;
            }
            if ((cpu->writeback.rs2 == cpu->memory.rd && cpu->memory.has_insn == TRUE) || (cpu->writeback.rs2 == cpu->execute.rd && cpu->execute.has_insn == TRUE))
            {
                cpu->flag[cpu->writeback.rs2] = 1;
            }
            else
            {
                cpu->flag[cpu->writeback.rs2] = 0;
            }
            break;
        }
        }

        cpu->insn_completed++;
        cpu->writeback.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Writeback", &cpu->writeback);
        }

        if (cpu->writeback.opcode == OPCODE_HALT)
        {
            /* Stop the APEX simulator */
            return TRUE;
        }
    }

    /* Default */
    return 0;
}

/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename)
{
    int i;
    APEX_CPU *cpu;

    if (!filename)
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);
    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);
    cpu->single_step = ENABLE_SINGLE_STEP;

    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }
    cpu->btb = initializeBTB();

    if (ENABLE_DEBUG_MESSAGES)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2",
               "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;
    return cpu;
}

/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void APEX_cpu_run(APEX_CPU *cpu)
{
    char user_prompt_val;

    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock + 1);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock + 1, cpu->insn_completed);
            break;
        }

        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);
        printf("--------------\n%s\n--------------\n", "Memory Values:");
        print_data_memory(cpu);
        printf("-------\n%s\n-------\n", "Flags:");
        printf("P = %d\n", cpu->positive_flag);
        printf("Z = %d\n", cpu->zero_flag);
        printf("N = %d\n", cpu->negative_flag);
        displayBTB(cpu);

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock + 1, cpu->insn_completed);
                break;
            }
        }

        cpu->clock++;
    }
}
void APEX_cpu_simulate(APEX_CPU *cpu, int cycles)
{
    while (cycles != 0)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle: %d\n", cpu->clock + 1);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock + 1, cpu->insn_completed);
            break;
        }

        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);
        printf("--------------\n%s\n--------------\n", "Memory Values:");
        print_data_memory(cpu);
        printf("-------\n%s\n-------\n", "Flags:");
        printf("P = %d\n", cpu->positive_flag);
        printf("Z = %d\n", cpu->zero_flag);
        printf("N = %d\n", cpu->negative_flag);
        displayBTB(cpu);
        cpu->clock++;
        cycles--;
    }
}
void APEX_cpu_display(APEX_CPU *cpu)
{
    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle: %d\n", cpu->clock + 1);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock + 1, cpu->insn_completed);
            break;
        }

        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);
        printf("--------------\n%s\n--------------\n", "Memory Values:");
        print_data_memory(cpu);
        printf("-------\n%s\n-------\n", "Flags:");
        printf("P = %d\n", cpu->positive_flag);
        printf("Z = %d\n", cpu->zero_flag);
        printf("N = %d\n", cpu->negative_flag);
        displayBTB(cpu);
        cpu->clock++;
    }
}
void APEX_cpu_show_mem(APEX_CPU *cpu, int mem_loc)
{
    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle: %d\n", cpu->clock + 1);
            printf("--------------------------------------------\n");
        }
        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock + 1, cpu->insn_completed);
            break;
        }
        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);
        printf("--------------\n%s\n--------------\n", "Memory Values:");
        print_data_memory(cpu);
        printf("-------\n%s\n-------\n", "Flags:");
        printf("P = %d\n", cpu->positive_flag);
        printf("Z = %d\n", cpu->zero_flag);
        printf("N = %d\n", cpu->negative_flag);
        displayBTB(cpu);
        cpu->clock++;
    }
    printf("\nValue at Memory Location is MEM[%d]  = %d\n", mem_loc, cpu->data_memory[mem_loc]);
}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu->code_memory);
    free(cpu);
}