#pragma once

namespace context {
    const int MCU_GPIO_INSTRUCTION = 1000;
    const int MCU_TURN_INSTRUCTION = 1001;
    const int MCU_READ_INSTRUCTION = 1002;

    const int ALL_ALLOC_INSTRUCTION                     = 0;
    const int ALL_STORE_INSTRUCTION                     = 1;
    const int ALL_LOAD_INSTRUCTION                      = 2;

    const int ALL_CALL_INSTRUCTION                      = 100;
    const int ALL_RETURN_INSTRUCTION                    = 101;

    const int ALL_BRANCH_INSTRUCTION                    = 200;
    const int ALL_BRANCH_IF_NOT_EQUALS_ZERO_INSTRUCTION = 201;
    const int ALL_BRANCH_IF_EQUALS_ZERO_INSTRUCTION     = 202;
    const int ALL_BRANCH_IF_IS_ZERO_INSTRUCTION         = 203;
    const int ALL_BRANCH_IF_GREATER_INSTRUCTION         = 204;
    const int ALL_BRANCH_IF_LESS_INSTRUCTION            = 205;
    const int ALL_BRANCH_IF_LESS_EQUAL_INSTRUCTION      = 206;
    const int ALL_BRANCH_IF_GREATER_EQUAL_INSTRUCTION   = 207;

    const int ALL_WAIT_STATEMENT                        = 300;
    const int ALL_LOOP_STATEMENT                        = 301;
    const int ALL_NEXT_STATEMENT                        = 302;
    const int ALL_STOP_STATEMENT                        = 303;

    const int ALL_OPERATION_INSTRUCTION                 = 400;
    const int ALL_ALERT_INSTRUCTION                     = 401;

    const int ALL_TUPLE_INSTRUCTION                     = 500;
    const int ALL_ADDINPTR_INSTRUCTION                  = 501;

}
