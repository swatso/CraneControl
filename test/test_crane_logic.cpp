#include <unity.h>
#include "CraneController.h"

void test_command_word_values_are_mapped() {
    CraneController controller;

    controller.setCommand(CraneCommand::DisableAllMovement);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CraneCommand::DisableAllMovement), controller.command());

    controller.setCommand(CraneCommand::MoveToPosition1);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CraneCommand::MoveToPosition1), controller.command());

    controller.setCommand(CraneCommand::Home);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CraneCommand::Home), controller.command());
}

void test_position_targets_are_consistent() {
    CraneController controller;

    controller.setPulseCount(0);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CraneStatus::Home), controller.evaluateStatus());

    controller.setPulseCount(50);
    controller.setTargetPosition(CraneTargetPosition::Position1);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CraneStatus::Position1), controller.evaluateStatus());

    controller.setPulseCount(100);
    controller.setTargetPosition(CraneTargetPosition::Position2);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CraneStatus::Position2), controller.evaluateStatus());
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_command_word_values_are_mapped);
    RUN_TEST(test_position_targets_are_consistent);
    return UNITY_END();
}
