#include "context.h"
#include "../error/logging.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../system/time.h"  // For time_get_ms()

// Global state context
static StateContext g_state_context;

void state_context_init(void) {
    memset(&g_state_context, 0, sizeof(StateContext));
    g_state_context.entry_time = deskthang_time_get_ms();
    g_state_context.current_state = STATE_HARDWARE_INIT;
    g_state_context.previous_state = STATE_HARDWARE_INIT;
    g_state_context.last_condition = CONDITION_NONE;
    g_state_context.can_retry = true;  // Start with retry enabled
}

void state_context_set_state(SystemState state, StateCondition condition) {
    g_state_context.previous_state = g_state_context.current_state;
    g_state_context.current_state = state;
    g_state_context.last_condition = condition;
    g_state_context.entry_time = deskthang_time_get_ms();
    g_state_context.last_update = deskthang_time_get_ms();
}

SystemState state_context_get_state(void) {
    return g_state_context.current_state;
}

SystemState state_context_get_previous(void) {
    return g_state_context.previous_state;
}

uint32_t state_context_get_duration(void) {
    return deskthang_time_get_ms() - g_state_context.entry_time;
}

uint32_t state_context_get_last_update(void) {
    return g_state_context.last_update;
}

bool state_context_set_data(void *data, size_t size) {
    if (data == NULL || size == 0) {
        state_context_clear_data();
        return true;
    }

    void *new_data = malloc(size);
    if (new_data == NULL) {
        return false;
    }

    memcpy(new_data, data, size);
    state_context_clear_data();
    g_state_context.state_data = new_data;
    g_state_context.data_size = size;
    return true;
}

void state_context_clear_data(void) {
    if (g_state_context.state_data != NULL) {
        free(g_state_context.state_data);
        g_state_context.state_data = NULL;
    }
    g_state_context.data_size = 0;
}

void *state_context_get_data(void) {
    return g_state_context.state_data;
}

void state_context_increment_error(void) {
    g_state_context.error_count++;
}

void state_context_reset_error(void) {
    g_state_context.error_count = 0;
}

uint32_t state_context_get_error_count(void) {
    return g_state_context.error_count;
}

bool state_context_can_retry(void) {
    return g_state_context.can_retry && g_state_context.retry_count < 3;  // Max 3 retries
}

void state_context_increment_retry(void) {
    g_state_context.retry_count++;
    if (g_state_context.retry_count >= 3) {  // Max 3 retries
        g_state_context.can_retry = false;
    }
}

void state_context_reset_retry(void) {
    g_state_context.retry_count = 0;
    g_state_context.can_retry = true;
}

uint32_t state_context_get_retry_count(void) {
    return g_state_context.retry_count;
}

bool context_can_retry(StateContext *ctx) {
    return ctx != NULL && ctx->can_retry && ctx->retry_count < 3;  // Max 3 retries
}

bool context_is_valid(const StateContext *ctx) {
    return ctx != NULL && state_machine_validate_state(ctx->current_state);
}

void context_clear_state_data(StateContext *ctx) {
    if (ctx != NULL && ctx->state_data != NULL) {
        free(ctx->state_data);
        ctx->state_data = NULL;
        ctx->data_size = 0;
    }
}
