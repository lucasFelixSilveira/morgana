#pragma once

namespace context {
    const int MCU_GPIO_INSTRUCTION = 1000;
    const int MCU_TURN_INSTRUCTION = 1001;
    const int MCU_READ_INSTRUCTION = 1002;

    const int ALL_ALLOC_INSTRUCTION = 0;
    const int ALL_STORE_INSTRUCTION = 1;
    const int ALL_LOAD_INSTRUCTION = 2;

    const int ALL_CALL_INSTRUCTION = 3;
    const int ALL_RETURN_INSTRUCTION = 4;

    const int ALL_BRANCH_INSTRUCTION = 5;
    const int ALL_BRANCH_IF_NOT_EQUALS_ZERO_INSTRUCTION = 6;
    const int ALL_BRANCH_IF_IS_ZERO_INSTRUCTION = 7;
    const int ALL_BRANCH_IF_GREATER_INSTRUCTION = 8;
    const int ALL_BRANCH_IF_LESS_INSTRUCTION = 9;

    const int ALL_LOOP_STATEMENT = 10;
    const int ALL_NEXT_STATEMENT = 11;
    const int ALL_STOP_STATEMENT = 12;
}
