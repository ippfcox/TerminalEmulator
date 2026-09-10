#pragma once

#define CHECK_EXPR_RET_(level, expr, ret, fmt...) \
    do                                            \
    {                                             \
        if (expr)                                 \
        {                                         \
            SPDLOG_##level(fmt);                  \
            return ret;                           \
        }                                         \
    }                                             \
    while (0)

#define CHECK_EXPR_GOTO_(level, expr, label, fmt...) \
    do                                               \
    {                                                \
        if (expr)                                    \
        {                                            \
            SPDLOG_##level(fmt);                     \
            goto label;                              \
        }                                            \
    }                                                \
    while (0)

#define CHECK_EXPR_EXIT_(level, code, expr, fmt...) \
    do                                              \
    {                                               \
        if (expr)                                   \
        {                                           \
            SPDLOG_##level(fmt);                    \
            exit(code);                             \
        }                                           \
    }                                               \
    while (0)

#define CHECK_EXPR_RET_ERROR(expr, ret, fmt...) CHECK_EXPR_RET_(ERROR, expr, ret, fmt)

#define CHECK_EXPR_GOTO_ERROR(expr, label, fmt...) CHECK_EXPR_GOTO_(ERROR, expr, label, fmt)

#define CHECK_EXPR_EXIT_ERROR(expr, code, fmt...) CHECK_EXPR_EXIT_(ERROR, expr, code, fmt)